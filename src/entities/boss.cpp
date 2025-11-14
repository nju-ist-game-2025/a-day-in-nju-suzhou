#include "boss.h"

Boss::Boss(const QPixmap& pic, double scale)
    : Enemy(pic, scale) {
    setHealth(300); //更高血量
    setContactDamage(5); //更高伤害
    setVisionRange(350); //更多视野
    setAttackRange(60); //更大攻击范围（用于判定）
    setAttackCooldown(1500); //攻击频率略低
    setSpeed(1.5); //更笨重

    crash_r = 30; //实际攻击范围扩大

    damageScale = 0.8; //伤害减免
}

Boss::~Boss(){
    // qDebug() <<"Boss被击败！";
}
