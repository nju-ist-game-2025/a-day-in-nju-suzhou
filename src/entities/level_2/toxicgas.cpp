#include "toxicgas.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QtMath>
#include "../../items/statuseffect.h"
#include "../player.h"

ToxicGas::ToxicGas(QPointF startPos, QPointF direction, const QPixmap& pic, Player* player)
    : QGraphicsPixmapItem(pic),
      m_player(player),
      m_moveTimer(nullptr),
      m_effectTimer(nullptr),
      m_despawnTimer(nullptr),
      m_direction(direction),
      m_speed(4.0),
      m_isStationary(false),
      m_isPaused(false),
      m_isDestroying(false),
      m_damagePerTick(1),
      m_slowFactor(0.5) {
    setPos(startPos);
    setZValue(50);  // 在敌人之上，UI之下

    // 归一化方向向量
    double length = qSqrt(m_direction.x() * m_direction.x() + m_direction.y() * m_direction.y());
    if (length > 0.1) {
        m_direction /= length;
    }

    // 移动定时器
    m_moveTimer = new QTimer(this);
    connect(m_moveTimer, &QTimer::timeout, this, &ToxicGas::onMoveTimer);
    m_moveTimer->start(16);  // 约60fps

    // 持续伤害定时器（碰到玩家后启动）
    m_effectTimer = new QTimer(this);
    connect(m_effectTimer, &QTimer::timeout, this, &ToxicGas::onEffectTimer);

    // 消失定时器（碰到玩家后启动，10秒后消失）
    m_despawnTimer = new QTimer(this);
    m_despawnTimer->setSingleShot(true);
    connect(m_despawnTimer, &QTimer::timeout, this, &ToxicGas::onDespawnTimer);

    qDebug() << "ToxicGas created at" << startPos << "direction:" << m_direction;
}

ToxicGas::~ToxicGas() {
    if (m_moveTimer) {
        m_moveTimer->stop();
    }
    if (m_effectTimer) {
        m_effectTimer->stop();
    }
    if (m_despawnTimer) {
        m_despawnTimer->stop();
    }
    qDebug() << "ToxicGas destroyed";
}

void ToxicGas::onMoveTimer() {
    if (m_isPaused || m_isStationary || m_isDestroying)
        return;

    // 移动
    QPointF newPos = pos() + m_direction * m_speed;
    setPos(newPos);

    // 检查是否超出边界
    if (newPos.x() < -50 || newPos.x() > 850 || newPos.y() < -50 || newPos.y() > 650) {
        // 飞出场外，销毁
        onDespawnTimer();
        return;
    }

    // 检查碰撞
    checkCollision();
}

void ToxicGas::checkCollision() {
    if (!m_player || m_isStationary || m_isDestroying)
        return;

    // 检查与玩家的碰撞
    QPointF gasCenter = pos() + QPointF(pixmap().width() / 2.0, pixmap().height() / 2.0);
    QPointF playerCenter = m_player->pos() + QPointF(30, 30);  // 假设玩家中心偏移

    double dx = gasCenter.x() - playerCenter.x();
    double dy = gasCenter.y() - playerCenter.y();
    double dist = qSqrt(dx * dx + dy * dy);

    // 碰撞检测半径
    double collisionRadius = 40.0;

    if (dist < collisionRadius) {
        stopMoving();
    }
}

void ToxicGas::stopMoving() {
    if (m_isStationary)
        return;

    m_isStationary = true;
    qDebug() << "ToxicGas hit player, stopping and applying effects";

    // 停止移动
    if (m_moveTimer) {
        m_moveTimer->stop();
    }

    // 立即应用第一次伤害和减速
    applyDamageAndSlow();

    // 启动持续伤害定时器（每秒一次）
    m_effectTimer->start(1000);

    // 启动10秒后消失的定时器
    m_despawnTimer->start(10000);
}

void ToxicGas::onEffectTimer() {
    if (m_isPaused || m_isDestroying)
        return;

    applyDamageAndSlow();
}

void ToxicGas::applyDamageAndSlow() {
    if (!m_player || m_isDestroying)
        return;

    // 如果玩家处于无敌状态（如被吸纳），不造成伤害和减速
    if (m_player->isInvincible()) {
        return;
    }

    // 检查玩家是否仍在毒气范围内
    QPointF gasCenter = pos() + QPointF(pixmap().width() / 2.0, pixmap().height() / 2.0);
    QPointF playerCenter = m_player->pos() + QPointF(30, 30);

    double dx = gasCenter.x() - playerCenter.x();
    double dy = gasCenter.y() - playerCenter.y();
    double dist = qSqrt(dx * dx + dy * dy);

    // 效果范围（略大于碰撞范围）
    double effectRadius = 60.0;

    if (dist < effectRadius) {
        // 造成伤害
        m_player->takeDamage(m_damagePerTick);

        // 应用减速效果（短时间，会被持续刷新），最多叠加2层
        if (m_player->canApplySlowStack(2)) {
            m_player->addSlowStack();

            SpeedEffect* slowEffect = new SpeedEffect(2, m_slowFactor);

            // 连接效果结束信号，减少叠加层数
            QPointer<Player> playerPtr = m_player;
            QObject::connect(slowEffect, &QObject::destroyed, [playerPtr]() {
                if (playerPtr) {
                    playerPtr->removeSlowStack();
                }
            });

            slowEffect->applyTo(m_player);
        }

        qDebug() << "ToxicGas applied damage and slow to player";
    }
}

void ToxicGas::onDespawnTimer() {
    if (m_isDestroying)
        return;

    m_isDestroying = true;

    // 停止所有定时器
    if (m_moveTimer)
        m_moveTimer->stop();
    if (m_effectTimer)
        m_effectTimer->stop();
    if (m_despawnTimer)
        m_despawnTimer->stop();

    // 从场景移除
    if (scene()) {
        scene()->removeItem(this);
    }

    deleteLater();
}

void ToxicGas::setPaused(bool paused) {
    m_isPaused = paused;
}
