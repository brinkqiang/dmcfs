#include "dmcfs.h"
#include "dmcfs_task.h"
#include "gtest.h"
#include <string>

class env_dmcfs
{
public:
    void init(){}
    void uninit(){}
};

class frame_dmcfs : public testing::Test
{
public:
    virtual void SetUp()
    {
        env.init();
    }
    virtual void TearDown()
    {
        env.uninit();
    }
protected:
    env_dmcfs env;
};

// 简单的 Mock Task
class MockTask : public Idmcfs_task {
public:
    MockTask(uint32_t id, const char* name, int nice) 
        : m_id(id), m_name(name), m_nice(nice) {}

    virtual uint32_t getId() const override { return m_id; }
    virtual const char* getName() const override { return m_name.c_str(); }
    virtual int getNiceValue() const override { return m_nice; }

    virtual TaskRunResult run(uint32_t requested_count) override {
        // 模拟正常工作，消耗了一些单位时间
        TaskRunResult res;
        res.actual_count = requested_count;
        res.consumed_ms = 10; 
        run_count++;
        return res;
    }

    virtual DmcfsSchedulingState& getSchedulingState() override { return m_sched_state; }
    virtual DmcfsTuningState& getTuningState() override { return m_tune_state; }

    uint32_t m_id;
    std::string m_name;
    int m_nice;
    int run_count = 0;
    
private:
    DmcfsSchedulingState m_sched_state;
    DmcfsTuningState m_tune_state;
};

// 测试异常的 Task
class ThrowingTask : public MockTask {
public:
    ThrowingTask(uint32_t id, const char* name, int nice) 
        : MockTask(id, name, nice) {}

    virtual TaskRunResult run(uint32_t requested_count) override {
        throw std::runtime_error("Simulated crash");
    }
};


TEST_F(frame_dmcfs, init)
{
    Idmcfs* module = dmcfsGetModule();
    ASSERT_TRUE(module != nullptr);
    module->Release();
}

TEST_F(frame_dmcfs, add_and_dispatch)
{
    Idmcfs* cfs = dmcfsGetModule();
    ASSERT_TRUE(cfs != nullptr);

    MockTask task1(1, "Task_A", 0);
    MockTask task2(2, "Task_B", 0);

    cfs->addTask(&task1);
    cfs->addTask(&task2);

    // 应该会按顺序或者基于vruntime策略调度
    uint32_t run1 = cfs->dispatch(100);
    EXPECT_TRUE(run1 == 1 || run1 == 2);
    
    uint32_t run2 = cfs->dispatch(100);
    EXPECT_TRUE(run2 == 1 || run2 == 2);

    EXPECT_NE(run1, run2); // 因为运行了一次之后 vruntime 改变了

    cfs->Release();
}

TEST_F(frame_dmcfs, remove_task)
{
    Idmcfs* cfs = dmcfsGetModule();
    
    MockTask task1(1, "Task_A", 0);
    cfs->addTask(&task1);
    
    // 删除前能调度到
    cfs->removeTask(1);
    
    // 队列应当为空，返回 0
    uint32_t next = cfs->dispatch(100);
    EXPECT_EQ(next, 0);

    cfs->Release();
}

TEST_F(frame_dmcfs, dispatch_throws_safely)
{
    Idmcfs* cfs = dmcfsGetModule();
    
    ThrowingTask faulting_task(100, "FaultingTask", 0);
    cfs->addTask(&faulting_task);
    
    // 应该捕捉异常并防止崩溃
    uint32_t dispatched_id = cfs->dispatch(10);
    
    // 判断还是正确的调出了任务，且不崩溃
    EXPECT_EQ(dispatched_id, 100);

    cfs->Release();
}

TEST_F(frame_dmcfs, mixed_priority_behavior)
{
    Idmcfs* cfs = dmcfsGetModule();
    
    // T1 是高优先级 (-20), T2 是低优先级 (19)
    MockTask t1(1, "High_Prio", -20);
    MockTask t2(2, "Low_Prio", 19);

    cfs->addTask(&t1);
    cfs->addTask(&t2);

    // 高优先级的应该首先调度或者有更慢的 vruntime 增长
    uint32_t id1 = cfs->dispatch(100);
    uint32_t id2 = cfs->dispatch(100);
    
    EXPECT_TRUE(id1 == 1 || id1 == 2);
    EXPECT_TRUE(id2 == 1 || id2 == 2);
    
    cfs->Release();
}

