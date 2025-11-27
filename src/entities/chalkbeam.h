#ifndef CHALKBEAM_H
#define CHALKBEAM_H

#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QObject>
#include <QPixmap>
#include <QPointF>
#include <QTimer>

class Player;

/**
 * @brief ChalkBeam - 粉笔光束特效（随机点名/挂科警告）
 * 特性：
 * - 在目标位置显示红色警告圈
 * - 警告时间后粉笔落下，产生爆炸
 * - 在爆炸范围内对玩家造成伤害
 */
class ChalkBeam : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT

   public:
    ChalkBeam(QPointF targetPos, const QPixmap& beamPic, QGraphicsScene* scene);
    ~ChalkBeam() override;

    // 开始警告阶段
    void startWarning();

    // 设置警告时间（毫秒）
    void setWarningTime(int ms) { m_warningTime = ms; }

    // 设置伤害
    void setDamage(int damage) { m_damage = damage; }

    // 设置爆炸半径
    void setExplosionRadius(double radius) { m_explosionRadius = radius; }

   private slots:
    void onWarningTimeout();
    void onFallTimer();
    void onExplosionComplete();

   private:
    void createWarningCircle();
    void startFalling();
    void explode();
    void damagePlayer();

    QGraphicsScene* m_scene;
    QPointF m_targetPos;   // 目标位置
    QPixmap m_beamPixmap;  // 粉笔图片

    QGraphicsEllipseItem* m_warningCircle;  // 警告圈
    QTimer* m_warningTimer;                 // 警告定时器
    QTimer* m_fallTimer;                    // 下落定时器

    int m_warningTime;         // 警告时间（毫秒）
    int m_damage;              // 伤害
    double m_explosionRadius;  // 爆炸半径
    double m_fallSpeed;        // 下落速度
    double m_currentY;         // 当前Y坐标
    bool m_isDestroying;       // 是否正在销毁
};

#endif  // CHALKBEAM_H
