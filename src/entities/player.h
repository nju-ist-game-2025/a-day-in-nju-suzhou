#ifndef PLAYER_H
#define PLAYER_H

#include <QKeyEvent>
#include <QPointF>
#include <QTimer>
#include "constants.h"
#include "entity.h"
#include "projectile.h"

const int max_red_contain = 12;
const int max_soul = 12;

class Player : public Entity {
    int redContainers;
    double redHearts;
    int soulHearts;
    int blackHearts;
    bool invincible;                   // 是否短暂无敌，防止持续攻击
    QMap<int, bool> keysPressed;       // 移动按键状态
    QMap<int, bool> shootKeysPressed;  // 射击按键状态
    QTimer* keysTimer;
    QTimer* crashTimer;
    QTimer* shootTimer;  // 射击检测定时器（持续检测）
    int shootCooldown;   // 射击冷却时间（毫秒）
    QPixmap pic_bullet;
    qint64 lastShootTime;  // 上次射击的时间戳

    void shoot(int key);  // 射击方法
    void checkShoot();    // 检测并执行射击

   public:
    Player(const QPixmap& pic_player, double scale = 1.0);
    void keyPressEvent(QKeyEvent* event) override;  // 控制移动
    void keyReleaseEvent(QKeyEvent* event) override;
    void move() override;
    void setBulletPic(const QPixmap& pic) { pic_bullet = pic; };
    void setShootCooldown(int milliseconds) { shootCooldown = milliseconds; }  // 设置射击冷却时间
    void takeDamage(int damage) override;                                      // 减血
    void crashEnemy();
    void die();
    void setInvincible();
};

#endif  // PLAYER_H
