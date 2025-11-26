#ifndef CLOCKENEMY_H
#define CLOCKENEMY_H

#include "enemy.h"

class Player;

/**
 * @brief 时钟怪物 - 攻击触发惊吓效果
 * 特性：攻击命中时触发惊吓效果，使玩家移动速度大幅增加但受到伤害提升150%，持续3秒
 */
class ClockEnemy : public Enemy
{
    Q_OBJECT

public:
    explicit ClockEnemy(const QPixmap &pic, double scale = 1.0);
    ~ClockEnemy() override = default;

    // 接触玩家时触发惊吓效果
    void onContactWithPlayer(Player *p) override;

protected:
    void attackPlayer() override;

private:
    void applyScareEffect(); // 惊吓效果
};

#endif // CLOCKENEMY_H
