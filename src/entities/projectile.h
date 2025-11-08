#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "entity.h"
#include <QGraphicsScene>
#include <QPixmap>
#include <QPointF>

class Projectile : public Entity
{
    QTimer *moveTimer;
    QTimer *crashTimer;
    int mode;//Player发出：0, Enemy发出：1
public:
    Projectile(int _mode, double _hurt, QPointF pos, const QPixmap& pic_bullet, double scale = 1.0);
    void setDir(int x, int y){xdir = x; ydir = y;};
    void move() override;
    void checkCrash();
};

#endif // PROJECTILE_H
