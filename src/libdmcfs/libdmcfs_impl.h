
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

#include "dmcfs.h"
#include <map>
#include <unordered_map>
#include <vector>

// 真实Linux内核中的权重表
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

// NICE_0_LOAD
static const uint32_t NICE_0_LOAD = 1024;

class DmcfsImpl : public Idmcfs {
public:
    DmcfsImpl();
    virtual ~DmcfsImpl() override;

    // 实现 Idmcfs 接口
    virtual void DMAPI Release(void) override;
    virtual void DMAPI addTask(Idmcfs_task* task) override;
    virtual void DMAPI removeTask(uint32_t task_id) override;
    virtual uint32_t DMAPI dispatch(uint64_t exec_slice_ms) override;

private:
    uint32_t get_weight(int nice_value);

    // 使用 vruntime 和 task_id 作为复合键来保证唯一性
    using VRuntimeKey = std::pair<uint64_t, uint32_t>;
    // 运行队列红黑树，存储指向任务的指针
    std::map<VRuntimeKey, Idmcfs_task*> m_run_queue;
    // 用于通过ID快速查找任务指针
    std::unordered_map<uint32_t, Idmcfs_task*> m_task_lookup;

    uint64_t m_min_vruntime;
};

#endif // __LIBDMCFS_IMPL_H_INCLUDE__