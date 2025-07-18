
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

#include "libdmcfs_impl.h"
#include "dmcfs_task.h"
#include <iostream>
#include <algorithm>
#include <utility>

DmcfsImpl::DmcfsImpl() : m_min_vruntime(0) {}

DmcfsImpl::~DmcfsImpl() {}

void DmcfsImpl::Release(void) {
    delete this;
}

uint32_t DmcfsImpl::get_weight(int nice_value) {
    // 将 nice 值从 [-20, 19] 映射到数组索引 [0, 39]
    int index = std::clamp(nice_value + 20, 0, 39);
    return sched_prio_to_weight[index];
}

void DmcfsImpl::addTask(Idmcfs_task* task) {
    if (!task || m_task_lookup.count(task->getId())) {
        return;
    }

    int nice_value = task->getNiceValue();
    uint32_t weight = get_weight(nice_value);

    DmcfsSchedulingState& state = task->getSchedulingState();
    state.weight = weight;
    state.vruntime = m_min_vruntime; // 新任务设置为当前最小vruntime

    uint32_t task_id = task->getId();
    VRuntimeKey key = {state.vruntime, task_id};

    m_run_queue[key] = task;
    m_task_lookup[task_id] = task;

    std::cout << "Scheduler: Task '" << task->getName() << "' (ID: " << task_id
        << ", nice: " << nice_value << ") added." << std::endl;
}

void DmcfsImpl::removeTask(uint32_t task_id) {
    auto it_lookup = m_task_lookup.find(task_id);
    if (it_lookup != m_task_lookup.end()) {
        Idmcfs_task* task = it_lookup->second;
        DmcfsSchedulingState& state = task->getSchedulingState();

        VRuntimeKey key = {state.vruntime, task->getId()};

        m_run_queue.erase(key);
        m_task_lookup.erase(it_lookup);

        std::cout << "Scheduler: Task '" << task->getName() << "' (ID: " << task_id << ") removed." << std::endl;

        if (!m_run_queue.empty()) {
            m_min_vruntime = m_run_queue.begin()->second->getSchedulingState().vruntime;
        }
    }
}

uint32_t DmcfsImpl::dispatch(uint64_t exec_slice_ms) {
    if (m_run_queue.empty()) {
        return 0; // 返回0表示没有任务被调度
    }

    // 1. 选择任务 (Pick)
    auto it = m_run_queue.begin();
    Idmcfs_task* task_to_run = it->second;

    // 从队列中移除旧条目，因为vruntime即将改变
    m_run_queue.erase(it);

    // 2. 执行任务 (Run)
    std::cout << "\nScheduler: Dispatching task '" << task_to_run->getName()
        << "' (vruntime: " << task_to_run->getSchedulingState().vruntime << ")." << std::endl;

    task_to_run->run();

    // 3. 更新状态 (Update)
    DmcfsSchedulingState& state = task_to_run->getSchedulingState();
    uint64_t delta_vruntime = (exec_slice_ms * NICE_0_LOAD) / state.weight;
    state.vruntime += delta_vruntime;

    // 4. 重新插入队列
    VRuntimeKey new_key = {state.vruntime, task_to_run->getId()};
    m_run_queue[new_key] = task_to_run;

    // 更新整个队列的最小 vruntime
    m_min_vruntime = m_run_queue.begin()->second->getSchedulingState().vruntime;

    return task_to_run->getId();
}

// 工厂函数实现
extern "C" DMEXPORT_DLL Idmcfs* DMAPI dmcfsGetModule() {
    return new DmcfsImpl();
}