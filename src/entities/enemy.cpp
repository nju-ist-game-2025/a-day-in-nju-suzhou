#include "enemy.h"
#include <QDateTime>
#include <QGraphicsScene>
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include "player.h"

Enemy::Enemy(const QPixmap& pic, double scale)
    : Entity(nullptr),
      currentState(WANDER),
      player(nullptr),
      health(100),
      maxHealth(100),
      contactDamage(1),
      visionRange(250.0),
      attackRange(40.0),
      attackCooldown(1000),
      lastAttackTime(0),
      wanderCooldown(0) {
    // 设置图像
    setPixmap(pic.scaled(pic.width() * scale, pic.height() * scale,
                         Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // 设置基础属性
    speed = 2.0;
    hurt = contactDamage;
    crash_r = 20;
    damageScale = 1.0;
    invincible = false;

    xdir = 0;
    ydir = 0;
    curr_xdir = 0;
    curr_ydir = 1;

    // 禁用缓存以避免拖影
    setCacheMode(QGraphicsItem::NoCache);

    // 初始化随机漫游点
    wanderTarget = getRandomWanderPoint();

    // AI更新定时器 (每50ms更新一次AI)
    aiTimer = new QTimer(this);
    connect(aiTimer, &QTimer::timeout, this, &Enemy::updateAI);
    aiTimer->start(50);

    // 移动定时器 (每16ms移动一次，约60fps)
    QTimer* moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, &Enemy::move);
    moveTimer->start(16);

    // 攻击检测定时器
    attackTimer = new QTimer(this);
    connect(attackTimer, &QTimer::timeout, this, &Enemy::tryAttack);
    attackTimer->start(100);
}

Enemy::~Enemy() {
    if (aiTimer) {
        aiTimer->stop();
        delete aiTimer;
    }
    if (attackTimer) {
        attackTimer->stop();
        delete attackTimer;
    }
}

void Enemy::updateAI() {
    if (!scene())
        return;

    // 如果玩家不存在或已死亡，只执行漫游
    if (!player) {
        currentState = WANDER;
        wander();
        return;
    }

    updateState();

    switch (currentState) {
        case IDLE:
            xdir = 0;
            ydir = 0;
            break;

        case WANDER:
            wander();
            break;

        case CHASE:
            moveTowardsPlayer();
            break;

        case ATTACK:
            // 攻击状态下继续朝向玩家
            moveTowardsPlayer();
            break;
    }
}

void Enemy::updateState() {
    if (!player) {
        currentState = WANDER;
        return;
    }

    double dist = distanceToPlayer();

    // 状态转换逻辑
    if (dist <= attackRange) {
        currentState = ATTACK;
    } else if (canSeePlayer() && dist <= visionRange) {
        currentState = CHASE;
    } else {
        currentState = WANDER;
    }
}

bool Enemy::canSeePlayer() {
    if (!player)
        return false;

    double dist = distanceToPlayer();
    return dist <= visionRange;
}

double Enemy::distanceToPlayer() {
    if (!player)
        return 9999.0;

    QPointF enemyPos = pos();
    QPointF playerPos = player->pos();

    double dx = playerPos.x() - enemyPos.x();
    double dy = playerPos.y() - enemyPos.y();

    return qSqrt(dx * dx + dy * dy);
}

void Enemy::moveTowardsPlayer() {
    if (!player)
        return;

    QPointF enemyPos = pos();
    QPointF playerPos = player->pos();

    double dx = playerPos.x() - enemyPos.x();
    double dy = playerPos.y() - enemyPos.y();
    double dist = qSqrt(dx * dx + dy * dy);

    if (dist > 0.1) {
        // 归一化方向向量
        xdir = static_cast<int>(qRound((dx / dist) * 10));
        ydir = static_cast<int>(qRound((dy / dist) * 10));

        // 更新朝向
        if (qAbs(dx) > qAbs(dy)) {
            curr_xdir = (dx > 0) ? 1 : -1;
            curr_ydir = 0;
        } else {
            curr_xdir = 0;
            curr_ydir = (dy > 0) ? 1 : -1;
        }
    }
}

void Enemy::wander() {
    QPointF currentPos = pos();
    double dx = wanderTarget.x() - currentPos.x();
    double dy = wanderTarget.y() - currentPos.y();
    double dist = qSqrt(dx * dx + dy * dy);

    // 到达漫游点或需要新目标
    if (dist < 20 || wanderCooldown <= 0) {
        wanderTarget = getRandomWanderPoint();
        wanderCooldown = 60;  // 约3秒后重新选择目标
    } else {
        wanderCooldown--;

        // 朝向漫游点移动（速度较慢）
        if (dist > 0.1) {
            xdir = static_cast<int>(qRound((dx / dist) * 5));
            ydir = static_cast<int>(qRound((dy / dist) * 5));
        }
    }
}

QPointF Enemy::getRandomWanderPoint() {
    // 在场景范围内随机选择一个点
    double margin = 50;
    int xrange = qMax(1, static_cast<int>(scene_bound_x - 2 * margin));
    int yrange = qMax(1, static_cast<int>(scene_bound_y - 2 * margin));
    double x = margin + QRandomGenerator::global()->bounded(xrange);
    double y = margin + QRandomGenerator::global()->bounded(yrange);
    return QPointF(x, y);
}

void Enemy::tryAttack() {
    if (currentState != ATTACK || !player)
        return;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // 检查冷却时间
    if (currentTime - lastAttackTime < attackCooldown)
        return;

    attackPlayer();
    lastAttackTime = currentTime;
}

void Enemy::attackPlayer() {
    if (!player)
        return;

    // 近战攻击：检测碰撞
    QList<QGraphicsItem*> collisions = collidingItems();
    for (QGraphicsItem* item : collisions) {
        Player* p = dynamic_cast<Player*>(item);
        if (p) {
            p->takeDamage(contactDamage);
            break;
        }
    }
}

void Enemy::move() {
    if (!scene())
        return;

    // 计算新位置
    double newX = x() + xdir * speed / 10.0;
    double newY = y() + ydir * speed / 10.0;

    // 边界检测
    QRectF pixmapRect = pixmap().rect();
    if (newX < 0)
        newX = 0;
    if (newX + pixmapRect.width() > scene_bound_x)
        newX = scene_bound_x - pixmapRect.width();
    if (newY < 0)
        newY = 0;
    if (newY + pixmapRect.height() > scene_bound_y)
        newY = scene_bound_y - pixmapRect.height();

    setPos(newX, newY);
}

void Enemy::takeDamage(int damage) {
    flash();
    int realDamage = qMax(1, damage);  // 每次至少1点伤害
    health -= realDamage;
    if (health <= 0) {
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
    }
}
