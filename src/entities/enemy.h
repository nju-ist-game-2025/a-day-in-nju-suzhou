#ifndef ENEMY_H
#define ENEMY_H

#include <QGraphicsScene>
#include <QTimer>
#include <QtMath>
#include "entity.h"

class Player;

class Enemy : public Entity {
    Q_OBJECT

   public:
    // 敌人状态枚举
    enum State {
        IDLE,    // 空闲/巡逻
        CHASE,   // 追击
        ATTACK,  // 攻击
        WANDER   // 漫游
    };

    Enemy(const QPixmap& pic, double scale = 1.0);
    ~Enemy();

    void move() override;
    void takeDamage(int damage) override;
    void setPlayer(Player* p) { player = p; }

    // 配置参数
    void setVisionRange(double range) { visionRange = range; }
    void setAttackRange(double range) { attackRange = range; }
    void setAttackCooldown(int ms) { attackCooldown = ms; }
    void setHealth(int hp) {
        health = hp;
        maxHealth = hp;
    }
    void setContactDamage(int dmg) { contactDamage = dmg; }
    int getContactDamage() const { return contactDamage; }
    void setHurt(double h) { Entity::setHurt(h); }

   private slots:
    void updateAI();
    void tryAttack();

   private:
    // AI相关
    State currentState;
    Player* player;
    QTimer* aiTimer;
    QTimer* attackTimer;
    QTimer* wanderTimer;

    // 属性
    int health;
    int maxHealth;
    int contactDamage;      // 接触伤害
    double visionRange;     // 视野范围
    double attackRange;     // 攻击范围
    int attackCooldown;     // 攻击冷却
    qint64 lastAttackTime;  // 上次攻击时间

    // 漫游相关
    QPointF wanderTarget;  // 漫游目标点
    int wanderCooldown;    // 漫游冷却

    // AI方法
    void updateState();
    bool canSeePlayer();
    double distanceToPlayer();
    void moveTowardsPlayer();
    void wander();
    void attackPlayer();
    QPointF getRandomWanderPoint();
};

#endif  // ENEMY_H
