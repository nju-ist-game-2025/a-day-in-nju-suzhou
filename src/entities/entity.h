#ifndef ENTITY_H
#define ENTITY_H

#include <QGraphicsPixmapItem>
#include <QImage>
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

    // 碰撞掩码相关（用于像素级碰撞检测）
    QImage collisionMask;         // 碰撞掩码（非透明区域为1）
    bool maskNeedsUpdate = true;  // 掩码是否需要更新
    int alphaThreshold = 50;      // Alpha 阈值，大于此值视为不透明

public:
    int crash_r;
    double damageScale;

    explicit Entity(QGraphicsPixmapItem *parent = nullptr);

    [[nodiscard]] double getSpeed() const { return speed; };

    void setSpeed(double sp) { speed = sp; };

    [[nodiscard]] double getshootSpeed() const { return shootSpeed; };

    void setshootSpeed(double sp) { shootSpeed = sp; };

    [[nodiscard]] double getHurt() const { return hurt; };

    void setHurt(double h) { hurt = h; };

    virtual void move() = 0;

    // 保留原接口（player/enemy 可能会调用）
    void setPixmapofDirs(QPixmap &down, QPixmap &up, QPixmap &left, QPixmap &right);

    // 覆写 setPixmap，保持对 base 图的设置权限（仍调用基类实现）
    void setPixmap(const QPixmap &pix);

    virtual void takeDamage(int damage);

    void flash();

    void cancelFlash();  // 取消当前闪烁，用于阶段切换时重置

    void setCrashR(int r) { crash_r = r; };

    void setInvincible(bool i) { invincible = i; };

    bool isInvincible() const { return invincible; }

    // 重载 setPos，以便在每次位置更新时检查并调整朝向
    void setPos(qreal x, qreal y);

    void setPos(const QPointF &pos);

    // 碰撞检测相关方法
    void generateCollisionMask();                                                          // 生成碰撞掩码
    void preloadCollisionMask() { generateCollisionMask(); }                               // 预加载碰撞掩码（游戏启动时调用）
    const QImage &getCollisionMask();                                                      // 获取碰撞掩码（惰性生成）
    void invalidateCollisionMask() { maskNeedsUpdate = true; }                             // 标记掩码需要更新
    bool hasCollisionMask() const { return !collisionMask.isNull() && !maskNeedsUpdate; }  // 检查掩码是否已加载

    // 获取实际pixmap的场景边界框（不包含boundingRect扩展的区域，如血条）
    QRectF pixmapSceneBoundingRect() const;

    static bool pixelCollision(Entity *a, Entity *b);  // Entity 之间的像素级碰撞检测

    // 通用像素级碰撞检测（用于非Entity对象如ExamPaper）
    static bool pixelCollisionWithPixmapItem(Entity *entity, QGraphicsPixmapItem *item, int alphaThreshold = 50);

protected:
    // 检查 xdir，必要时基于当前 pixmap 做水平镜像从而切换朝向
    void updateFacing();
};

#endif  // ENTITY_H
