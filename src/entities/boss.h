#ifndef BOSS_H
#define BOSS_H

#include <QPixmap>
#include "enemy.h"
class Boss : public Enemy {
   public:
    Boss(const QPixmap& pic, double scale = 1.0);
};

#endif  // BOSS_H
