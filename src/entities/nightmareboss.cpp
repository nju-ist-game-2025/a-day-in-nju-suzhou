#include "nightmareboss.h"
#include <QDebug>

NightmareBoss::NightmareBoss(const QPixmap &pic, double scale)
    : Boss(pic, scale), m_phase(1)
{
    // 暂时沿用洗衣机的基础属性，方便调试
    setHealth(300);          // Boss血量
    setContactDamage(5);     // 接触伤害
    setVisionRange(350);     // 视野范围
    setAttackRange(60);      // 攻击判定范围
    setAttackCooldown(1500); // 攻击冷却
    setSpeed(1.5);           // 移动速度

    crash_r = 30;      // 实际攻击范围
    damageScale = 0.8; // 伤害减免

    qDebug() << "Nightmare Boss创建，一阶段模式";
}

NightmareBoss::~NightmareBoss()
{
    qDebug() << "Nightmare Boss被击败！";
}
