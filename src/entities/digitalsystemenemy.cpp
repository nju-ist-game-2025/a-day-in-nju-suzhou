#include "digitalsystemenemy.h"
#include <QDebug>

DigitalSystemEnemy::DigitalSystemEnemy(const QPixmap &pic, double scale)
        : ScalingEnemy(pic, scale) {
    // DigitalSystem敌人的特定参数（可以根据需要调整）
    setHealth(25);
    setContactDamage(3);
    setCircleRadius(180.0);
    setSpeed(2.5);

    qDebug() << "创建DigitalSystem敌人";
}
