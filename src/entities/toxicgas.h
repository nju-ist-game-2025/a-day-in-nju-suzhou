#ifndef TOXICGAS_H
#define TOXICGAS_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPixmap>
#include <QPointF>
#include <QTimer>

class Player;

/**
 * @brief ToxicGas - 洗衣机Boss变异阶段的毒气团投射物
 * 特性：
 * - 向玩家方向移动
 * - 碰到玩家后停止移动，持续造成伤害和减速
 * - 未碰到玩家则飞出场外消失
 * - 碰到后持续10秒消失
 */
class ToxicGas : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT

   public:
    ToxicGas(QPointF startPos, QPointF direction, const QPixmap& pic, Player* player);
    ~ToxicGas() override;

    void setPaused(bool paused);
    bool isPaused() const { return m_isPaused; }

    // 设置移动速度
    void setSpeed(double speed) { m_speed = speed; }

   private slots:
    void onMoveTimer();
    void onEffectTimer();
    void onDespawnTimer();

   private:
    void checkCollision();
    void applyDamageAndSlow();
    void stopMoving();

    Player* m_player;
    QTimer* m_moveTimer;     // 移动定时器
    QTimer* m_effectTimer;   // 持续伤害定时器
    QTimer* m_despawnTimer;  // 消失定时器

    QPointF m_direction;  // 移动方向（归一化）
    double m_speed;       // 移动速度
    bool m_isStationary;  // 是否已停止（碰到玩家）
    bool m_isPaused;      // 是否暂停
    bool m_isDestroying;  // 是否正在销毁

    int m_damagePerTick;  // 每次伤害量
    double m_slowFactor;  // 减速因子
};

#endif  // TOXICGAS_H
