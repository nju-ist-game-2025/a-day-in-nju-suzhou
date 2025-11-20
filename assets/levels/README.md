# 关卡配置系统说明

## 概述

`LevelConfig` 类提供了一个通用的关卡配置系统，允许通过 JSON 文件定义关卡的房间布局、连接关系和内容。

## JSON 配置文件格式

关卡配置文件位于 `assets/levels/level{N}.json`，其中 `{N}` 是关卡号。

### 示例配置 (level1.json)

```json
{
    "name": "第一关 - 洗衣房",
    "startRoom": 0,
    "rooms": [
        {
            "background": "map1",
            "enemyCount": 3,
            "hasBoss": false,
            "hasChest": true,
            "isChestLocked": false,
            "doors": {
                "up": -1,
                "down": 1,
                "left": -1,
                "right": -1
            }
        }
    ]
}
```

### 配置字段说明

#### 顶层字段

- `name` (string): 关卡名称
- `startRoom` (int): 玩家出生的房间索引（从 0 开始）
- `rooms` (array): 房间配置列表

#### 房间配置字段

- `background` (string): 背景图片的配置 key（在 `config.json` 的 `assets` 中定义）
- `enemyCount` (int): 房间内普通敌人的数量
- `hasBoss` (bool): 是否有 Boss
- `hasChest` (bool): 是否有宝箱
- `isChestLocked` (bool): 宝箱是否锁定（需要钥匙）
- `doors` (object): 门的连接关系
    - `up` (int): 上门连接到的房间索引，-1 表示无门
    - `down` (int): 下门连接到的房间索引
    - `left` (int): 左门连接到的房间索引
    - `right` (int): 右门连接到的房间索引

## 使用方法

### 在 Level 类中使用

```cpp
LevelConfig levelConfig;
if (levelConfig.loadFromFile(levelNumber)) {
    int roomCount = levelConfig.getRoomCount();
    int startRoom = levelConfig.getStartRoomIndex();
    
    for (int i = 0; i < roomCount; ++i) {
        const RoomConfig& roomCfg = levelConfig.getRoom(i);
        
        // 创建房间
        bool hasUp = roomCfg.doorUp >= 0;
        bool hasDown = roomCfg.doorDown >= 0;
        bool hasLeft = roomCfg.doorLeft >= 0;
        bool hasRight = roomCfg.doorRight >= 0;
        
        Room* room = new Room(player, hasUp, hasDown, hasLeft, hasRight);
        
        // 根据配置生成敌人、宝箱等
        // ...
    }
}
```

## 设计优势

1. **灵活性**: 通过修改 JSON 文件即可调整关卡布局，无需重新编译
2. **可扩展**: 易于添加新的房间属性（如陷阱、NPC 等）
3. **版本控制友好**: JSON 文本格式便于跟踪变更
4. **快速迭代**: 关卡设计师可以独立调整配置

## 后续扩展建议

1. **敌人类型配置**: 添加敌人类型数组，支持不同种类的敌人
2. **物品掉落**: 配置宝箱内的物品列表
3. **房间特效**: 添加环境效果（如雾、光照）
4. **触发器**: 支持房间内的事件触发器
5. **多层地图**: 支持楼层/区域的概念
6. **传送门**: 添加非相邻房间的传送机制
