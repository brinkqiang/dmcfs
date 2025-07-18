#include "dmcfs.h"
#include "dmcfs_task.h"
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <random>
#include <iomanip>
#include <algorithm>
#include "dmfix_win_utf8.h"
// --- ANSI 颜色代码 ---
#define RESET   "\033[0m"
#define BOLD_CYAN    "\033[1;36m"

// 1. 玩家输入处理任务
class PlayerInputTask : public Idmcfs_task {
public:
    PlayerInputTask(uint32_t id) : m_id(id) {}
    uint32_t getId() const override { return m_id; }
    const char* getName() const override { return "玩家输入"; }
    int getNiceValue() const override { return -20; }
    DmcfsSchedulingState& getSchedulingState() override { return m_sched_state; }
    DmcfsTuningState& getTuningState() override { return m_tuning_state; }

    uint32_t run(uint32_t requested_count) override {
        std::cout << "  > [执行] " << getName() << ": 正在轮询键鼠... (建议量: " << requested_count << ", 实际: 1)";
        return 1;
    }
private:
    uint32_t m_id;
    DmcfsSchedulingState m_sched_state;
    DmcfsTuningState m_tuning_state;
};

// 2. 物理引擎任务
class PhysicsEngineTask : public Idmcfs_task {
public:
    PhysicsEngineTask(uint32_t id, uint32_t particle_count) : m_id(id), m_total_particles(particle_count) {}
    uint32_t getId() const override { return m_id; }
    const char* getName() const override { return "物理引擎"; }
    int getNiceValue() const override { return -10; } // 降低一点优先级，使其不总是抢在输入前面
    DmcfsSchedulingState& getSchedulingState() override { return m_sched_state; }
    DmcfsTuningState& getTuningState() override { return m_tuning_state; }

    uint32_t run(uint32_t requested_count) override {
        uint32_t particles_to_process = std::min(requested_count, m_total_particles);
        std::cout << "  > [执行] " << getName() << ": 正在模拟 " << particles_to_process << " 个粒子... (建议量: " << requested_count << ")";
        return particles_to_process;
    }
private:
    uint32_t m_id;
    uint32_t m_total_particles;
    DmcfsSchedulingState m_sched_state;
    DmcfsTuningState m_tuning_state;
};

// 3. 资源流式加载任务
class AssetStreamingTask : public Idmcfs_task {
public:
    AssetStreamingTask(uint32_t id, std::string asset_name, uint32_t total_kb)
        : m_id(id), m_asset_name(std::move(asset_name)), m_kb_loaded(0), m_total_kb(total_kb) {
    }
    uint32_t getId() const override { return m_id; }
    const char* getName() const override { return "资源加载器"; }
    int getNiceValue() const override { return 15; }
    DmcfsSchedulingState& getSchedulingState() override { return m_sched_state; }
    DmcfsTuningState& getTuningState() override { return m_tuning_state; }
    bool isDone() const { return m_kb_loaded >= m_total_kb; }

    uint32_t run(uint32_t requested_count) override {
        if (isDone()) return 0;
        uint32_t kb_to_read = std::min(requested_count, (uint32_t)10);
        kb_to_read = std::min(kb_to_read, m_total_kb - m_kb_loaded);
        m_kb_loaded += kb_to_read;
        std::cout << "  > [执行] " << getName() << ": 加载 '" << m_asset_name << "'... " << m_kb_loaded << "/" << m_total_kb
            << " KB. (建议量: " << requested_count << ", 实际: " << kb_to_read << ")";
        return kb_to_read;
    }
private:
    uint32_t m_id;
    std::string m_asset_name;
    uint32_t m_kb_loaded;
    uint32_t m_total_kb;
    DmcfsSchedulingState m_sched_state;
    DmcfsTuningState m_tuning_state;
};


// --- 主游戏循环 ---
void printTaskState(Idmcfs_task* task, uint64_t vruntime_scale) {
    auto& tuning_state = task->getTuningState();
    auto& sched_state = task->getSchedulingState();
    double readable_vruntime = (double)sched_state.vruntime / vruntime_scale;

    std::cout << "    > 任务: " << std::left << std::setw(12) << task->getName()
        << " | 建议工作量: " << std::setw(5) << tuning_state.recommended_count
        << " | 平均完成率: " << std::fixed << std::setprecision(2) << (double)tuning_state.avg_completion_ratio_scaled / 10000.0
        << " | vruntime: " << readable_vruntime
        << std::endl;
}

int main(int argc, char* argv[]) {
    dmcfsPtr scheduler(dmcfsGetModule());
    if (!scheduler) { return 1; }

    // 为了演示效果，把所有任务都预先创建好
    auto player_input = std::make_unique<PlayerInputTask>(101);
    auto physics_engine = std::make_unique<PhysicsEngineTask>(201, 5000);
    auto asset_task = std::make_unique<AssetStreamingTask>(901, "地牢贴图.pak", 85);

    std::vector<Idmcfs_task*> all_tasks;
    all_tasks.push_back(player_input.get());
    all_tasks.push_back(physics_engine.get());
    all_tasks.push_back(asset_task.get());

    for (const auto& task : all_tasks) {
        scheduler->addTask(task);
    }

    const int TOTAL_FRAMES = 30;
    const int DISPATCHES_PER_FRAME = 4;

    std::cout << "\n" << BOLD_CYAN << "==================== 游戏开始 (修正版) ====================" << RESET << "\n";
    for (int frame = 1; frame <= TOTAL_FRAMES; ++frame) {
        std::cout << "\n" << BOLD_CYAN << "--- 游戏帧 #" << frame << " ---" << RESET << std::endl;

        for (int d = 0; d < DISPATCHES_PER_FRAME; ++d) {
            uint32_t dispatched_id = scheduler->dispatch(2); // 基础时间片为2ms
            if (dispatched_id == 0) break;
        }

        if (asset_task && asset_task->isDone()) {
            std::cout << "引擎: 资源 '地牢贴图.pak' 加载完毕，从调度器移除。" << std::endl;
            scheduler->removeTask(asset_task->getId());
            // 将其从我们的报告列表中移除
            all_tasks.erase(std::remove(all_tasks.begin(), all_tasks.end(), asset_task.get()), all_tasks.end());
            asset_task.reset();
        }
    }
    std::cout << "\n" << BOLD_CYAN << "==================== 游戏结束 ====================" << RESET << "\n";

    std::cout << "\n--- 最终自适应状态报告 ---" << std::endl;
    for (const auto& task : all_tasks) {
        printTaskState(task, 1024); // 使用在impl中定义的scale
    }

    return 0;
}