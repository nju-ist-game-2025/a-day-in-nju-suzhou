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

    void setContactDamage(int dmg) { contactDamage = dmg; }

    int getContactDamage() const { return contactDamage; }

    void setHurt(double h) { Entity::setHurt(h); }

    void getEffects();

    int getHealth() { return health; };

    // 暂停/恢复所有定时器（用于对话期间）
    void pauseTimers();
    void resumeTimers();

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

    // AI方法 - protected 允许子类访问和重写
    void updateState();

    bool canSeePlayer();

    double distanceToPlayer();

    void moveTowardsPlayer();

    void wander();

    virtual void attackPlayer();

    QPointF getRandomWanderPoint();
};

#endif // ENEMY_H
