#ifndef EXAMPAPER_H
#define EXAMPAPER_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QTimer>

class Player;

/**
 * @brief ExamPaper - 期中考卷投射物
 * 特性：
 * - 旋转飞向玩家
 * - 击中玩家：1点伤害 + 2秒晕厥效果
 * - 飞出场景后消失
 */
class ExamPaper : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT

   public:
    ExamPaper(QPointF startPos, QPointF direction, const QPixmap& pic, Player* player);
    ~ExamPaper() override;

    // 设置飞行速度
    void setSpeed(double speed) { m_speed = speed; }

    // 设置伤害
    void setDamage(int damage) { m_damage = damage; }

    // 设置晕厥时间（毫秒）
    void setStunDuration(int ms) { m_stunDuration = ms; }

    // 暂停控制
    void setPaused(bool paused);
    bool isPaused() const { return m_isPaused; }

   private slots:
    void onMoveTimer();

   private:
    void checkCollision();
    void applyStunEffect();
    void destroy();

    QPointer<Player> m_player;
    QTimer* m_moveTimer;

    QPointF m_direction;     // 移动方向（归一化）
    double m_speed;          // 移动速度
    double m_rotation;       // 当前旋转角度
    double m_rotationSpeed;  // 旋转速度
    int m_damage;            // 伤害
    int m_stunDuration;      // 晕厥持续时间（毫秒）
    bool m_isPaused;         // 是否暂停
    bool m_isDestroying;     // 是否正在销毁
};

#endif  // EXAMPAPER_H
