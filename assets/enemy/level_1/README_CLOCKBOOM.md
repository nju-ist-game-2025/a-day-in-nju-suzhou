# Clock Boom Enemy Assets

## Required Files

为了让ClockBoom敌人正常工作，需要在此文件夹中准备以下图片：

1. **clock_boom.png** - 普通状态的clock_boom图片（已存在）
2. **clock_boom_red.png** - 深红色闪烁状态的clock_boom图片（**需要添加**）

## 图片说明

- **clock_boom_red.png** 应该是clock_boom.png的深红色版本
- 用于倒计时期间的闪烁效果（每0.5秒在两个图片之间切换）
- 建议尺寸：40x40像素（会自动缩放）

## ClockBoom机制

1. 不移动、不巡逻
2. 首次与玩家碰撞后触发2.5秒倒计时
3. 倒计时期间在普通图片和红色图片之间闪烁（每0.5秒切换）
4. 倒计时结束后爆炸，对范围100像素内：
    - 玩家造成1点伤害
    - 其他敌人造成3点伤害

## 生成规则（第一关）

- 房间1（room index 1）：不生成clock_boom
- 房间k（k > 1）：生成 k-1 个clock_boom
- 例如：房间3生成2个，房间5生成4个，房间8生成7个
