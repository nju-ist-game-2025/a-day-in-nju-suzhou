#include "scalingenemy.h"
#include <QDebug>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPointer>
#include <QRandomGenerator>
#include <QTimer>
#include "player.h"

ScalingEnemy::ScalingEnemy(const QPixmap& pic, double scale)
    : Enemy(pic, scale),
      m_scalingTimer(nullptr),
      m_baseScale(scale),
      m_minScale(3.0),  // 相对于baseScale）
      m_maxScale(9.0),  //（相对于baseScale，确保高分辨率图片时也有足够大的最大尺寸）
      m_currentScale(1.0),
      m_scaleSpeed(0.04),  // 每次缩放4%
      m_scalingUp(true),
      m_originalPixmap(pic) {
    // 设置移动模式为绕圈移动
    setMovementPattern(MOVE_CIRCLE);

    // 设置绕圈参数
    setCircleRadius(180.0);  // 绕圈半径
    setSpeed(2.5);           // 移动速度
    setHealth(25);           // 生命值
    setContactDamage(3);     // 接触伤害

    // 创建缩放定时器
    m_scalingTimer = new QTimer(this);
    connect(m_scalingTimer, &QTimer::timeout, this, &ScalingEnemy::updateScaling);
    m_scalingTimer->start(50);  // 每50ms更新一次缩放
}

ScalingEnemy::~ScalingEnemy() {
    if (m_scalingTimer) {
        m_scalingTimer->stop();
        delete m_scalingTimer;
        m_scalingTimer = nullptr;
    }
}

void ScalingEnemy::pauseTimers() {
    Enemy::pauseTimers();
    if (m_scalingTimer) {
        m_scalingTimer->stop();
    }
}

void ScalingEnemy::resumeTimers() {
    Enemy::resumeTimers();
    if (m_scalingTimer) {
        m_scalingTimer->start(50);
    }
}

void ScalingEnemy::updateScaling() {
    // 更新缩放比例
    if (m_scalingUp) {
        m_currentScale += m_scaleSpeed;
        if (m_currentScale >= m_maxScale) {
            m_currentScale = m_maxScale;
            m_scalingUp = false;
        }
    } else {
        m_currentScale -= m_scaleSpeed;
        if (m_currentScale <= m_minScale) {
            m_currentScale = m_minScale;
            m_scalingUp = true;
        }
    }

    // 应用缩放 - 使用Qt的setScale来缩放整个图形项
    // 这会同时影响视觉大小和碰撞判定范围
    setScale(m_baseScale * m_currentScale);
}

void ScalingEnemy::onContactWithPlayer(Player* p) {
    Q_UNUSED(p);
    // 接触时50%概率触发昏睡效果
    if (QRandomGenerator::global()->bounded(100) < 50) {
        applySleepEffect();
    }
}

void ScalingEnemy::attackPlayer() {
    if (!player)
        return;

    // 暂停状态下不攻击
    if (m_isPaused)
        return;

    // 近战攻击：使用像素级碰撞检测
    if (Entity::pixelCollision(this, player)) {
        player->takeDamage(contactDamage);

        // 50%概率触发昏睡效果
        if (QRandomGenerator::global()->bounded(100) < 50) {
            applySleepEffect();
        }
    }
}

void ScalingEnemy::applySleepEffect() {
    if (!player || !scene())
        return;

    // 检查玩家是否已经处于昏睡状态（不可叠加）
    if (!player->canMove())
        return;

    // 检查效果冷却
    if (player->isEffectOnCooldown())
        return;

    qDebug() << "ScalingEnemy触发昏睡效果！玩家无法移动1.5秒（50%概率触发）";

    // 立即设置效果冷却，防止重复触发
    player->setEffectCooldown(true);

    // 显示"昏睡ZZZ"文字提示
    QGraphicsTextItem* sleepText = new QGraphicsTextItem("昏睡ZZZ");
    QFont font;
    font.setPointSize(16);
    font.setBold(true);
    sleepText->setFont(font);
    sleepText->setDefaultTextColor(QColor(128, 128, 128));  // 灰色
    sleepText->setPos(player->pos().x(), player->pos().y() - 40);
    sleepText->setZValue(200);
    scene()->addItem(sleepText);

    // 禁用玩家移动
    player->setCanMove(false);

    // 使用QPointer保护player指针
    QPointer<Player> playerPtr = player;

    // 1.5秒后恢复移动并删除文字
    QTimer::singleShot(1500, [playerPtr, sleepText]() {
        if (playerPtr) {
            playerPtr->setCanMove(true);
            qDebug() << "昏睡效果结束，玩家恢复移动，3秒后可再次触发";
            // 效果结束后3秒再解除冷却
            QTimer::singleShot(3000, [playerPtr]() {
                if (playerPtr) {
                    playerPtr->setEffectCooldown(false);
                }
            });
        }
        if (sleepText) {
            if (sleepText->scene()) {
                sleepText->scene()->removeItem(sleepText);
            }
            delete sleepText;
        } });
}
