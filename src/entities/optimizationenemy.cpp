#include "optimizationenemy.h"
#include <QDebug>

OptimizationEnemy::OptimizationEnemy(const QPixmap &pic, double scale)
    : ScalingEnemy(pic, scale)
{
    // Optimization敌人的特定参数
    setHealth(25);
    setContactDamage(3);
    setCircleRadius(180.0);
    setSpeed(2.5);

    qDebug() << "创建Optimization敌人";
}
