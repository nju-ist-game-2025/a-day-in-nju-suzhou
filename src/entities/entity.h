#ifndef ENTITY_H
#define ENTITY_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPointF>
#include <QTimer>
#include "constants.h"

class Entity : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
   protected:
    int xdir, ydir;            // 移动方向
    int curr_xdir, curr_ydir;  // 目前朝向
    double speed;
    double shootSpeed;
    double hurt;  // 既可以当作玩家敌人的攻击伤害，又可以作为射击的伤害
    bool invincible;
    QPixmap down, up, left, right;  // 不同朝向的图像，可选
   public:
    int crash_r;
    double damageScale;  // 用于伤害减免
    explicit Entity(QGraphicsPixmapItem* parent = nullptr);

    double getSpeed() { return speed; };
    void setSpeed(double sp) { speed = sp; };
    double getshootSpeed() { return shootSpeed; };
    void setshootSpeed(double sp) { shootSpeed = sp; };
    double getHurt() { return hurt; };
    void setHurt(double h) { hurt = h; };

    virtual void move() = 0;
    void setPixmapofDirs(QPixmap& down, QPixmap& up, QPixmap& left, QPixmap& right);
    virtual void takeDamage(int damage);
    void flash();  // 受击闪烁效果
    void setCrashR(int r) { crash_r = r; };
    void setInvincible(bool i) { invincible = i; };
};

#endif  // ENTITY_H
