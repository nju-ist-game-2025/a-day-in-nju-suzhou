# 智科er的一天

一个类似《以撒的结合》的2D Roguelike游戏，使用Qt框架开发。

## 项目简介

这是一个团队协作开发的2D肉鸽游戏。

游戏视频：【【NJU智科程设大作业】到底是谁在公用洗衣机洗袜子？现在好了，它们全变异了。】https://www.bilibili.com/video/BV1GZSFB7EX9?vd_source=5a955946411c93bc66870830e2728fe0

### 前置要求

- **CMake** 3.16 或更高版本
- **Qt 6**（Widgets + Multimedia 模块）
- **C++17** 编译器（MinGW / MSVC / GCC）

### 克隆和编译

```bash
# 克隆仓库
git clone https://github.com/nju-ist-game-2025/a-day-in-nju-suzhou.git
cd our_game

# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 编译
cmake --build .

# 运行游戏
./game_final.exe  # Windows
./game_final      # Linux/Mac
```
### 下载发行版
我们提供了游戏的压缩包。可在Releases中下载zip文件，解压后找到game_final.exe，双击即可游玩(目前仅在Windows系统上测试，不一定支持Linux/Mac)

---

## 代码架构

### 目录结构

```
src/
├── main.cpp                    # 程序入口
├── constants.h                 # 全局常量定义
│
├── core/                       # 核心模块
│   ├── gamewindow.cpp/h        # 主窗口，管理界面切换
│   ├── audiomanager.cpp/h      # 音频管理器
│   ├── configmanager.cpp/h     # 配置管理器
│   ├── configvalidator.cpp/h   # 配置验证器
│   ├── logging.cpp/h           # 日志系统
│   └── resourcefactory.h       # 资源工厂
│
├── entities/                   # 游戏实体
│   ├── entity.cpp/h            # 实体基类
│   ├── player.cpp/h            # 玩家
│   ├── enemy.cpp/h             # 敌人基类
│   ├── boss.cpp/h              # Boss基类
│   ├── projectile.cpp/h        # 子弹
│   ├── usagi.cpp/h             # 乌萨奇
│   │
│   ├── level_1/                # 第一关
│   │   ├── clockenemy.cpp/h    # 闹钟怪
│   │   ├── clockboom.cpp/h     # 爆炸闹钟
│   │   ├── pillowenemy.cpp/h   # 枕头怪
│   │   └── nightmareboss.cpp/h # 梦魇
│   │
│   ├── level_2/                # 第二关
│   │   ├── sockenemy.cpp/h     # 臭袜子
│   │   ├── sockshooter.cpp/h   # 射击袜子
│   │   ├── orbitingsock.cpp/h  # 旋转袜子
│   │   ├── pantsenemy.cpp/h    # 内裤怪
│   │   ├── walker.cpp/h        # 毒行者
│   │   ├── toxicgas.cpp/h      # 毒气弹
│   │   └── washmachineboss.cpp/h # 洗衣机
│   │
│   └── level_3/                # 第三关
│       ├── scalingenemy.cpp/h      # 缩放敌人基类
│       ├── optimizationenemy.cpp/h # 凸优化
│       ├── digitalsystemenemy.cpp/h# 数字系统
│       ├── yanglinenemy.cpp/h      # yanglin
│       ├── zhuhaoenemy.cpp/h       # zhuhao
│       ├── xukeenemy.cpp/h         # 沙漠狙神
│       ├── probabilityenemy.cpp/h  # 概率论
│       ├── invigilator.cpp/h       # 监考员
│       ├── chalkbeam.cpp/h         # 粉笔激光（Boss技能）
│       ├── exampaper.cpp/h         # 试卷陷阱（Boss技能）
│       ├── mletrap.cpp/h           # MLE陷阱（Boss技能）
│       └── teacherboss.cpp/h       # 奶牛张boos
│
├── world/                      # 世界/关卡系统
│   ├── level.cpp/h             # 关卡管理
│   ├── room.cpp/h              # 房间状态
│   ├── door.cpp/h              # 门
│   ├── roommanager.cpp/h       # 房间管理器
│   ├── levelconfig.cpp/h       # 关卡配置加载
│   ├── bossfight.cpp/h         # Boss战斗管理
│   ├── rewardsystem.cpp/h      # 奖励系统
│   └── factory/                # 工厂模式
│       ├── enemyfactory.cpp/h  # 敌人工厂（根据类型创建敌人）
│       └── bossfactory.cpp/h   # Boss工厂（根据关卡创建Boss）
│
├── items/                      # 道具系统
│   ├── item.cpp/h              # 道具基类
│   ├── chest.cpp/h             # 宝箱（开启动画、道具生成）
│   ├── droppeditem.cpp/h       # 掉落物
│   ├── droppeditemfactory.cpp/h# 掉落物工厂
│   ├── itemeffectconfig.cpp/h  # 道具效果配置
│   └── statuseffect.cpp/h      # 状态效果
│
└── ui/                         # 用户界面
    ├── gameview.cpp/h          # 游戏主视图
    ├── mainmenu.cpp/h          # 主菜单
    ├── hud.cpp/h               # HUD（血条、钥匙、小地图、技能CD）
    ├── dialogsystem.cpp/h      # 对话系统（剧情、Boss台词）
    ├── pausemenu.cpp/h         # 暂停菜单
    ├── codex.cpp/h             # 图鉴系统
    ├── characterselector.cpp/h # 角色选择界面
    └── explosion.cpp/h         # 爆炸动画效果
```