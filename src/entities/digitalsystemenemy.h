#ifndef DIGITALSYSTEMENEMY_H
#define DIGITALSYSTEMENEMY_H

#include "scalingenemy.h"

/**
 * @brief DigitalSystem敌人 - 第三关小怪
 *
 * 继承自ScalingEnemy，具有：
 * 1. 绕圈寻敌模式
 * 2. 大小伸缩变化
 * 3. 50%概率触发昏睡效果
 */
class DigitalSystemEnemy : public ScalingEnemy
{
    Q_OBJECT

public:
    explicit DigitalSystemEnemy(const QPixmap &pic, double scale = 1.0);
    ~DigitalSystemEnemy() override = default;
};

#endif // DIGITALSYSTEMENEMY_H
