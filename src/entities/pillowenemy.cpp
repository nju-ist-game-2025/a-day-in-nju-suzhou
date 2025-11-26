#include "pillowenemy.h"

PillowEnemy::PillowEnemy(const QPixmap &pic, double scale)
    : Enemy(pic, scale)
{
    // 设置移动模式为绕圈移动
    setMovementPattern(MOVE_CIRCLE);

    // 设置绕圈半径和速度等参数
    setCircleRadius(200.0); // 绕圈半径
    setSpeed(3.0);          // 移动速度
    setHealth(20);          // 生命值
    setContactDamage(2);    // 接触伤害
}
