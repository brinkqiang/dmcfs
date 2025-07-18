# dmcfs

Copyright (c) 2013-2018 brinkqiang (brink.qiang@gmail.com)

[![dmcfs](https://img.shields.io/badge/brinkqiang-dmcfs-blue.svg?style=flat-square)](https://github.com/brinkqiang/dmcfs)
[![License](https://img.shields.io/badge/license-MIT-brightgreen.svg)](https://github.com/brinkqiang/dmcfs/blob/master/LICENSE)
[![blog](https://img.shields.io/badge/Author-Blog-7AD6FD.svg)](https://brinkqiang.github.io/)
[![Open Source Love](https://badges.frapsoft.com/os/v3/open-source.png)](https://github.com/brinkqiang)
[![GitHub stars](https://img.shields.io/github/stars/brinkqiang/dmcfs.svg?label=Stars)](https://github.com/brinkqiang/dmcfs) 
[![GitHub forks](https://img.shields.io/github/forks/brinkqiang/dmcfs.svg?label=Fork)](https://github.com/brinkqiang/dmcfs)

## Build status
| [Linux][lin-link] | [Mac][mac-link] | [Windows][win-link] |
| :---------------: | :----------------: | :-----------------: |
| ![lin-badge]      | ![mac-badge]       | ![win-badge]        |

[lin-badge]: https://github.com/brinkqiang/dmcfs/workflows/linux/badge.svg "linux build status"
[lin-link]:  https://github.com/brinkqiang/dmcfs/actions/workflows/linux.yml "linux build status"
[mac-badge]: https://github.com/brinkqiang/dmcfs/workflows/mac/badge.svg "mac build status"
[mac-link]:  https://github.com/brinkqiang/dmcfs/actions/workflows/mac.yml "mac build status"
[win-badge]: https://github.com/brinkqiang/dmcfs/workflows/win/badge.svg "win build status"
[win-link]:  https://github.com/brinkqiang/dmcfs/actions/workflows/win.yml "win build status"

## Intro
dmcfs
```cpp
#include "dmcfs.h"
#include "dmcfs_task.h"
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <random>
#include "dmfix_win_utf8.h"
// --- 游戏相关的具体任务实现 ---

// 1. 玩家输入处理任务 (最高优先级)
class PlayerInputTask : public Idmcfs_task {
public:
    PlayerInputTask(uint32_t id) : m_id(id) {}
    uint32_t getId() const override { return m_id; }
    const char* getName() const override { return "PlayerInput"; }
    int getNiceValue() const override { return -20; } // 最高优先级
    DmcfsSchedulingState& getSchedulingState() override { return m_state; }

    void run() override {
        std::cout << "  └─ [Input] Polling keyboard and mouse state..." << std::endl;
    }
private:
    uint32_t m_id;
    DmcfsSchedulingState m_state;
};

// 2. 物理引擎任务 (高优先级)
class PhysicsEngineTask : public Idmcfs_task {
public:
    PhysicsEngineTask(uint32_t id) : m_id(id), m_step_count(0) {}
    uint32_t getId() const override { return m_id; }
    const char* getName() const override { return "PhysicsEngine"; }
    int getNiceValue() const override { return -15; } // 高优先级
    DmcfsSchedulingState& getSchedulingState() override { return m_state; }

    void run() override {
        m_step_count++;
        std::cout << "  └─ [Physics] Simulating step #" << m_step_count << ". Updating transforms and detecting collisions." << std::endl;
    }
private:
    uint32_t m_id;
    uint64_t m_step_count;
    DmcfsSchedulingState m_state;
};

// 3. AI行为任务 (中等优先级)
class AiBehaviorTask : public Idmcfs_task {
public:
    enum class State { IDLE, PATROLLING, ATTACKING };
    AiBehaviorTask(uint32_t id, std::string name) : m_id(id), m_name(std::move(name)), m_current_state(State::IDLE) {}
    uint32_t getId() const override { return m_id; }
    const char* getName() const override { return m_name.c_str(); }
    int getNiceValue() const override { return 5; } // 中等优先级
    DmcfsSchedulingState& getSchedulingState() override { return m_state; }

    void run() override {
        // 模拟简单的状态切换
        int random_event = std::rand() % 10;
        if (random_event > 8) m_current_state = State::ATTACKING;
        else if (random_event > 5) m_current_state = State::PATROLLING;
        else m_current_state = State::IDLE;

        std::cout << "  └─ [AI:" << m_name << "] Current state: ";
        switch (m_current_state) {
        case State::IDLE: std::cout << "Idling." << std::endl; break;
        case State::PATROLLING: std::cout << "Patrolling area." << std::endl; break;
        case State::ATTACKING: std::cout << "Attacking player!" << std::endl; break;
        }
    }
private:
    uint32_t m_id;
    std::string m_name;
    State m_current_state;
    DmcfsSchedulingState m_state;
};


// 4. 资源流式加载任务 (最低优先级)
class AssetStreamingTask : public Idmcfs_task {
public:
    AssetStreamingTask(uint32_t id, std::string asset_name)
        : m_id(id), m_asset_name(std::move(asset_name)), m_progress(0), m_total_size(100) {
    }

    uint32_t getId() const override { return m_id; }
    const char* getName() const override { return "AssetStreamer"; }
    int getNiceValue() const override { return 15; } // 最低优先级
    DmcfsSchedulingState& getSchedulingState() override { return m_state; }
    bool isDone() const { return m_progress >= m_total_size; }

    void run() override {
        if (isDone()) return;
        m_progress += 25; // 模拟每次加载25%
        std::cout << "  └─ [Assets] Loading '" << m_asset_name << "'... " << m_progress << "%" << std::endl;
    }
private:
    uint32_t m_id;
    std::string m_asset_name;
    int m_progress;
    int m_total_size;
    DmcfsSchedulingState m_state;
};


// --- 主游戏循环 ---
int main(int argc, char* argv[]) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    dmcfsPtr scheduler(dmcfsGetModule());
    if (!scheduler) {
        std::cerr << "Error: Failed to get dmcfs module." << std::endl;
        return 1;
    }

    // 创建常驻的核心游戏任务
    auto player_input = std::make_unique<PlayerInputTask>(101);
    auto physics_engine = std::make_unique<PhysicsEngineTask>(201);
    auto orc_ai = std::make_unique<AiBehaviorTask>(301, "Orc Grunt");
    auto dragon_ai = std::make_unique<AiBehaviorTask>(302, "Ancient Dragon");

    scheduler->addTask(player_input.get());
    scheduler->addTask(physics_engine.get());
    scheduler->addTask(orc_ai.get());
    scheduler->addTask(dragon_ai.get());

    std::unique_ptr<AssetStreamingTask> asset_task = nullptr;

    const int TOTAL_FRAMES = 15;
    const int DISPATCHES_PER_FRAME = 5; // 在一帧的时间内，调度器可以切换5次任务

    // 模拟游戏主循环
    std::cout << "\n==================== GAME START ====================\n";
    for (int frame = 1; frame <= TOTAL_FRAMES; ++frame) {
        std::cout << "\n--- Frame #" << frame << " ---" << std::endl;

        // 模拟事件：在第3帧，玩家进入新区域，需要加载资源
        if (frame == 3 && !asset_task) {
            std::cout << "Engine: Player entered 'Dark Forest'. Triggering asset streaming..." << std::endl;
            asset_task = std::make_unique<AssetStreamingTask>(901, "dark_forest.pak");
            scheduler->addTask(asset_task.get());
        }

        // 在一帧的时间内，让CFS调度多个任务
        for (int d = 0; d < DISPATCHES_PER_FRAME; ++d) {
            // 每次调度，分配2ms的执行时间片
            scheduler->dispatch(2);
        }

        // 检查后台任务是否完成
        if (asset_task && asset_task->isDone()) {
            std::cout << "Engine: Asset 'dark_forest.pak' finished loading." << std::endl;
            scheduler->removeTask(asset_task->getId());
            asset_task.reset(); // 销毁任务对象
        }
    }
    std::cout << "\n==================== GAME END ====================\n";

    return 0;
}
```
## Contacts

## Thanks
