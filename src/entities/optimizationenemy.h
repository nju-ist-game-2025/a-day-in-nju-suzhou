#ifndef OPTIMIZATIONENEMY_H
#define OPTIMIZATIONENEMY_H

#include "scalingenemy.h"

/**
 * @brief Optimization敌人 - 第三关小怪
 *
 * 继承自ScalingEnemy，具有：
 * 1. 绕圈寻敌模式
 * 2. 大小伸缩变化
 * 3. 50%概率触发昏睡效果
 */
class OptimizationEnemy : public ScalingEnemy
{
    Q_OBJECT

public:
    explicit OptimizationEnemy(const QPixmap &pic, double scale = 1.0);
    ~OptimizationEnemy() override = default;
};

#endif // OPTIMIZATIONENEMY_H
