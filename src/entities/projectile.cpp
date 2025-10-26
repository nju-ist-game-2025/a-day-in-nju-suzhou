#include "projectile.h"
#include "player.h"
#include "enemy.h"

Projectile::Projectile(int _mode, int _hurt, QPointF pos, const QPixmap& pic_bullet, double scale)
    :mode(_mode) {
    setTransformationMode(Qt::SmoothTransformation);
    xdir = 0;
    ydir = 0;
    speed = 1.0;
    hurt = _hurt;
    this->setPixmap(pic_bullet.scaled(scale, scale));
    this->setPos(pos);

    moveTimer = new QTimer();
    connect(moveTimer, &QTimer::timeout, this, &Projectile::move);
    moveTimer->start(16);

    crashTimer = new QTimer();
    connect(crashTimer, &QTimer::timeout, this, &Projectile::checkCrash);
    crashTimer->start(50);
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

void Projectile::checkCrash() {
    foreach(QGraphicsItem *item, scene()->items()) {
        if(mode){
            if (auto it = dynamic_cast<Player*>(item)) {
                if(abs(it->pos().x() - this->pos().x()) > it->crash_r ||
                    abs(it->pos().y() - this->pos().y()) > it->crash_r) continue;
                else it->takeDamage(hurt);
            }
        } else {
            if (auto it = dynamic_cast<Enemy*>(item)) {
                if(abs(it->pos().x() - this->pos().x()) > it->crash_r ||
                    abs(it->pos().y() - this->pos().y()) > it->crash_r) continue;
                else it->takeDamage(hurt);
            }
        }
    }
}
