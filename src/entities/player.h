#ifndef PLAYER_H
#define PLAYER_H

#include <QKeyEvent>
#include <QPointF>
#include <QTimer>
#include <QVector>
#include "constants.h"
#include "entity.h"
#include "projectile.h"
#include "../core/audiomanager.h"

const int max_red_contain = 9;
const int max_soul = 6;
const int bomb_r = 60;
const int bombHurt = 1;

class Player : public Entity {
Q_OBJECT
    int redContainers;
    double redHearts;
    int soulHearts;
    int blackHearts;                   // 是否短暂无敌，防止持续攻击
    QMap<int, bool> shootKeysPressed;  // 射击按键状态
    QTimer *keysTimer;
    QTimer *crashTimer;
    QTimer *shootTimer;  // 射击检测定时器（持续检测）
    int shootCooldown;   // 射击冷却时间（毫秒）
    int shootType;       // 0=普通, 1=激光
    QPixmap pic_bullet;
    qint64 lastShootTime;  // 上次射击的时间戳
    void shoot(int key);   // 射击方法
    void checkShoot();     // 检测并执行射击

    // 需要UI显式实现的
    int bombs;
    int keys;

    int bulletHurt;  // 玩家子弹伤害，可配置
    bool isDead;     // 玩家是否已死亡

public:
    friend class Item;

    QMap<int, bool> keysPressed;

    Player(const QPixmap &pic_player, double scale = 1.0);

    void keyPressEvent(QKeyEvent *event) override;  // 控制移动
    void keyReleaseEvent(QKeyEvent *event) override;

    void move() override;

    void setBulletPic(const QPixmap &pic) { pic_bullet = pic; };

    void setShootCooldown(int milliseconds) { shootCooldown = milliseconds; }  // 设置射击冷却时间
    int getShootCooldown() { return shootCooldown; };

    void takeDamage(int damage) override;  // 减血
    float getCurrentHealth() const { return redHearts; }

    float getMaxHealth() const { return redContainers; }

    void addRedContainers(int n) {
        if (redContainers + n <= max_red_contain)
            redContainers += n;
    };

    void addRedHearts(double n) {
        if (redHearts + n <= redContainers)
            redHearts += n;
    }

    void addSoulHearts(int n) {
        if (soulHearts + n <= max_soul)
            soulHearts += n;
    };
    int getSoulHearts() {return soulHearts;};

    void addBlackHearts(int n) { blackHearts += n; };
    int getBlackHearts() {return blackHearts;};

    void setShootType(int type) { shootType = type; };

    void crashEnemy();

    void die();

    void setInvincible();

    void addBombs(int n) { bombs += n; };

    void placeBomb();

    void addKeys(int n) { keys += n; };

    int getKeys() { return keys; };

    // 玩家子弹伤害相关接口
    void setBulletHurt(int hurt) { bulletHurt = hurt; }

    int getBulletHurt() const { return bulletHurt; }

signals:

    void playerDied();  // 玩家死亡信号
    void healthChanged(float current, float max);

    void playerDamaged();

protected:
    void focusOutEvent(QFocusEvent *event) override;
};

#endif  // PLAYER_H

