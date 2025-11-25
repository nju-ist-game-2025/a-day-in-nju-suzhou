#include "washmachineboss.h"
#include <QDebug>

WashMachineBoss::WashMachineBoss(const QPixmap &pic, double scale)
    : Boss(pic, scale)
{
    // 洗衣机Boss的属性配置
    setHealth(300);
    setContactDamage(5);
    setVisionRange(350);
    setAttackRange(60);
    setAttackCooldown(1500);
    setSpeed(1.5);

    crash_r = 30;
    damageScale = 0.8;

    qDebug() << "WashMachine Boss创建";
}

WashMachineBoss::~WashMachineBoss()
{
    qDebug() << "WashMachine Boss被击败！";
}
