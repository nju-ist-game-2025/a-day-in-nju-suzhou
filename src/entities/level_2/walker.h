#ifndef WALKER_H
#define WALKER_H

#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include "../enemy.h"

class Player;

class PoisonTrail;

/**
 * @brief Walker 敌人 - 毒行者
 * 特性：
 * - 极快的速度在全图随机游走，无碰撞伤害
 * - 留下墨绿色渐变毒痕，毒痕5秒后自动消散
 * - 对经过的enemy造成鼓舞效果（+50%移速，3秒），对同类Walker无效
 * - 玩家踩到毒痕100%触发中毒效果，3秒CD不可叠加
 * - 每3秒随机切换移动方向
 */
class Walker : public Enemy {
Q_OBJECT

public:
    explicit Walker(const QPixmap &pic, double scale = 1.0);

    ~Walker() override;

    // 暂停/恢复定时器
    void pauseTimers() override;

    void resumeTimers() override;

    // ========== 参数设置接口 ==========

    // 设置移动速度
    void setWalkerSpeed(double speed) { m_walkerSpeed = speed; }

    double getWalkerSpeed() const { return m_walkerSpeed; }

    // 设置方向切换间隔（毫秒）
    void setDirectionChangeInterval(int ms) { m_dirChangeInterval = ms; }

    int getDirectionChangeInterval() const { return m_dirChangeInterval; }

    // 设置毒痕生成间隔（毫秒）
    void setTrailSpawnInterval(int ms) { m_trailSpawnInterval = ms; }

    int getTrailSpawnInterval() const { return m_trailSpawnInterval; }

    // 设置毒痕持续时间（毫秒）
    void setTrailDuration(int ms) { m_trailDuration = ms; }

    int getTrailDuration() const { return m_trailDuration; }

    // 设置鼓舞效果持续时间（秒）
    void setEncourageDuration(double sec) { m_encourageDuration = sec; }

    double getEncourageDuration() const { return m_encourageDuration; }

    // 设置中毒效果持续时间（秒）
    void setPoisonDuration(double sec) { m_poisonDuration = sec; }

    double getPoisonDuration() const { return m_poisonDuration; }

protected:
    void executeMovement() override;

private slots:

    void changeDirection();  // 随机改变移动方向
    void spawnPoisonTrail(); // 生成毒痕

private:
    void initTimers();

    QPointF getRandomDirection(); // 获取随机方向向量

    // 定时器
    QTimer *m_directionTimer; // 方向切换定时器
    QTimer *m_trailTimer;     // 毒痕生成定时器

    // 当前移动方向（归一化向量）
    QPointF m_currentDirection;

    // ========== 可调整参数 ==========
    double m_walkerSpeed;       // 移动速度（默认：12.0，快速）
    int m_dirChangeInterval;    // 方向切换间隔（默认：3000ms）
    int m_trailSpawnInterval;   // 毒痕生成间隔（默认：100ms）
    int m_trailDuration;        // 毒痕持续时间（默认：3000ms）
    double m_encourageDuration; // 鼓舞效果持续时间（默认：3秒）
    double m_poisonDuration;    // 中毒效果持续时间（默认：3秒）

    // ========== 默认参数常量 ==========
    static constexpr double DEFAULT_WALKER_SPEED = 12.0;
    static constexpr int DEFAULT_DIR_CHANGE_INTERVAL = 3000;  // 3秒
    static constexpr int DEFAULT_TRAIL_SPAWN_INTERVAL = 100;  // 100ms
    static constexpr int DEFAULT_TRAIL_DURATION = 3000;       // 3秒
    static constexpr double DEFAULT_ENCOURAGE_DURATION = 3.0; // 3秒
    static constexpr double DEFAULT_POISON_DURATION = 3.0;    // 3秒
    static constexpr double DEFAULT_VISION_RANGE = 1000.0;    // 超大视野（全图游走）
};

/**
 * @brief 毒痕类 - Walker留下的有毒轨迹
 * 特性：
 * - 墨绿色渐变视觉效果
 * - 5秒后自动消散（带淡出动画）
 * - 对玩家：100%触发中毒，3秒CD
 * - 对敌人（非Walker）：+50%移速鼓舞效果，3秒CD
 */
class PoisonTrail : public QObject, public QGraphicsEllipseItem {
Q_OBJECT

public:
    explicit PoisonTrail(const QPointF &center, int duration, double encourageDur, double poisonDur,
                         QObject *parent = nullptr);

    ~PoisonTrail() override;

    void setScene(QGraphicsScene *scene);

    // 检测碰撞并应用效果
    void checkCollisions();

    // 静态方法：管理冷却时间
    static bool canApplyPoisonTo(Player *player);

    static bool canApplyEncourageTo(Enemy *enemy);

    static void markPoisonApplied(Player *player);

    static void markEncourageApplied(Enemy *enemy);

    static void clearCooldowns();

private slots:

    void updateFade();

    void onExpired();

private:
    void applyPoisonToPlayer(Player *player);

    void applyEncourageToEnemy(Enemy *enemy);

    QTimer *m_fadeTimer;
    QTimer *m_checkTimer;
    int m_totalDuration;        // 总持续时间
    int m_elapsedTime;          // 已经过时间
    double m_encourageDuration; // 鼓舞效果持续时间
    double m_poisonDuration;    // 中毒效果持续时间

    // 静态冷却管理
    static QMap<Player *, qint64> s_playerPoisonCooldowns;
    static QMap<Enemy *, qint64> s_enemyEncourageCooldowns;
    static constexpr int EFFECT_COOLDOWN_MS = 3000; // 3秒冷却

    // 毒痕大小
    static constexpr double TRAIL_RADIUS = 15.0;
};

#endif // WALKER_H
