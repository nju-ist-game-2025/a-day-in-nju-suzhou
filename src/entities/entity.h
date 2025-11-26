#ifndef ENTITY_H
#define ENTITY_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPointF>
#include <QTimer>
#include <QTransform>
#include <QVector>
#include "constants.h"

class Entity : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
   protected:
    int xdir{}, ydir{};            // 移动方向（由 player/enemy 维护）
    int curr_xdir{}, curr_ydir{};  // 目前朝向（可选使用）
    double speed{};
    double shootSpeed{};
    double hurt{};
    bool invincible{};
    bool isFlashing;

    // 保留四方向图（兼容 player/enemy）
    QPixmap down, up, left, right;

    // 记录当前面向（true = 向右，false = 向左）
    bool facingRight = true;

    // 当我们需要避免在翻转期间再次触发翻转时用到的锁
    bool flippingInProgress = false;

   public:
    int crash_r;
    double damageScale;

    explicit Entity(QGraphicsPixmapItem* parent = nullptr);

    [[nodiscard]] double getSpeed() const { return speed; };
    void setSpeed(double sp) { speed = sp; };

    [[nodiscard]] double getshootSpeed() const { return shootSpeed; };
    void setshootSpeed(double sp) { shootSpeed = sp; };

    [[nodiscard]] double getHurt() const { return hurt; };
    void setHurt(double h) { hurt = h; };

    virtual void move() = 0;

    // 保留原接口（player/enemy 可能会调用）
    void setPixmapofDirs(QPixmap& down, QPixmap& up, QPixmap& left, QPixmap& right);

    // 覆写 setPixmap，保持对 base 图的设置权限（仍调用基类实现）
    void setPixmap(const QPixmap& pix);

    virtual void takeDamage(int damage);
    void flash();
    void cancelFlash();  // 取消当前闪烁，用于阶段切换时重置

    void setCrashR(int r) { crash_r = r; };
    void setInvincible(bool i) { invincible = i; };

    // 重载 setPos，以便在每次位置更新时检查并调整朝向
    void setPos(qreal x, qreal y);
    void setPos(const QPointF& pos);

   protected:
    // 检查 xdir，必要时基于当前 pixmap 做水平镜像从而切换朝向
    void updateFacing();
};

#endif  // ENTITY_H
