#ifndef MLETRAP_H
#define MLETRAP_H

#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QObject>
#include <QPainter>
#include <QPointF>
#include <QPointer>
#include <QTimer>

class Player;

/**
 * @brief MleTrap - 极大似然估计陷阱
 * 特性：
 * - 在预判位置显示红圈+螺旋图案
 * - 玩家踩中后被定身1.5秒
 * - 触发后或超时后消失
 */
class MleTrap : public QObject, public QGraphicsItem {
Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    MleTrap(QPointF position, Player *player);

    ~MleTrap() override;

    // QGraphicsItem 接口
    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // 设置陷阱参数
    void setRadius(double radius) { m_radius = radius; }

    void setRootDuration(int ms) { m_rootDuration = ms; }

    void setLifetime(int ms) { m_lifetime = ms; }

private slots:

    void onUpdateTimer();

    void onLifetimeTimeout();

private:
    void checkPlayerCollision();

    void applyRootEffect();

    void destroy();

    void drawSpiral(QPainter *painter);

    QPointer<Player> m_player;
    QTimer *m_updateTimer;    // 更新定时器
    QTimer *m_lifetimeTimer;  // 生命周期定时器

    QPointF m_position;    // 陷阱位置
    double m_radius;       // 陷阱半径
    int m_rootDuration;    // 定身持续时间（毫秒）
    int m_lifetime;        // 陷阱存在时间（毫秒）
    double m_spiralAngle;  // 螺旋动画角度
    bool m_triggered;      // 是否已触发
    bool m_isDestroying;   // 是否正在销毁
};

#endif  // MLETRAP_H
