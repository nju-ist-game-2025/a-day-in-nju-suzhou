#ifndef ENEMY_H
#define ENEMY_H

#include <QGraphicsScene>
#include <QTimer>
#include <QVector>
#include <QtMath>
#include "entity.h"
#include "statuseffect.h"

class Player;

class Enemy : public Entity
{
    Q_OBJECT

public:
    // 敌人状态枚举
    enum State
    {
        IDLE,   // 空闲/巡逻
        CHASE,  // 追击
        ATTACK, // 攻击
        WANDER  // 漫游
    };

    // 移动模式枚举
    enum MovementPattern
    {
        MOVE_NONE,          // 不自动移动（自定义移动逻辑）
        MOVE_DIRECT,        // 直线追击（默认行为）
        MOVE_ZIGZAG,        // Z字形追击（左右蛇形接近）
        MOVE_CIRCLE,        // 绕圈接近（螺旋式靠近玩家）
        MOVE_DASH,          // 冲刺模式（蓄力后快速冲向玩家）
        MOVE_KEEP_DISTANCE, // 保持距离（远程敌人用，保持一定距离）
        MOVE_DIAGONAL       // 斜向接近（斜着靠近以躲避玩家直线子弹）
    };

    Enemy(const QPixmap &pic, double scale = 1.0);

    ~Enemy();

    void move() override;

    void takeDamage(int damage) override;

    void setPlayer(Player *p) { player = p; }

    // 配置参数
    void setVisionRange(double range) { visionRange = range; }

    void setAttackRange(double range) { attackRange = range; }

    void setAttackCooldown(int ms) { attackCooldown = ms; }

    void setHealth(int hp)
    {
        health = hp;
        maxHealth = hp;
    }
    
    // 单独设置当前生命值（不改变上限）
    void setCurrentHealth(int hp)
    {
        health = qMin(hp, maxHealth);
    }
    
    int getMaxHealth() const { return maxHealth; }

    void setContactDamage(int dmg) { contactDamage = dmg; }

    int getContactDamage() const { return contactDamage; }

    void setHurt(double h) { Entity::setHurt(h); }

    void getEffects();

    int getHealth() { return health; };

    // 接触玩家时的特殊效果（子类可重写添加惊吓/昏迷等效果）
    virtual void onContactWithPlayer(Player *p) { Q_UNUSED(p); }

    // 移动模式配置
    void setMovementPattern(MovementPattern pattern) { m_movePattern = pattern; }

    MovementPattern getMovementPattern() const { return m_movePattern; }

    // 移动模式参数配置
    void setPreferredDistance(double dist) { m_preferredDistance = dist; }

    void setDashSpeed(double spd) { m_dashSpeed = spd; }

    void setDashChargeTime(int ms) { m_dashChargeMs = ms; }

    void setZigzagAmplitude(double amp) { m_zigzagAmplitude = amp; }

    void setCircleRadius(double radius) { m_circleRadius = radius; }

    void setCircleAngle(double angle) { m_circleAngle = angle; }  // 设置绕圈初始角度（弧度）

    // 暂停/恢复所有定时器（用于对话期间）
    virtual void pauseTimers();

    virtual void resumeTimers();

    bool isPaused() const { return m_isPaused; }

    // 召唤标记（召唤的敌人不触发bonus）
    void setIsSummoned(bool summoned) { m_isSummoned = summoned; }

    bool isSummoned() const { return m_isSummoned; }

signals:

    void dying(Enemy *enemy); // 敌人即将死亡信号（在deleteLater之前发出）

private slots:

    void updateAI();

    void tryAttack();

protected:
    // AI相关 - protected 允许子类访问
    State currentState;
    Player *player;
    QTimer *aiTimer;
    QTimer *moveTimer;
    QTimer *attackTimer;

    // 属性 - protected 允许子类访问
    int health;
    int maxHealth;
    int contactDamage;     // 接触伤害
    double visionRange;    // 视野范围
    double attackRange;    // 攻击范围
    int attackCooldown;    // 攻击冷却
    qint64 lastAttackTime; // 上次攻击时间
    bool firstBonus;

    // 漫游相关
    QPointF wanderTarget; // 漫游目标点
    int wanderCooldown;   // 漫游冷却

    // 移动模式相关
    MovementPattern m_movePattern; // 当前移动模式
    double m_zigzagPhase;          // Z字形相位（用于正弦波动）
    double m_zigzagAmplitude;      // Z字形振幅
    double m_circleAngle;          // 绕圈当前角度
    double m_circleRadius;         // 绕圈半径
    bool m_isDashing;              // 是否正在冲刺
    int m_dashChargeCounter;       // 冲刺蓄力计数器
    int m_dashChargeMs;            // 冲刺蓄力时间（毫秒）
    double m_dashSpeed;            // 冲刺速度倍率
    QPointF m_dashTarget;          // 冲刺目标位置
    int m_dashDuration;            // 冲刺持续时间计数器
    double m_preferredDistance;    // 保持距离模式的目标距离
    int m_diagonalDirection;       // 斜向方向 (1 或 -1)，用于斜向移动和保持距离模式
    bool m_isSummoned;             // 是否是被召唤的敌人（不触发bonus）
    bool m_isPaused;               // 是否处于暂停状态

    // AI方法 - protected 允许子类访问和重写
    void updateState();

    bool canSeePlayer();

    double distanceToPlayer();

    void moveTowardsPlayer();

    void wander();

    virtual void attackPlayer();

    QPointF getRandomWanderPoint();

    // 移动模式实现方法
    virtual void executeMovement(); // 根据当前模式执行移动（可重写）
    void moveZigzag();              // Z字形移动
    void moveCircle();              // 绕圈移动
    void moveDash();                // 冲刺移动
    void moveKeepDistance();        // 保持距离移动
    void moveDiagonal();            // 斜向移动
};

#endif // ENEMY_H
