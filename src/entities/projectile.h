#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "entity.h"
#include <QGraphicsScene>
#include <QPixmap>
#include <QPointF>

class Projectile : public Entity
{
    QTimer *moveTimer;
public:
    Projectile(QPointF pos, const QPixmap& pic_bullet, double scale = 1.0);
    void setDir(int x, int y){xdir = x; ydir = y;};
    void move() override;
};

#endif // PROJECTILE_H
