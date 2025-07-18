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
#include <iomanip>
#include <stdexcept> // 用于异常处理

// --- ANSI 颜色代码 ---
#define RESET   "\033[0m"
#define BOLD_CYAN    "\033[1;36m"
#define MAGENTA "\033[0;35m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define RED     "\033[0;31m"

// --- 定点数运算常量 ---
const uint32_t RATIO_BASE = 10000;
const uint32_t EMA_ALPHA_SCALED = 1000;
const uint32_t CPU_BOUND_GROWTH_RATE_SCALED = 10200;
const uint32_t MINIMUM_RECOMMENDED_COUNT = 10;
// vruntime 缩放因子，解决整数除法精度问题
const uint64_t VRUNTIME_SCALE = 1024;

// --- **【修改】** 调优策略常量 ---
const int TUNE_INTERVAL = 5; // 每调度5次，进行一次调优
const uint32_t DECREASE_THRESHOLD_SCALED = 9000; // 低于90%则减少
const uint32_t INCREASE_THRESHOLD_SCALED = 9800; // 高于98%则增加


DmcfsImpl::DmcfsImpl() : m_min_vruntime(0) {}

DmcfsImpl::~DmcfsImpl() {}

void DmcfsImpl::Release(void) {
    delete this;
}

uint32_t DmcfsImpl::get_weight(int nice_value) {
    int index = std::clamp(nice_value + 20, 0, 39);
    return sched_prio_to_weight[index];
}

void DmcfsImpl::addTask(Idmcfs_task* task) {
    if (!task || m_task_lookup.count(task->getId())) {
        return;
    }
    int nice_value = task->getNiceValue();
    uint32_t weight = get_weight(nice_value);

    DmcfsSchedulingState& sched_state = task->getSchedulingState();
    sched_state.weight = weight;
    sched_state.vruntime = m_min_vruntime;

    uint32_t task_id = task->getId();
    VRuntimeKey key = {sched_state.vruntime, task_id};

    m_run_queue[key] = task;
    m_task_lookup[task_id] = task;
    m_dispatch_counts[task_id] = 0; // **【修改】** 初始化计数器
}

void DmcfsImpl::removeTask(uint32_t task_id) {
    // **【修改】** 全面修正此函数逻辑
    auto it_lookup = m_task_lookup.find(task_id);
    if (it_lookup != m_task_lookup.end()) {
        Idmcfs_task* task = it_lookup->second;
        DmcfsSchedulingState& state = task->getSchedulingState();
        VRuntimeKey key = {state.vruntime, task->getId()};

        bool is_min_task = false;
        if (!m_run_queue.empty() && m_run_queue.begin()->second->getId() == task_id) {
            is_min_task = true;
        }

        m_run_queue.erase(key);
        m_task_lookup.erase(it_lookup);
        m_dispatch_counts.erase(task_id); // 移除计数器

        if (m_run_queue.empty()) {
            m_min_vruntime = 0; // 队列为空，vruntime归零
        }
        else if (is_min_task) {
            // 被移除的是最小任务，更新min_vruntime
            m_min_vruntime = m_run_queue.begin()->second->getSchedulingState().vruntime;
        }
    }
}

