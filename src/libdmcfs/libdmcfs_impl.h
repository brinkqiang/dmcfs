
// Copyright (c) 2018 brinkqiang (brink.qiang@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __LIBDMCFS_IMPL_H_INCLUDE__
#define __LIBDMCFS_IMPL_H_INCLUDE__

#include "dmcfs.h" // 包含新的头文件
#include <map>
#include <unordered_map>
#include <vector>

// 权重表和常量保持不变
static const int sched_prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     45462,     36423,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /* -5 */      3121,      2501,      1991,      1586,      1277,
 /* 0 */      1024,       820,       655,       526,       423,
 /* 5 */       335,       272,       215,       172,       137,
 /* 10 */       110,        87,        70,        56,        45,
 /* 15 */        36,        29,        23,        18,        15,
};
static const uint32_t NICE_0_LOAD = 1024;

// 实现类的命名也相应修改
class DmcfsImpl : public Idmcfs {
public:
    DmcfsImpl();
    virtual ~DmcfsImpl() override;

    virtual void DMAPI Release(void) override;
    virtual void DMAPI AddTask(uint32_t id, const std::string& name, int nice_value) override;
    virtual std::optional<CfsTask> DMAPI PickNextTask() const override;
    virtual bool DMAPI UpdateTaskRuntime(uint32_t task_id, uint64_t exec_time) override;
    virtual void DMAPI RemoveTask(uint32_t task_id) override;

private:
    uint32_t get_weight(int nice_value);
    
    using VRuntimeKey = std::pair<uint64_t, uint32_t>;
    std::map<VRuntimeKey, CfsTask> m_run_queue;
    std::unordered_map<uint32_t, VRuntimeKey> m_task_lookup;
    uint64_t m_min_vruntime;
};

#endif // __LIBDMCFS_IMPL_H_INCLUDE__