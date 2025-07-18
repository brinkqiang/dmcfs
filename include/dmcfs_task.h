
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

#ifndef __DMCFS_TASK_H_INCLUDE__
#define __DMCFS_TASK_H_INCLUDE__

#include <cstdint>

struct DmcfsSchedulingState {
    uint64_t vruntime = 0;
    uint32_t weight = 0;
};

// 任务接口类
class Idmcfs_task {
public:
    virtual ~Idmcfs_task() {}

    //--- 任务的固有属性 (由任务实现者提供) ---//

    virtual uint32_t getId() const = 0;
    virtual const char* getName() const = 0;
    virtual int getNiceValue() const = 0;

    //--- 任务的执行体 (由任务实现者定义核心逻辑) ---//

    virtual void run() = 0;

    //--- 调度状态 (由调度器管理) ---//

    virtual DmcfsSchedulingState& getSchedulingState() = 0;
};

#endif // __DMCFS_TASK_H_INCLUDE__