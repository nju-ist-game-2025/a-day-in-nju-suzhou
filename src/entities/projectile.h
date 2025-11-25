#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <QGraphicsScene>
#include <QPixmap>
#include <QPointF>
#include <QVector>
#include "entity.h"

class Projectile : public Entity {
    QTimer* moveTimer;
    QTimer* crashTimer;
    int mode;           // Player发出：0, Enemy发出：1
    bool isDestroying;  // 标记对象正在销毁，防止重复操作
    bool m_isPaused;    // 暂停状态
   public:
    Projectile(int _mode, double _hurt, QPointF pos, const QPixmap& pic_bullet, double scale = 1.0);
    ~Projectile();

    void setDir(int x, int y) {
        xdir = x;
        ydir = y;
    };

    void move() override;

    void checkCrash();

    void destroy();  // 唯一的删除入口

    // 暂停控制
    void setPaused(bool paused);
    bool isPaused() const { return m_isPaused; }
};

#endif  // PROJECTILE_H
