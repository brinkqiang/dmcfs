#include "dmcfs.h" // 包含新的头文件
#include <iostream>
#include <iomanip>
#include "dmfix_win_utf8.h"
void PrintSeparator() {
    std::cout << "---------------------------------------------------------" << std::endl;
}

int main() {
    // 使用新的智能指针和工厂函数
    dmcfsPtr scheduler(dmcfsGetModule());
    if (!scheduler) {
        std::cerr << "Failed to get dmcfs module." << std::endl;
        return 1;
    }

    scheduler->AddTask(1, "WebServer", -10);
    scheduler->AddTask(2, "Calculator", 0);
    scheduler->AddTask(3, "VideoEncoder", 10);
    PrintSeparator();
    
    for (int i = 0; i < 10; ++i) {
        std::cout << "Scheduling cycle " << i + 1 << ":" << std::endl;
        
        auto next_task_opt = scheduler->PickNextTask();
        if (!next_task_opt) {
            std::cout << "No tasks to run." << std::endl;
            break;
        }
        
        CfsTask task_to_run = *next_task_opt;
        std::cout << "Picked: '" << task_to_run.name << "' (vruntime: " << task_to_run.vruntime << ")" << std::endl;
        
        scheduler->UpdateTaskRuntime(task_to_run.id, 10);
        
        PrintSeparator();
    }
    
    // ... 其他测试代码
    
    return 0;
}