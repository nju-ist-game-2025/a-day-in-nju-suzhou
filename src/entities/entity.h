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
    QPixmap down, up, left, right;//不同朝向的图像，可选
public:
    explicit Entity(QGraphicsPixmapItem *parent = nullptr);
    void setSpeed(double sp) {speed = sp;};
    virtual void move() = 0;
    void setPixmapofDirs(QPixmap& down, QPixmap& up, QPixmap& left, QPixmap& right);
};

#endif // ENTITY_H
