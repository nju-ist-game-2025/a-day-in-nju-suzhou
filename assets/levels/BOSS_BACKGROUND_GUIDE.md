# Boss对话背景图片配置指南

## 功能说明

现在可以为每个boss房间的对话设置独立的背景图片，如果不设置则使用关卡默认背景图片。

## 配置方法

### 1. 准备背景图片

将你的boss对话背景图片放在项目的 `assets/galgame/` 目录下，例如：

```
assets/galgame/
├── l1.png           (第一关默认背景)
├── l2.png           (第二关默认背景)
├── l3.png           (第三关默认背景)
├── boss1.png        (第一关boss对话背景 - 可选)
├── boss2.png        (第二关boss对话背景 - 可选)
└── boss3.png        (第三关boss对话背景 - 可选)
```

**推荐图片规格**：

- 分辨率：800x600 像素
- 格式：PNG（支持透明度）或 JPG
- 风格：根据boss主题定制，可以是战斗场景、boss特写等

### 2. 在JSON中配置

在关卡JSON文件（如 `level1.json`）的boss房间配置中，添加 `bossDialogBackground` 字段：

```json
{
  "id": 8,
  "name": "Boss Room",
  "background": "map1",
  "hasBoss": true,
  "bossDialog": [
    "（智科er推开最后一扇门）",
    "**智科er** \n 这就是最终考验？",
    "【Boss】\n『欢迎来到地狱！』"
  ],
  "bossDialogBackground": "assets/galgame/boss1.png",
  "hasChest": true,
  "doors": {
    "up": -1,
    "down": -1,
    "left": -1,
    "right": 6
  }
}
```

### 3. 字段说明

| 字段                     | 类型     | 必填 | 说明              |
|------------------------|--------|----|-----------------|
| `bossDialogBackground` | String | 否  | boss对话背景图片的相对路径 |

**路径规则**：

- 相对于项目根目录的路径
- 例如：`"assets/galgame/boss1.png"`
- 如果不填写或留空，将使用该关卡的默认背景（l1.png, l2.png, l3.png）

### 4. 完整示例

#### 第一关 - 巨型闹钟Boss

```json
{
  "id": 8,
  "name": "Room 9 - Boss Room",
  "background": "map1",
  "hasBoss": true,
  "bossDialog": [
    "（智科er推开宿舍最深处的房门）",
    "**智科er** \n（眼前赫然矗立着一座巨型闹钟）",
    "【巨型闹钟】\n『滴答...滴答...滴答...』",
    "**智科er** \n 这就是...让我每天早起的罪魁祸首？",
    "【巨型闹钟】\n『你以为逃避闹钟，就能逃避责任吗？』"
  ],
  "bossDialogBackground": "assets/galgame/boss1.png",
  "hasChest": true,
  "doors": {
    "up": -1,
    "down": -1,
    "left": -1,
    "right": 6
  }
}
```

#### 第二关 - 脏袜子Boss

```json
{
  "id": 7,
  "name": "Boss Room",
  "background": "washroom",
  "hasBoss": true,
  "bossDialog": [
    "（洗衣房深处传来恶臭）",
    "**智科er** \n（捂住口鼻）这什么味道...",
    "【脏袜子Boss】\n『三个月没洗的臭味，如何？』"
  ],
  "bossDialogBackground": "assets/galgame/boss2.png",
  "hasChest": true,
  "doors": {
    "up": 5,
    "down": -1,
    "left": -1,
    "right": -1
  }
}
```

## 背景图片设计建议

### 1. 第一关 - 起床战争

- **主题**：宿舍、闹钟、早晨
- **色调**：温暖的晨光色调（橙黄色）
- **元素**：巨大的闹钟、床铺、窗帘透光
- **氛围**：紧张但不失温馨

### 2. 第二关 - 洗衣房

- **主题**：洗衣房、脏衣服、混乱
- **色调**：灰暗潮湿（灰蓝色）
- **元素**：洗衣机、堆积的袜子、水渍
- **氛围**：压抑、混乱

### 3. 第三关 - 教室

- **主题**：教室、考试、学习压力
- **色调**：冷色调（蓝白色）
- **元素**：黑板、试卷、课桌
- **氛围**：紧张、严肃

## 回退机制

如果配置的背景图片文件不存在或加载失败，系统会：

1. 在控制台输出警告信息
2. 自动回退到该关卡的默认背景（l1.png, l2.png, l3.png）
3. 继续正常显示对话

## 测试检查清单

- [ ] 背景图片文件已放置在正确的目录
- [ ] JSON配置中的路径正确（相对路径）
- [ ] 图片尺寸合适（推荐800x600）
- [ ] 进入boss房间时背景正确显示
- [ ] 对话文字在背景上清晰可读
- [ ] 如果图片加载失败，能正常回退到默认背景

## 常见问题

### Q: 背景图片没有显示？

A: 检查：

1. 文件路径是否正确（相对于项目根目录）
2. 文件扩展名大小写是否正确（.png vs .PNG）
3. 查看控制台是否有"图片文件不存在"的警告

### Q: 可以使用什么格式的图片？

A: 支持Qt可识别的所有图片格式，推荐使用PNG（支持透明）或JPG格式。

### Q: 不同关卡可以使用同一张boss背景吗？

A: 可以，只需在不同关卡的JSON中指向同一个图片路径即可。

### Q: 如何恢复使用默认背景？

A: 删除或不填写 `bossDialogBackground` 字段即可自动使用默认背景。

## 代码接口说明

**C++ 接口**：

```cpp
// Level.h
void showStoryDialog(const QStringList &dialogs, 
                     bool isBossDialog = false, 
                     const QString &customBackground = QString());

// 调用示例
showStoryDialog(bossDialogs, true, "assets/galgame/boss1.png");
```

**JSON 配置结构**：

```cpp
struct RoomConfig {
    // ... 其他字段
    bool hasBoss;                   // 是否有Boss
    QStringList bossDialog;         // Boss对话文案
    QString bossDialogBackground;   // Boss对话背景图片（可选）
    // ...
};
```
