#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include <QTimer>
#include <QKeyEvent>
#include <QPointF>
#include "constants.h"
#include "projectile.h"

class Player : public Entity
{
    QMap<int, bool> keysPressed;
    QTimer *keysTimer;
    QPixmap pic_bullet;
public:
    Player(const QPixmap& pic_player, double scale = 1.0);
    void keyPressEvent(QKeyEvent *event) override;//控制移动
    void keyReleaseEvent(QKeyEvent *event) override;
    void move() override;
    void setBulletPic(const QPixmap& pic) {pic_bullet = pic;};
};

#endif // PLAYER_H
