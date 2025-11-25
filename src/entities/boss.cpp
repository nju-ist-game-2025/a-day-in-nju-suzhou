#include "boss.h"

Boss::Boss(const QPixmap& pic, double scale)
    // 暂时这样设置 此后要为不同的Boss设计专门的属性和行为
    : Enemy(pic, scale) {
    setHealth(300);           // 更高血量
    setContactDamage(5);      // 更高伤害
    setVisionRange(350);      // 更多视野
    setAttackRange(60);       // 更大攻击范围（用于判定）
    setAttackCooldown(1500);  // 攻击频率略低
    setSpeed(1.5);            // 更笨重

    crash_r = 30;  // 实际攻击范围扩大

    damageScale = 0.8;  // 伤害减免

    // 使用绕圈接近模式，增加战术感和压迫感
    setMovementPattern(MOVE_CIRCLE);
    setCircleRadius(120.0);
}

Boss::~Boss() {
    // qDebug() <<"Boss被击败！";
}
