
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
#ifndef __DMCFS_H_INCLUDE__
#define __DMCFS_H_INCLUDE__

#include "dmos.h"
#include "dmmoduleptr.h"
#include <cstdint>

class Idmcfs_task;

// 调度器接口类
class Idmcfs; 
using dmcfsPtr = DmModulePtr<Idmcfs>;

class Idmcfs {
public:
    virtual ~Idmcfs() {}
    virtual void DMAPI Release(void) = 0;

    /**
     * @brief 向调度器添加一个任务。调度器不拥有该任务指针的所有权。
     * @param task 指向实现了 Idmcfs_task 接口的对象的指针。
     */
    virtual void DMAPI addTask(Idmcfs_task* task) = 0;
    
    /**
     * @brief 移除一个任务。
     * @param task_id 要移除的任务的ID。
     */
    virtual void DMAPI removeTask(uint32_t task_id) = 0;

    /**
     * @brief 执行一个调度周期。
     * 它会选择vruntime最小的任务，调用其run()方法，
     * 然后根据给定的执行时间更新该任务的vruntime。
     * @param exec_slice_ms 分配给选定任务的执行时间（毫秒）。
     * @return 返回被执行的任务的ID，如果没有任务可执行则返回0。
     */
    virtual uint32_t DMAPI dispatch(uint64_t exec_slice_ms) = 0;
};

// 工厂函数返回的是接口指针 Idmcfs*
extern "C" DMEXPORT_DLL Idmcfs* DMAPI dmcfsGetModule();
typedef Idmcfs* (DMAPI* PFN_dmcfsGetModule)();

#endif // __DMCFS_H_INCLUDE__