
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

#include "libdmcfs_impl.h" // 包含对应的实现头文件
#include <iostream>
#include <algorithm>

// 实现的类名和方法需要全量更新
DmcfsImpl::DmcfsImpl() : m_min_vruntime(0) {}

DmcfsImpl::~DmcfsImpl() {
    std::cout << "CFS implementation (DmcfsImpl) destroyed." << std::endl;
}

void DmcfsImpl::Release(void) {
    delete this;
}

uint32_t DmcfsImpl::get_weight(int nice_value) {
    int index = std::clamp(nice_value + 20, 0, 39);
    return sched_prio_to_weight[index];
}

void DmcfsImpl::AddTask(uint32_t id, const std::string& name, int nice_value) {
    if (m_task_lookup.count(id)) {
        return;
    }
    CfsTask task;
    task.id = id;
    task.name = name;
    task.nice_value = nice_value;
    task.weight = get_weight(nice_value);
    task.vruntime = m_min_vruntime;
    VRuntimeKey key = {task.vruntime, task.id};
    m_run_queue[key] = task;
    m_task_lookup[id] = key;
}

std::optional<CfsTask> DmcfsImpl::PickNextTask() const {
    if (m_run_queue.empty()) {
        return std::nullopt;
    }
    return m_run_queue.begin()->second;
}

bool DmcfsImpl::UpdateTaskRuntime(uint32_t task_id, uint64_t exec_time) {
    auto it = m_task_lookup.find(task_id);
    if (it == m_task_lookup.end()) {
        return false;
    }
    VRuntimeKey old_key = it->second;
    CfsTask task = m_run_queue.at(old_key);
    m_run_queue.erase(old_key);

    uint64_t delta_vruntime = (exec_time * NICE_0_LOAD) / task.weight;
    task.vruntime += delta_vruntime;

    VRuntimeKey new_key = {task.vruntime, task.id};
    m_run_queue[new_key] = task;
    m_task_lookup[task_id] = new_key;

    if (!m_run_queue.empty()) {
        m_min_vruntime = m_run_queue.begin()->first.first;
    }
    return true;
}

void DmcfsImpl::RemoveTask(uint32_t task_id) {
    auto it = m_task_lookup.find(task_id);
    if (it != m_task_lookup.end()) {
        VRuntimeKey key = it->second;
        m_run_queue.erase(key);
        m_task_lookup.erase(it);
        if (!m_run_queue.empty()) {
            m_min_vruntime = m_run_queue.begin()->first.first;
        }
    }
}

// 工厂函数实现
extern "C" DMEXPORT_DLL Idmcfs* DMAPI dmcfsGetModule() {
    return new DmcfsImpl();
}