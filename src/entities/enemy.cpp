#include "enemy.h"
#include <QDateTime>
#include <QDebug>
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include "../core/audiomanager.h"
#include "../ui/explosion.h"
#include "player.h"

Enemy::Enemy(const QPixmap& pic, double scale)
    : Entity(nullptr),
      currentState(WANDER),
      player(nullptr),
      health(10),
      maxHealth(10),
      contactDamage(1),
      visionRange(250.0),
      attackRange(40.0),
      attackCooldown(1000),
      lastAttackTime(0),
      wanderCooldown(0),
      m_movePattern(MOVE_DIRECT),
      m_zigzagPhase(0.0),
      m_zigzagAmplitude(80.0),
      m_circleAngle(0.0),
      m_circleRadius(100.0),
      m_isDashing(false),
      m_dashChargeCounter(0),
      m_dashChargeMs(1500),
      m_dashSpeed(4.0),
      m_dashDuration(0),
      m_preferredDistance(150.0),
      m_diagonalDirection(1),
      m_isSummoned(false),
      m_isPaused(false) {
    // 设置图像
    setPixmap(pic.scaled(pic.width() * scale, pic.height() * scale,
                         Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // 设置基础属性
    speed = 2.0;
    hurt = contactDamage;
    damageScale = 1.0;
    invincible = false;

    xdir = 0;
    ydir = 0;
    curr_xdir = 0;
    curr_ydir = 1;

    firstBonus = true;

    // 禁用缓存以避免拖影
    setCacheMode(QGraphicsItem::NoCache);

    // 初始化随机漫游点
    wanderTarget = getRandomWanderPoint();

    // AI更新定时器
    aiTimer = new QTimer(this);
    connect(aiTimer, &QTimer::timeout, this, &Enemy::updateAI);
    aiTimer->start(100);

    // 移动定时器 (每20ms移动一次)
    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, &Enemy::move);
    moveTimer->start(20);

    // 攻击检测定时器
    attackTimer = new QTimer(this);
    connect(attackTimer, &QTimer::timeout, this, &Enemy::tryAttack);
    attackTimer->start(100);
}

Enemy::~Enemy() {
    if (aiTimer) {
        aiTimer->stop();
        delete aiTimer;
        aiTimer = nullptr;
    }
    if (moveTimer) {
        moveTimer->stop();
        delete moveTimer;
        moveTimer = nullptr;
    }
    if (attackTimer) {
        attackTimer->stop();
        delete attackTimer;
        attackTimer = nullptr;
    }
}

void Enemy::getEffects() {
    if (!player)
        return;
    if (player->getCurrentHealth() < 1)
        return;

    int type = QRandomGenerator::global()->bounded(4);
    StatusEffect* effect = nullptr;

    switch (type) {
        case 0: {
            // 防止中毒效果在delete后施加
            int duration = (3 > player->getCurrentHealth() * 2 ? (int)player->getCurrentHealth() : 3);
            effect = new PoisonEffect(player, duration, 1);
            break;
        }
        case 1:
            effect = new SpeedEffect(5, 0.5);
            break;
        case 2:
            effect = new DamageEffect(5, 0.5);
            break;
        case 3:
            effect = new shootSpeedEffect(5, 0.5);
            break;
    }

    // 50% 概率应用效果
    if (QRandomGenerator::global()->bounded(2) == 0) {
        if (effect) {
            effect->applyTo(player);
            // effect会在expire()中调用deleteLater()自我销毁
        }
    } else {
        // 如果未选中应用，需要手动删除创建的对象
        if (effect) {
            delete effect;
        }
    }
}

void Enemy::updateAI() {
    if (!scene())
        return;

    // 暂停状态下不更新AI
    if (m_isPaused)
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
            executeMovement();  // 使用配置的移动模式
            break;

        case ATTACK:
            // 攻击状态下继续使用配置的移动模式
            executeMovement();
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
    // 暂停状态下不攻击
    if (m_isPaused)
        return;

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

    // 暂停状态下不攻击
    if (m_isPaused)
        return;

    // 近战攻击：使用像素级碰撞检测
    if (Entity::pixelCollision(this, player)) {
        player->takeDamage(contactDamage);
        // 基类不再有特殊效果，子类可以重写此方法添加特殊效果
    }
}

void Enemy::move() {
    if (!scene())
        return;

    // 暂停状态下不移动
    if (m_isPaused)
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
        // 立即停止所有定时器，防止死亡后仍在运行AI
        if (aiTimer) {
            aiTimer->stop();
        }
        if (moveTimer) {
            moveTimer->stop();
        }
        if (attackTimer) {
            attackTimer->stop();
        }

        // 播放死亡音效
        AudioManager::instance().playSound("enemy_death");
        qDebug() << "敌人死亡音效已触发";

        // 创建爆炸动画
        Explosion* explosion = new Explosion();
        explosion->setPos(this->pos());  // 在敌人位置创建爆炸
        if (scene()) {
            scene()->addItem(explosion);
            explosion->startAnimation();
            qDebug() << "创建爆炸动画在位置:" << this->pos();
        }

        qDebug() << "Enemy::takeDamage - 敌人死亡，发出dying信号";
        emit dying(this);  // 在删除之前发出信号

        if (scene()) {
            scene()->removeItem(this);
        }

        // 立即删除，不再延迟
        deleteLater();
    }
}

