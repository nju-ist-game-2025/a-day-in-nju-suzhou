#ifndef BOSS_H
#define BOSS_H

#include <QPixmap>
#include "enemy.h"

class Boss : public Enemy {
    Q_OBJECT

   public:
    explicit Boss(const QPixmap& pic, double scale = 1.5);

    ~Boss() override;
};

#endif  // BOSS_H
