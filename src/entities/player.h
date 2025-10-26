#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include <QTimer>
#include <QKeyEvent>
#include <QPointF>
#include "constants.h"
#include "projectile.h"

const int max_red_contain = 12;
const int max_soul = 12;

class Player : public Entity
{
    int redContainers;
    double redHearts;
    int soulHearts;
    int blackHearts;
    bool invincible;//是否短暂无敌，防止持续攻击
    QMap<int, bool> keysPressed;
    QTimer *keysTimer;
    QTimer *crashTimer;
    QPixmap pic_bullet;
public:
    Player(const QPixmap& pic_player, double scale = 1.0);
    void keyPressEvent(QKeyEvent *event) override;//控制移动
    void keyReleaseEvent(QKeyEvent *event) override;
    void move() override;
    void setBulletPic(const QPixmap& pic) {pic_bullet = pic;};
    void takeDamage(int damage) override;//减血
    void crashEnemy();
    void die();
    void setInvincible();
};

#endif // PLAYER_H
