#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"

class Enemy : public Entity
{
public:
    Enemy();
    int crash_hurt;//与player碰撞的带给player的直接伤害
    //void takeDamage(int damage) override;
};

#endif // ENEMY_H
