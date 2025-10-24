#include "projectile.h"

Projectile::Projectile(QPointF pos, const QPixmap& pic_bullet, double scale) {
    setTransformationMode(Qt::SmoothTransformation);
    xdir = 0;
    ydir = 0;
    this->setPixmap(pic_bullet.scaled(scale, scale));
    this->setPos(pos);

    moveTimer = new QTimer();
    connect(moveTimer, &QTimer::timeout, this, &Projectile::move);
    moveTimer->start(16);
}

void Projectile::move() {
    if(pos().x() + xdir >= scene_bound_x || pos().y() + ydir >= scene_bound_y) {
        scene()->removeItem(this);
        //this->deleteLater();//理应删除但是危险，容易使游戏中断
    } else {
        QPointF dir(xdir, ydir);
        this->setPos(pos() + speed*dir);
    }
}
