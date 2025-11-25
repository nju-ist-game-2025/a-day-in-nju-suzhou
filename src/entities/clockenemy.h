#ifndef CLOCKENEMY_H
#define CLOCKENEMY_H

#include "enemy.h"

class Player;

/**
 * @brief 时钟怪物 - 攻击有50%概率触发昏睡效果
 * 特性：攻击命中时50%概率使玩家无法移动0.3秒
 */
class ClockEnemy : public Enemy
{
    Q_OBJECT

public:
    explicit ClockEnemy(const QPixmap &pic, double scale = 1.0);
    ~ClockEnemy() override = default;

protected:
    void attackPlayer() override;

private:
    void applySleepEffect();
};

#endif // CLOCKENEMY_H