uint32_t DmcfsImpl::dispatch(uint64_t base_slice_ms) {
    if (m_run_queue.empty()) {
        return 0;
    }

    auto it = m_run_queue.begin();
    Idmcfs_task* task_to_run = it->second;
    uint32_t task_id = task_to_run->getId();

    // 为异常安全，先保存原始状态
    DmcfsSchedulingState original_sched_state = task_to_run->getSchedulingState();
    m_run_queue.erase(it);

    try {
        DmcfsTuningState& tuning_state = task_to_run->getTuningState();
        uint32_t requested_count = tuning_state.recommended_count;

        double readable_vruntime = (double)original_sched_state.vruntime / VRUNTIME_SCALE;
        std::cout << "调度器: 选择任务 '" << task_to_run->getName() << "' (vruntime: "
            << std::fixed << std::setprecision(2) << readable_vruntime << ")" << std::endl;

        // **【修改】** 调用新的run方法，并获取实际消耗时间
        std::cout << MAGENTA;
        TaskRunResult result = task_to_run->run(requested_count);
        std::cout << RESET;

        // 每次都更新EMA，用于平滑数据
        if (requested_count > 0) {
            uint64_t current_ratio_scaled = ((uint64_t)result.actual_count * RATIO_BASE) / requested_count;
            uint64_t old_avg_scaled = tuning_state.avg_completion_ratio_scaled;
            uint64_t new_avg_scaled = (((uint64_t)EMA_ALPHA_SCALED * current_ratio_scaled) +
                ((uint64_t)(RATIO_BASE - EMA_ALPHA_SCALED) * old_avg_scaled)) / RATIO_BASE;
            tuning_state.avg_completion_ratio_scaled = (uint32_t)new_avg_scaled;
        }

        // **【修改】** 增加调优频率和稳定区间判断
        m_dispatch_counts[task_id]++;
        if (m_dispatch_counts[task_id] >= TUNE_INTERVAL) {
            m_dispatch_counts[task_id] = 0; // 重置计数器

            uint32_t old_recommended = tuning_state.recommended_count;
            uint32_t new_recommended = old_recommended;

            if (tuning_state.avg_completion_ratio_scaled < DECREASE_THRESHOLD_SCALED && old_recommended > MINIMUM_RECOMMENDED_COUNT) {
                uint64_t adjusted_count = ((uint64_t)old_recommended * tuning_state.avg_completion_ratio_scaled) / RATIO_BASE;
                new_recommended = std::max(MINIMUM_RECOMMENDED_COUNT, (uint32_t)adjusted_count);
                if (new_recommended != old_recommended) {
                    std::cout << YELLOW << "  └─ [调优] 任务 '" << task_to_run->getName() << "' 完成率低, 优化建议量: "
                        << old_recommended << " -> " << new_recommended << RESET << std::endl;
                }
            }
            else if (tuning_state.avg_completion_ratio_scaled > INCREASE_THRESHOLD_SCALED) {
                uint64_t increased_count = ((uint64_t)old_recommended * CPU_BOUND_GROWTH_RATE_SCALED) / RATIO_BASE;
                new_recommended = std::max(old_recommended, (uint32_t)increased_count);
                if (new_recommended != old_recommended) {
                    std::cout << GREEN << "  └─ [调优] 任务 '" << task_to_run->getName() << "' 持续繁忙, 试探性增加建议量: "
                        << old_recommended << " -> " << new_recommended << RESET << std::endl;
                }
            }
            tuning_state.recommended_count = new_recommended;
        }

        // **【修改】** 使用实际消耗的时间来计算vruntime
        DmcfsSchedulingState& sched_state = task_to_run->getSchedulingState();
        uint64_t delta_vruntime_scaled = (result.consumed_ms * NICE_0_LOAD * VRUNTIME_SCALE) / sched_state.weight;
        sched_state.vruntime += delta_vruntime_scaled;

        VRuntimeKey new_key = {sched_state.vruntime, task_id};
        m_run_queue[new_key] = task_to_run;

    }
    catch (const std::exception& e) {
        // **【修改】** 增加异常安全处理
        std::cerr << RED << "错误: 任务 '" << task_to_run->getName() << "' 执行时抛出异常: " << e.what() << RESET << std::endl;
        VRuntimeKey original_key = {original_sched_state.vruntime, task_id};
        m_run_queue[original_key] = task_to_run;
        std::cerr << YELLOW << "调度器: 任务 '" << task_to_run->getName() << "' 已被安全放回队列，不会丢失。" << RESET << std::endl;
    }

    if (!m_run_queue.empty()) {
        m_min_vruntime = m_run_queue.begin()->second->getSchedulingState().vruntime;
    }
    else {
        m_min_vruntime = 0; // 如果异常导致最后一个任务被移除，也应重置
    }

    return task_id;
}

extern "C" DMEXPORT_DLL Idmcfs* DMAPI dmcfsGetModule() {
    return new DmcfsImpl();
}