#ifndef ENTITY_H
#define ENTITY_H

#include <QGraphicsPixmapItem>
#include <QPointF>
#include <QTimer>
#include "constants.h"

class Entity : public QGraphicsPixmapItem, public QObject
{
protected:
    int xdir, ydir;//移动方向
    int curr_xdir, curr_ydir;//目前朝向
    double speed;
    int hurt;//既可以当作玩家敌人的攻击伤害，又可以作为射击的伤害
    QPixmap down, up, left, right;//不同朝向的图像，可选
public:
    int crash_r;
    explicit Entity(QGraphicsPixmapItem *parent = nullptr);
    void setSpeed(double sp) {speed = sp;};
    virtual void move() = 0;
    void setPixmapofDirs(QPixmap& down, QPixmap& up, QPixmap& left, QPixmap& right);
    virtual void takeDamage(int damage){};
    void setHurt(int h){hurt = h;};
    void setCrashR(int r){crash_r = r;};
};

#endif // ENTITY_H
