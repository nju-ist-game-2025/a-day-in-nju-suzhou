#include "nightmareboss.h"
#include <QDebug>

NightmareBoss::NightmareBoss(const QPixmap& pic, double scale)
    : Boss(pic, scale), m_phase(1) {
    // Nightmare Boss 的属性
    setHealth(350);           // Boss血量，比普通boss稍高
    setContactDamage(6);      // 接触伤害
    setVisionRange(400);      // 视野范围，更大
    setAttackRange(50);       // 攻击判定范围
    setAttackCooldown(1200);  // 攻击冷却，更快
    setSpeed(1.0);            // 移动速度

    crash_r = 35;       // 实际攻击范围
    damageScale = 0.7;  // 伤害减免，更耐打

    // 使用冲刺模式 - 残魂会突然冲向玩家
    setMovementPattern(MOVE_DASH);
    setDashChargeTime(1000);  // 1秒蓄力
    setDashSpeed(6.0);        // 高速冲刺，非常具有压迫感

    qDebug() << "Nightmare Boss创建，一阶段模式，使用冲刺移动";
}

NightmareBoss::~NightmareBoss() {
    qDebug() << "Nightmare Boss被击败！";
}
