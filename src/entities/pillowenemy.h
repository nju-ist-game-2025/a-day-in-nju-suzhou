#ifndef PILLOWENEMY_H
#define PILLOWENEMY_H

#include "enemy.h"

class PillowEnemy : public Enemy
{
    Q_OBJECT

public:
    explicit PillowEnemy(const QPixmap &pic, double scale = 1.0);
    ~PillowEnemy() override = default;
};

#endif // PILLOWENEMY_H
