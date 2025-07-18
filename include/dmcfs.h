
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
#include <string>
#include <cstdint>
#include <optional>

class Idmcfs; 
using dmcfsPtr = DmModulePtr<Idmcfs>;

// 任务结构体保持不变，因为它描述的是数据，而不是模块本身
struct CfsTask {
    uint32_t id;
    std::string name;
    int nice_value;
    uint64_t vruntime;
    uint32_t weight;
};

class Idmcfs {
public:
    virtual ~Idmcfs() {}
    virtual void DMAPI Release(void) = 0;

    virtual void DMAPI AddTask(uint32_t id, const std::string& name, int nice_value) = 0;
    virtual std::optional<CfsTask> DMAPI PickNextTask() const = 0;
    virtual bool DMAPI UpdateTaskRuntime(uint32_t task_id, uint64_t exec_time) = 0;
    virtual void DMAPI RemoveTask(uint32_t task_id) = 0;
};

// 工厂函数也遵循您的风格
extern "C" DMEXPORT_DLL Idmcfs* DMAPI dmcfsGetModule();
typedef Idmcfs* (DMAPI* PFN_dmcfsGetModule)();

#endif // __DMCFS_H_INCLUDE__