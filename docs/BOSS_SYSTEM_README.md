# Boss并列机制说明文档

## 概述

实现了类似Enemy的Boss并列机制，允许为每一关设计独特的Boss。当前支持两种Boss类型：

- **NightmareBoss**（第一关）
- **WashMachineBoss**（第二、三关）

## 文件结构

### 新增文件

```
src/entities/
├── nightmareboss.h         # Nightmare Boss头文件
├── nightmareboss.cpp       # Nightmare Boss实现
├── washmachineboss.h       # WashMachine Boss头文件
└── washmachineboss.cpp     # WashMachine Boss实现

assets/boss/
├── Nightmare/
│   └── Nightmare.png       # Nightmare Boss图片
└── WashMachine/
    ├── WashMachineNormally.png   # 洗衣机普通状态
    └── WashMachineAngrily.png    # 洗衣机愤怒状态（预留）
```

### 修改文件

- `src/core/resourcefactory.h` - 添加boss类型参数支持
- `src/world/level.h` - 添加boss工厂方法
- `src/world/level.cpp` - 实现boss工厂和生成逻辑
- `CMakeLists.txt` - 添加新boss源文件

## 核心机制

### 1. Boss基类继承

```cpp
Boss (基类)
 ├── NightmareBoss (第一关)
 └── WashMachineBoss (第二、三关)
```

### 2. Boss工厂方法

```cpp
Boss *Level::createBossByLevel(int levelNumber, const QPixmap &pic, double scale)
{
    switch (levelNumber)
    {
    case 1:
        return new NightmareBoss(pic, scale);
    case 2:
    case 3:
        return new WashMachineBoss(pic, scale);
    default:
        return new Boss(pic, scale);
    }
}
```

### 3. Boss图片加载

```cpp
// ResourceFactory支持根据类型加载boss图片
QPixmap createBossImage(int size, int levelNumber, const QString &bossType)
{
    if (bossType == "nightmare")
        imagePath = "assets/boss/Nightmare/Nightmare.png";
    else if (bossType == "washmachine")
        imagePath = "assets/boss/WashMachine/WashMachineNormally.png";
    // ...
}
```

### 4. 关卡Boss配置

在 `spawnEnemiesInRoom()` 中：

```cpp
// 根据关卡号确定Boss类型
QString bossType;
if (m_levelNumber == 1)
    bossType = "nightmare";
else
    bossType = "washmachine";

// 加载对应的Boss图片
QPixmap bossPix = ResourceFactory::createBossImage(80, m_levelNumber, bossType);

// 使用工厂方法创建对应的Boss
Boss *boss = createBossByLevel(m_levelNumber, bossPix, 1.0);
```

## 当前Boss属性

### NightmareBoss（第一关）

暂时沿用洗衣机的基础属性以便调试：

- **血量**：300
- **接触伤害**：5
- **视野范围**：350
- **攻击范围**：60
- **攻击冷却**：1500ms
- **移动速度**：1.5
- **伤害减免**：0.8（80%伤害）
- **一阶段模式**：预留多阶段Boss机制

### WashMachineBoss（第二、三关）

- 属性与NightmareBoss相同（当前配置）
- 可以独立调整属性

## 扩展建议

### 1. 为Nightmare添加特殊行为

```cpp
// 在nightmareboss.h中添加
private:
    int m_phase;  // 战斗阶段
    void switchPhase(int newPhase);  // 阶段切换
    void specialAttack();  // 特殊攻击

// 在nightmareboss.cpp中实现
void NightmareBoss::takeDamage(int damage) override
{
    Boss::takeDamage(damage);
    
    // 血量到50%时进入第二阶段
    if (getHealth() <= 150 && m_phase == 1)
    {
        switchPhase(2);
    }
}
```

### 2. 添加新Boss类型

1. 创建新的Boss类（如 `ThirdBoss.h/cpp`）
2. 在 `createBossByLevel()` 中添加case
3. 在 `ResourceFactory::createBossImage()` 中添加图片路径
4. 在 `assets/boss/` 中添加对应文件夹和图片
5. 更新 `CMakeLists.txt`

### 3. Boss配置化

可以考虑将Boss配置移到JSON中：

```json
{
  "bossConfig": {
    "type": "nightmare",
    "health": 300,
    "damage": 5,
    "speed": 1.5,
    "phase1": { /* 一阶段配置 */ },
    "phase2": { /* 二阶段配置 */ }
  }
}
```

## 测试建议

1. **第一关Boss测试**：
    - 进入第一关boss房间
    - 验证Nightmare Boss正确生成
    - 检查图片是否正确显示
    - 测试Boss行为和属性

2. **第二、三关Boss测试**：
    - 进入第二、三关boss房间
    - 验证WashMachine Boss正确生成
    - 确认与第一关使用不同的Boss类

3. **属性调试**：
    - 当前两种Boss使用相同属性便于调试
    - 后续可以根据关卡难度独立调整

## 下一步工作

1. **Nightmare特色设计**：
    - 设计独特的攻击模式
    - 实现多阶段机制
    - 添加特效和音效

2. **Boss对话**：
    - 为Nightmare配置专属对话（已支持）
    - 设计有趣的战前剧情

3. **难度平衡**：
    - 根据测试调整Boss属性
    - 为不同关卡的Boss设置不同难度
