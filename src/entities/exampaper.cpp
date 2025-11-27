#include "exampaper.h"
#include <QDebug>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include "../core/audiomanager.h"
#include "player.h"

ExamPaper::ExamPaper(QPointF startPos, QPointF direction, const QPixmap& pic, Player* player)
    : QObject(),
      QGraphicsPixmapItem(),
      m_player(player),
      m_moveTimer(nullptr),
      m_direction(direction),
      m_speed(6.0),  // 飞行速度
      m_rotation(0.0),
      m_rotationSpeed(15.0),  // 每帧旋转15度
      m_damage(1),            // 1点伤害
      m_stunDuration(2000),   // 2秒晕厥
      m_isPaused(false),
      m_isDestroying(false) {
    // 设置图片
    if (!pic.isNull()) {
        setPixmap(pic);
    } else {
        // 创建默认考卷图片
        QPixmap defaultPix(30, 40);
        defaultPix.fill(Qt::lightGray);
        setPixmap(defaultPix);
    }

    // 设置位置
    setPos(startPos);
    setZValue(80);

    // 设置旋转中心
    setTransformOriginPoint(boundingRect().center());

    // 启动移动定时器
    m_moveTimer = new QTimer(this);
    connect(m_moveTimer, &QTimer::timeout, this, &ExamPaper::onMoveTimer);
    m_moveTimer->start(16);  // 约60fps

    qDebug() << "[ExamPaper] 创建考卷，起始位置:" << startPos;
}

ExamPaper::~ExamPaper() {
    if (m_moveTimer) {
        m_moveTimer->stop();
        delete m_moveTimer;
        m_moveTimer = nullptr;
    }
    qDebug() << "[ExamPaper] 考卷销毁";
}

void ExamPaper::onMoveTimer() {
    if (m_isPaused || m_isDestroying)
        return;

    // 移动
    QPointF currentPos = pos();
    QPointF newPos = currentPos + m_direction * m_speed;
    setPos(newPos);

    // 旋转
    m_rotation += m_rotationSpeed;
    if (m_rotation >= 360.0) {
        m_rotation -= 360.0;
    }
    setRotation(m_rotation);

    // 检测碰撞
    checkCollision();

    // 检查是否飞出场景
    if (newPos.x() < -50 || newPos.x() > 850 ||
        newPos.y() < -50 || newPos.y() > 650) {
        destroy();
    }
}

void ExamPaper::checkCollision() {
    if (!m_player || m_isDestroying)
        return;

    // 检测与玩家的碰撞
    QList<QGraphicsItem*> collisions = collidingItems();
    for (QGraphicsItem* item : collisions) {
        Player* p = dynamic_cast<Player*>(item);
        if (p) {
            // 击中玩家
            p->takeDamage(m_damage);
            applyStunEffect();
            destroy();
            break;
        }
    }
}

void ExamPaper::applyStunEffect() {
    if (!m_player || !scene())
        return;

    // 检查玩家是否已经处于晕厥状态
    if (!m_player->canMove())
        return;

    // 检查效果冷却
    if (m_player->isEffectOnCooldown())
        return;

    qDebug() << "[ExamPaper] 考卷击中玩家！触发晕厥效果，持续" << m_stunDuration << "ms";

    // 设置效果冷却
    m_player->setEffectCooldown(true);

    // 显示"晕厥"文字提示
    QGraphicsTextItem* stunText = new QGraphicsTextItem("晕厥!");
    QFont font;
    font.setPointSize(16);
    font.setBold(true);
    stunText->setFont(font);
    stunText->setDefaultTextColor(QColor(255, 165, 0));  // 橙色
    stunText->setPos(m_player->pos().x(), m_player->pos().y() - 40);
    stunText->setZValue(200);
    scene()->addItem(stunText);

    // 禁用玩家移动
    m_player->setCanMove(false);

    // 保存玩家指针（用于lambda）
    QPointer<Player> playerPtr = m_player;
    QGraphicsScene* currentScene = scene();

    // 晕厥结束后恢复移动
    QTimer::singleShot(m_stunDuration, [playerPtr, stunText, currentScene]() {
        if (playerPtr) {
            playerPtr->setCanMove(true);
            qDebug() << "[ExamPaper] 晕厥效果结束，玩家恢复移动";

            // 3秒后解除冷却
            QTimer::singleShot(3000, [playerPtr]() {
                if (playerPtr) {
                    playerPtr->setEffectCooldown(false);
                }
            });
        }

        // 删除文字
        if (stunText && currentScene) {
            currentScene->removeItem(stunText);
            delete stunText;
        }
    });
}

void ExamPaper::setPaused(bool paused) {
    m_isPaused = paused;
    if (m_moveTimer) {
        if (paused) {
            m_moveTimer->stop();
        } else {
            m_moveTimer->start();
        }
    }
}

void ExamPaper::destroy() {
    if (m_isDestroying)
        return;
    m_isDestroying = true;

    if (m_moveTimer) {
        m_moveTimer->stop();
    }

    if (scene()) {
        scene()->removeItem(this);
    }
    deleteLater();
}
