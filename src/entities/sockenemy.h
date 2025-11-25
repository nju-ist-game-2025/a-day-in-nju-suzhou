#ifndef SOCKENEMY_H
#define SOCKENEMY_H

#include "enemy.h"

/**
 * @brief 袜子怪物基类
 * 特性：攻击有50%概率触发中毒效果
 */
class SockEnemy : public Enemy
{
    Q_OBJECT

public:
    explicit SockEnemy(const QPixmap &pic, double scale = 1.0);
    ~SockEnemy() override = default;

protected:
    void attackPlayer() override;
    void applyPoisonEffect();
};

/**
 * @brief 普通袜子怪物 - 使用默认敌人机制
 */
class SockNormal : public SockEnemy
{
    Q_OBJECT

public:
    explicit SockNormal(const QPixmap &pic, double scale = 1.0);
    ~SockNormal() override = default;
};

/**
 * @brief 愤怒袜子怪物 - 移速和伤害提升150%
 */
class SockAngrily : public SockEnemy
{
    Q_OBJECT

public:
    explicit SockAngrily(const QPixmap &pic, double scale = 1.0);
    ~SockAngrily() override = default;
};

#endif // SOCKENEMY_H
