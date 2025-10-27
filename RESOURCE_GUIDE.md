# 如何添加美工资源 - 快速指南

## 🎯 快速开始（3步完成）

### 第1步：准备图片文件

准备以下PNG图片（可选，缺少的会使用默认图形）：
- `player.png` - 玩家图片（50x50像素）
- `bullet.png` - 子弹图片（15x15像素）
- `background.png` - 背景图片（800x600像素）

### 第2步：放置文件

将图片文件放到以下位置：
```
OurGame/
└── build/
    ├── game_final.exe    ← 游戏程序
    └── resources/        ← 在这里放图片！
        ├── player.png
        ├── bullet.png
        └── background.png
```

### 第3步：运行游戏

直接运行 `game_final.exe`，游戏会自动加载图片！

## 📝 示例文件获取

### 方法1：使用在线工具快速制作

1. 访问 https://www.pixilart.com/ 或 https://www.piskelapp.com/
2. 创建简单的像素图
3. 导出为PNG
4. 重命名并放到 resources 文件夹

### 方法2：使用Paint快速制作

1. 打开Windows画图工具
2. 创建新图片，设置尺寸（如50x50）
3. 画一个简单的角色
4. 保存为PNG格式
5. 放到 resources 文件夹

### 方法3：下载免费素材

推荐网站：
- https://opengameart.org/
- https://kenney.nl/assets
- https://itch.io/game-assets/free

## 🔧 常见问题

### Q: 图片不显示怎么办？
A: 检查以下几点：
1. 文件名是否正确（player.png 不是 Player.png）
2. 文件是否在 build/resources/ 文件夹中
3. 图片文件是否损坏（尝试用看图软件打开）

### Q: 可以使用JPG格式吗？
A: 可以，但推荐PNG格式（支持透明背景）

### Q: 图片尺寸必须完全匹配吗？
A: 不需要，游戏会自动缩放，但推荐使用建议尺寸以获得最佳效果

### Q: 想改变资源文件夹的位置怎么办？
A: 修改 `our_game/src/ui/gameview.cpp` 中的路径即可

## 🎨 推荐的图片规格

| 资源类型 | 推荐尺寸 | 格式    | 是否透明 |
| -------- | -------- | ------- | -------- |
| 玩家     | 50x50    | PNG     | 建议     |
| 子弹     | 50x50    | PNG     | 建议     | (在png中子弹比较小)
| 背景     | 800x600  | PNG/JPG | 可选     |
| 敌人     | 40x40    | PNG     | 建议     |

## 💡 小技巧

1. **先测试后美化**：不用等美工资源齐全，可以先用默认图形开发
2. **逐步替换**：可以一个一个地添加图片资源
3. **保持透明**：角色和子弹建议使用透明背景的PNG
4. **尺寸合理**：太大的图片会被缩小，浪费内存
5. **命名规范**：严格按照文档中的文件名命名

## 📞 需要帮助？

查看完整文档：`UI_GUIDE.md`
资源文件夹说明：`build/resources/README.md`
