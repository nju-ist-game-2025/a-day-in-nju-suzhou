# 智科er的一天

一个类似《以撒的结合》的2D Roguelike游戏，使用Qt框架开发。

## 项目简介

这是一个团队协作开发的2D肉鸽游戏
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