// ============== 移动模式实现 ==============

void Enemy::executeMovement() {
    switch (m_movePattern) {
        case MOVE_DIRECT:
            moveTowardsPlayer();
            break;
        case MOVE_ZIGZAG:
            moveZigzag();
            break;
        case MOVE_CIRCLE:
            moveCircle();
            break;
        case MOVE_DASH:
            moveDash();
            break;
        case MOVE_KEEP_DISTANCE:
            moveKeepDistance();
            break;
        case MOVE_DIAGONAL:
            moveDiagonal();
            break;
        default:
            moveTowardsPlayer();
            break;
    }
}

void Enemy::moveZigzag() {
    if (!player)
        return;

    QPointF enemyPos = pos();
    QPointF playerPos = player->pos();

    double dx = playerPos.x() - enemyPos.x();
    double dy = playerPos.y() - enemyPos.y();
    double dist = qSqrt(dx * dx + dy * dy);

    if (dist > 0.1) {
        // 归一化主方向
        double dirX = dx / dist;
        double dirY = dy / dist;

        // 计算垂直于主方向的侧向量
        double perpX = -dirY;
        double perpY = dirX;

        // 使用正弦函数产生Z字形偏移
        m_zigzagPhase += 0.2;  // 控制摆动频率
        if (m_zigzagPhase > 2 * M_PI)
            m_zigzagPhase -= 2 * M_PI;

        // 偏移量使用固定的振幅比例
        double zigzagOffset = qSin(m_zigzagPhase) * 0.8;  // 0.8是侧向移动的强度

        // 组合主方向和侧向偏移
        // 主方向占比0.6，侧向占比根据正弦波动
        double finalDirX = dirX * 0.7 + perpX * zigzagOffset;
        double finalDirY = dirY * 0.7 + perpY * zigzagOffset;

        // 重新归一化
        double finalDist = qSqrt(finalDirX * finalDirX + finalDirY * finalDirY);
        if (finalDist > 0.1) {
            xdir = static_cast<int>(qRound((finalDirX / finalDist) * 10));
            ydir = static_cast<int>(qRound((finalDirY / finalDist) * 10));
        }

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

void Enemy::moveCircle() {
    if (!player)
        return;

    QPointF enemyPos = pos();
    QPointF playerPos = player->pos();

    double dx = playerPos.x() - enemyPos.x();
    double dy = playerPos.y() - enemyPos.y();
    double dist = qSqrt(dx * dx + dy * dy);

    // 逐渐增加角度，实现绕圈效果
    m_circleAngle += 0.05;
    if (m_circleAngle > 2 * M_PI)
        m_circleAngle -= 2 * M_PI;

    // 计算目标点：在玩家周围的圆上，但半径逐渐减小
    double targetRadius = qMax(attackRange, dist * 0.8);  // 逐渐靠近
    double targetX = playerPos.x() + targetRadius * qCos(m_circleAngle);
    double targetY = playerPos.y() + targetRadius * qSin(m_circleAngle);

    // 朝向目标点移动
    double toDstX = targetX - enemyPos.x();
    double toDstY = targetY - enemyPos.y();
    double toDstDist = qSqrt(toDstX * toDstX + toDstY * toDstY);

    if (toDstDist > 0.1) {
        xdir = static_cast<int>(qRound((toDstX / toDstDist) * 10));
        ydir = static_cast<int>(qRound((toDstY / toDstDist) * 10));

        // 更新朝向（始终面向玩家）
        if (qAbs(dx) > qAbs(dy)) {
            curr_xdir = (dx > 0) ? 1 : -1;
            curr_ydir = 0;
        } else {
            curr_xdir = 0;
            curr_ydir = (dy > 0) ? 1 : -1;
        }
    }
}

void Enemy::moveDash() {
    if (!player)
        return;

    QPointF enemyPos = pos();
    QPointF playerPos = player->pos();

    double dx = playerPos.x() - enemyPos.x();
    double dy = playerPos.y() - enemyPos.y();
    double dist = qSqrt(dx * dx + dy * dy);

    if (!m_isDashing) {
        // 蓄力阶段
        m_dashChargeCounter += 100;  // AI更新间隔约100ms

        // 蓄力时缓慢追踪玩家（而不是完全停止）
        if (dist > 0.1) {
            // 蓄力时以较低速度追踪
            xdir = static_cast<int>(qRound((dx / dist) * 3));
            ydir = static_cast<int>(qRound((dy / dist) * 3));
        }

        if (m_dashChargeCounter >= m_dashChargeMs) {
            // 蓄力完成，开始冲刺
            m_isDashing = true;
            m_dashTarget = playerPos;  // 锁定冲刺目标
            m_dashDuration = 30;       // 冲刺持续帧数
            m_dashChargeCounter = 0;
        }
    } else {
        // 冲刺阶段
        double dxTarget = m_dashTarget.x() - enemyPos.x();
        double dyTarget = m_dashTarget.y() - enemyPos.y();
        double distTarget = qSqrt(dxTarget * dxTarget + dyTarget * dyTarget);

        if (distTarget > 15 && m_dashDuration > 0) {
            // 高速冲向目标
            xdir = static_cast<int>(qRound((dxTarget / distTarget) * 10 * m_dashSpeed));
            ydir = static_cast<int>(qRound((dyTarget / distTarget) * 10 * m_dashSpeed));
            m_dashDuration--;

            // 更新朝向
            if (qAbs(dxTarget) > qAbs(dyTarget)) {
                curr_xdir = (dxTarget > 0) ? 1 : -1;
                curr_ydir = 0;
            } else {
                curr_xdir = 0;
                curr_ydir = (dyTarget > 0) ? 1 : -1;
            }
        } else {
            // 冲刺结束，进入冷却
            m_isDashing = false;
            m_dashChargeCounter = 0;
            // 不要立即停止，继续缓慢追踪
            if (dist > 0.1) {
                xdir = static_cast<int>(qRound((dx / dist) * 3));
                ydir = static_cast<int>(qRound((dy / dist) * 3));
            }
        }
    }

    // 更新朝向（始终面向玩家）
    if (qAbs(dx) > qAbs(dy)) {
        curr_xdir = (dx > 0) ? 1 : -1;
        curr_ydir = 0;
    } else {
        curr_xdir = 0;
        curr_ydir = (dy > 0) ? 1 : -1;
    }
}

// 保持距离移动模式，适合远程敌人
// 行为：
//   1. X方向：保持与玩家的水平距离（m_preferredDistance）
//   2. Y方向：主动对齐玩家高度，以便水平射击能命中
void Enemy::moveKeepDistance() {
    if (!player)
        return;

    QPointF enemyPos = pos();
    QPointF playerPos = player->pos();

    double dx = playerPos.x() - enemyPos.x();
    double dy = playerPos.y() - enemyPos.y();
    double dist = qSqrt(dx * dx + dy * dy);

    if (dist < 0.1) {
        xdir = 0;
        ydir = 0;
        return;
    }

    double moveSpeed = 4.0;    // 移动速度（较平滑）
    double tolerance = 30.0;   // X轴距离容差
    double yTolerance = 20.0;  // Y轴对齐容差

    // ========== X方向：保持水平距离 ==========
    double absDx = qAbs(dx);
    if (absDx < m_preferredDistance - tolerance) {
        // 太近了，水平后退（远离玩家）
        xdir = (dx > 0) ? -static_cast<int>(moveSpeed) : static_cast<int>(moveSpeed);
    } else if (absDx > m_preferredDistance + tolerance) {
        // 太远了，水平靠近（接近玩家）
        xdir = (dx > 0) ? static_cast<int>(moveSpeed) : -static_cast<int>(moveSpeed);
    } else {
        // X距离合适，保持不动
        xdir = 0;
    }

    // ========== Y方向：主动对齐玩家高度 ==========
    if (qAbs(dy) > yTolerance) {
        // Y轴不对齐，移动到玩家同一水平线
        ydir = (dy > 0) ? static_cast<int>(moveSpeed) : -static_cast<int>(moveSpeed);
    } else {
        // Y轴已对齐
        ydir = 0;
    }

    // 如果完全不动（距离和Y都合适），保持一个小的xdir用于朝向更新
    if (xdir == 0 && ydir == 0) {
        xdir = (dx > 0) ? 1 : -1;
    }

    // 更新朝向（始终面向玩家，水平方向）
    curr_xdir = (dx > 0) ? 1 : -1;
    curr_ydir = 0;
}

// 斜向移动模式，斜着接近玩家以躲避直线子弹
void Enemy::moveDiagonal() {
    if (!player)
        return;

    QPointF enemyPos = pos();
    QPointF playerPos = player->pos();

    double dx = playerPos.x() - enemyPos.x();
    double dy = playerPos.y() - enemyPos.y();
    double dist = qSqrt(dx * dx + dy * dy);

    if (dist < 0.1)
        return;

    // 计算主方向
    double dirX = dx / dist;
    double dirY = dy / dist;

    // 计算垂直方向
    double perpX = -dirY;
    double perpY = dirX;

    // 斜向移动：45度角接近玩家
    // 主方向分量 + 侧向分量，形成斜向移动
    double diagonalRatio = 0.7;  // 侧向移动比例

    double finalDirX = dirX + perpX * diagonalRatio * m_diagonalDirection;
    double finalDirY = dirY + perpY * diagonalRatio * m_diagonalDirection;

    // 归一化
    double finalDist = qSqrt(finalDirX * finalDirX + finalDirY * finalDirY);
    if (finalDist > 0.1) {
        xdir = static_cast<int>(qRound((finalDirX / finalDist) * 10));
        ydir = static_cast<int>(qRound((finalDirY / finalDist) * 10));
    }

    // 随机切换斜向方向（低概率）
    if (QRandomGenerator::global()->bounded(100) < 1) {
        m_diagonalDirection = -m_diagonalDirection;
    }

    // 碰到边界时切换方向
    QRectF pixmapRect = pixmap().rect();
    double newX = x() + xdir * speed / 10.0;
    double newY = y() + ydir * speed / 10.0;

    if (newX < 10 || newX + pixmapRect.width() > scene_bound_x - 10 ||
        newY < 10 || newY + pixmapRect.height() > scene_bound_y - 10) {
        m_diagonalDirection = -m_diagonalDirection;
    }

    // 更新朝向
    if (qAbs(dx) > qAbs(dy)) {
        curr_xdir = (dx > 0) ? 1 : -1;
        curr_ydir = 0;
    } else {
        curr_xdir = 0;
        curr_ydir = (dy > 0) ? 1 : -1;
    }
}

void Enemy::pauseTimers() {
    m_isPaused = true;
    if (aiTimer && aiTimer->isActive()) {
        aiTimer->stop();
    }
    if (moveTimer && moveTimer->isActive()) {
        moveTimer->stop();
    }
    if (attackTimer && attackTimer->isActive()) {
        attackTimer->stop();
    }
}

void Enemy::resumeTimers() {
    m_isPaused = false;
    if (aiTimer) {
        aiTimer->start(200);
    }
    if (moveTimer) {
        moveTimer->start(20);
    }
    if (attackTimer) {
        attackTimer->start(100);
    }
}
