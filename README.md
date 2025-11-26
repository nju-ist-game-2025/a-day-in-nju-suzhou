# Our Game - 智科er的一天

一个类似《以撒的结合》的2D Roguelike游戏，使用Qt框架开发。

## 🎮 项目简介

这是一个团队协作开发的2D肉鸽游戏

## 🚀 快速开始

### 前置要求

- **CMake** 3.16 或更高版本
- **Qt 6** 或 **Qt 5**（Widgets + Multimedia 模块）
- **C++17** 编译器（MinGW / MSVC / GCC）

### 克隆和编译

```bash
# 克隆仓库
git clone <your-repo-url>
cd our_game

# 创建构建目录
mkdir build
cd build

# 配置项目（自动检测Qt）
cmake ..

# 编译
cmake --build .

# 运行游戏
./game_final.exe  # Windows
./game_final      # Linux/Mac
```

## 📁 项目结构

```
our_game/
├── assets/                  # 🎨 游戏资源文件夹
│   ├── README.md           # 资源文件说明
│   ├── background_main.png # 主菜单背景
│   ├── background_game.png # 游戏背景
│   ├── player.png          # 玩家图片
│   ├── bullet.png          # 子弹图片
│   └── enemy.png           # 敌人图片
├── src/                     # 💻 源代码
│   ├── core/               # 核心系统
│   │   ├── gamewindow.*    # 主窗口
│   │   ├── gamecontroller.*# 游戏控制器
│   │   └── resourcefactory.h # 资源工厂
│   ├── entities/           # 游戏实体
│   │   ├── player.*        # 玩家
│   │   ├── enemy.*         # 敌人
│   │   └── projectile.*    # 子弹
│   ├── ui/                 # UI界面
│   │   ├── mainmenu.*      # 主菜单
│   │   └── gameview.*      # 游戏视图
│   ├── world/              # 世界系统
│   ├── items/              # 道具系统
│   ├── constants.h         # 全局常量
│   └── main.cpp            # 入口
├── build/                   # 🔨 构建输出（不提交）
│   ├── game_final.exe
│   └── assets/             # 自动复制的资源
├── CMakeLists.txt          # CMake配置
├── README.md               # 本文件
└── RESOURCE_GUIDE.md       # 资源添加指南
```

## 🎨 添加美术资源

所有资源文件都**必须**放在 `our_game/assets/` 文件夹中。

### 必需的资源文件：

| 文件名                   | 尺寸      | 说明     |
|-----------------------|---------|--------|
| `background_main.png` | 800×600 | 主菜单背景  |
| `background_game.png` | 800×600 | 游戏场景背景 |
| `player.png`          | 60×60   | 玩家角色   |
| `bullet.png`          | 15×15   | 子弹     |
| `enemy.png`           | 40×40   | 敌人     |

**详细指南**：查看 [RESOURCE_GUIDE.md](RESOURCE_GUIDE.md)

### 音效资源

- `assets/sounds/teleport.wav` - 玩家瞬移时的音效

## 🎯 游戏操作

- **方向键** ↑↓←→ - 移动玩家
- **WASD** - 四方向射击
    - W - 向上射击
    - S - 向下射击
    - A - 向左射击
    - D - 向右射击
- **Q** - 在移动时使用一次瞬移，冷却5秒
- **ESC** - 返回主菜单

## 🧬 角色能力加成

角色选择界面中的四位角色现在拥有截然不同的初始能力，选择不同角色即可体验差异化的开局策略：

| 角色                           | 能力说明                                   |
|------------------------------|----------------------------------------|
| 美少女 (`beautifulGirl.png`)    | 子弹伤害翻倍，适合喜欢爆发输出的玩家。                    |
| 高雅人士 (`HighGracePeople.png`) | 初始心之容器 +2 并额外获得 2 点魂心，更耐打。             |
| 小蓝鲸 (`njuFish.png`)          | 移动速度提升 25%，子弹速度提升 20%，射击冷却 -40ms，灵活敏捷。 |
| 权服侠 (`quanfuxia.png`)        | 出场即携带 2 枚炸弹、2 把钥匙和 1 点黑心，资源更充裕。        |

## 🛠️ 开发说明

### 编译选项

```bash
# Release 构建（优化性能）
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# Debug 构建（调试信息）
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### 添加新资源

1. 将PNG文件放入 `our_game/assets/`
2. 提交到Git：`git add assets/`
3. 队友拉取后重新编译即可

### 修改资源路径

如需更改资源文件名，修改以下文件：

- `src/ui/gameview.cpp` - 游戏场景资源
- `src/ui/mainmenu.cpp` - 主菜单资源

## 📚 文档

- [RESOURCE_GUIDE.md](RESOURCE_GUIDE.md) - 美术资源添加指南
- [UI_GUIDE.md](UI_GUIDE.md) - UI系统使用说明
- [assets/README.md](assets/README.md) - 资源文件详细规范

## 🤝 团队协作

### 协作流程

1. **拉取最新代码**
   ```bash
   git pull
   ```

2. **添加/修改代码或资源**

3. **测试编译**
   ```bash
   cd build
   cmake --build .
   ```

4. **提交更改**
   ```bash
   git add .
   git commit -m "描述你的修改"
   git push
   ```

### 分支管理建议

- `main` - 稳定版本
- `dev` - 开发版本
- `feature/*` - 新功能分支

## ⚠️ 常见问题

### Q: 编译失败提示找不到Qt？

A: 确保Qt已安装并设置环境变量 `CMAKE_PREFIX_PATH`

```bash
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/mingw_64" ..
```

### Q: 游戏启动提示资源加载失败？

A: 检查 `assets/` 文件夹中是否包含所有必需的PNG文件

### Q: 修改了资源但游戏中没变化？

A: 重新编译项目：`cmake --build .`

## 📄 许可证

[添加你的许可证信息]

## 👥 贡献者

- [添加团队成员名单]

## 📞 联系方式

[添加联系方式]