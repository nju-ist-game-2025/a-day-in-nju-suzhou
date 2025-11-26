#ifndef PILLOWENEMY_H
#define PILLOWENEMY_H

#include "enemy.h"

class PillowEnemy : public Enemy
{
    Q_OBJECT

public:
    explicit PillowEnemy(const QPixmap &pic, double scale = 1.0);
    ~PillowEnemy() override = default;

    // 接触玩家时触发昏睡效果
    void onContactWithPlayer(Player *p) override;

protected:
    void attackPlayer() override;

private:
    void applySleepEffect(); // 100%概率触发昏睡效果
};

#endif // PILLOWENEMY_H
