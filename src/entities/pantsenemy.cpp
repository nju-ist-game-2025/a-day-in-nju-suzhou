#include "pantsenemy.h"
#include <QDateTime>
#include <QDebug>
#include <QGraphicsScene>
#include <QPainter>
#include <QTransform>
#include <QtMath>
#include "player.h"

PantsEnemy::PantsEnemy(const QPixmap &pic, double scale)
        : Enemy(pic, scale),
          m_isSpinning(false),
          m_spinningCooldownTimer(nullptr),
          m_spinningUpdateTimer(nullptr),
          m_spinningDurationTimer(nullptr),
          m_spinningCircle(nullptr),
          m_currentFrameIndex(0),
          m_lastSpinningDamageTime(0) {
    // 设置基础属性
    setHealth(20);           // 生命值
    setContactDamage(2);     // 普通接触伤害
    setVisionRange(300.0);   // 视野范围
    setAttackRange(50.0);    // 攻击范围
    setAttackCooldown(1000); // 攻击冷却
    setSpeed(2.5);           // 基础移速

    // 设置移动模式为 Z 字形
    setMovementPattern(MOVE_ZIGZAG);
    setZigzagAmplitude(70.0);

    // 保存原始图片和移速
    m_originalPixmap = pixmap();
    m_originalSpeed = speed;

    // 创建旋转动画帧
    createRotationFrames();

    // 创建技能冷却定时器（20秒）
    m_spinningCooldownTimer = new QTimer(this);
    m_spinningCooldownTimer->setInterval(SPINNING_COOLDOWN);
    connect(m_spinningCooldownTimer, &QTimer::timeout, this, &PantsEnemy::onSpinningTimer);

    // 创建旋转动画更新定时器
    m_spinningUpdateTimer = new QTimer(this);
    m_spinningUpdateTimer->setInterval(SPINNING_UPDATE_INTERVAL);
    connect(m_spinningUpdateTimer, &QTimer::timeout, this, &PantsEnemy::onSpinningUpdate);

    // 创建技能持续时间定时器（5秒，单次触发）
    m_spinningDurationTimer = new QTimer(this);
    m_spinningDurationTimer->setSingleShot(true);
    m_spinningDurationTimer->setInterval(SPINNING_DURATION);
    connect(m_spinningDurationTimer, &QTimer::timeout, this, &PantsEnemy::onSpinningEnd);

    // 开局立即释放一次技能
    QTimer::singleShot(500, this, &PantsEnemy::startSpinning);

    // 启动技能冷却定时器
    m_spinningCooldownTimer->start();
}

PantsEnemy::~PantsEnemy() {
    // 清理旋转圆
    if (m_spinningCircle && scene()) {
        scene()->removeItem(m_spinningCircle);
        delete m_spinningCircle;
        m_spinningCircle = nullptr;
    }

    // 停止所有定时器
    if (m_spinningCooldownTimer) {
        m_spinningCooldownTimer->stop();
        delete m_spinningCooldownTimer;
    }
    if (m_spinningUpdateTimer) {
        m_spinningUpdateTimer->stop();
        delete m_spinningUpdateTimer;
    }
    if (m_spinningDurationTimer) {
        m_spinningDurationTimer->stop();
        delete m_spinningDurationTimer;
    }
}

void PantsEnemy::createRotationFrames() {
    // 基于原始图片创建不同角度的旋转帧
    m_rotationFrames.clear();
    m_rotationFrames.reserve(ROTATION_FRAME_COUNT);

    for (int i = 0; i < ROTATION_FRAME_COUNT; ++i) {
        double angle = (360.0 / ROTATION_FRAME_COUNT) * i;
        QTransform transform;
        transform.rotate(angle);

        QPixmap rotatedPixmap = m_originalPixmap.transformed(transform, Qt::SmoothTransformation);

        // 裁剪到原始大小（旋转会增大图片尺寸）
        int originalW = m_originalPixmap.width();
        int originalH = m_originalPixmap.height();
        int newW = rotatedPixmap.width();
        int newH = rotatedPixmap.height();

        // 从中心裁剪
        int x = (newW - originalW) / 2;
        int y = (newH - originalH) / 2;
        QPixmap croppedPixmap = rotatedPixmap.copy(x, y, originalW, originalH);

        m_rotationFrames.append(croppedPixmap);
    }

    qDebug() << "PantsEnemy: 创建了" << m_rotationFrames.size() << "帧旋转动画";
}

void PantsEnemy::onSpinningTimer() {
    // 技能冷却结束，释放旋转技能
    if (!m_isSpinning && !m_isPaused) {
        startSpinning();
    }
}

void PantsEnemy::startSpinning() {
    if (m_isSpinning || !scene())
        return;

    m_isSpinning = true;
    m_currentFrameIndex = 0;

    qDebug() << "PantsEnemy: 开始释放旋转技能！";

    // 保存当前移速并大幅增加
    m_originalSpeed = speed;
    speed = m_originalSpeed * SPINNING_SPEED_MULTIPLIER;

    // 创建旋转伤害圆（浅灰色填充）
    double circleRadius = SPINNING_CIRCLE_RADIUS;
    m_spinningCircle = new QGraphicsEllipseItem(
            -circleRadius, -circleRadius,
            circleRadius * 2, circleRadius * 2);

    // 设置浅灰色半透明填充
    QColor fillColor(180, 180, 180, 120); // 浅灰色，半透明
    m_spinningCircle->setBrush(QBrush(fillColor));
    m_spinningCircle->setPen(QPen(QColor(150, 150, 150), 2)); // 边框
    m_spinningCircle->setZValue(zValue() - 1);                // 在怪物下方

    // 添加到场景并定位
    scene()->addItem(m_spinningCircle);
    updateSpinningCircle();

    // 启动旋转动画更新
    m_spinningUpdateTimer->start();

    // 启动技能持续时间计时
    m_spinningDurationTimer->start();
}

void PantsEnemy::stopSpinning() {
    if (!m_isSpinning)
        return;

    m_isSpinning = false;

    qDebug() << "PantsEnemy: 旋转技能结束";

    // 恢复原始移速
    speed = m_originalSpeed;

    // 停止旋转动画
    m_spinningUpdateTimer->stop();

    // 恢复原始图片（回到原来的角度）
    setPixmap(m_originalPixmap);

    // 移除旋转圆
    if (m_spinningCircle && scene()) {
        scene()->removeItem(m_spinningCircle);
        delete m_spinningCircle;
        m_spinningCircle = nullptr;
    }
}

void PantsEnemy::onSpinningUpdate() {
    if (!m_isSpinning)
        return;

    // 更新旋转动画帧
    m_currentFrameIndex = (m_currentFrameIndex + 1) % ROTATION_FRAME_COUNT;
    if (m_currentFrameIndex < m_rotationFrames.size()) {
        QGraphicsPixmapItem::setPixmap(m_rotationFrames[m_currentFrameIndex]);
    }

    // 更新旋转圆位置
    updateSpinningCircle();

    // 检测旋转伤害
    checkSpinningDamage();
}

void PantsEnemy::onSpinningEnd() {
    stopSpinning();
}

void PantsEnemy::updateSpinningCircle() {
    if (!m_spinningCircle)
        return;

    // 将圆心定位到怪物中心
    QRectF rect = boundingRect();
    QPointF center = pos() + QPointF(rect.width() / 2, rect.height() / 2);
    m_spinningCircle->setPos(center);
}

void PantsEnemy::checkSpinningDamage() {
    if (!m_isSpinning || !m_spinningCircle || !player || !scene())
        return;

    // 伤害间隔检测（避免连续伤害太快）
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - m_lastSpinningDamageTime < 500) // 0.5秒伤害间隔
        return;

    // 计算玩家中心和旋转圆圆心的距离
    QRectF playerRect = player->boundingRect();
    QPointF playerCenter = player->pos() + QPointF(playerRect.width() / 2, playerRect.height() / 2);

    QRectF myRect = boundingRect();
    QPointF myCenter = pos() + QPointF(myRect.width() / 2, myRect.height() / 2);

    double dx = playerCenter.x() - myCenter.x();
    double dy = playerCenter.y() - myCenter.y();
    double distance = qSqrt(dx * dx + dy * dy);

    // 如果玩家在旋转圆范围内
    if (distance <= SPINNING_CIRCLE_RADIUS) {
        player->takeDamage(SPINNING_DAMAGE);
        m_lastSpinningDamageTime = currentTime;
        qDebug() << "PantsEnemy: 旋转技能命中玩家，造成" << SPINNING_DAMAGE << "点伤害";
    }
}

void PantsEnemy::onContactWithPlayer(Player *p) {
    Q_UNUSED(p);
    // 普通接触伤害由基类处理，这里不需要额外效果
    // 旋转伤害由 checkSpinningDamage() 单独处理
}

void PantsEnemy::attackPlayer() {
    if (!player)
        return;

    if (m_isPaused)
        return;

    // 如果正在旋转，旋转伤害由 checkSpinningDamage 处理
    // 普通攻击仍然生效
    QList<QGraphicsItem *> collisions = collidingItems();
    for (QGraphicsItem *item: collisions) {
        Player *p = dynamic_cast<Player *>(item);
        if (p) {
            // 旋转状态下不造成普通接触伤害（旋转伤害更高且单独计算）
            if (!m_isSpinning) {
                p->takeDamage(contactDamage);
            }
            break;
        }
    }
}

void PantsEnemy::pauseTimers() {
    Enemy::pauseTimers();

    if (m_spinningCooldownTimer && m_spinningCooldownTimer->isActive()) {
        m_spinningCooldownTimer->stop();
    }
    if (m_spinningUpdateTimer && m_spinningUpdateTimer->isActive()) {
        m_spinningUpdateTimer->stop();
    }
    if (m_spinningDurationTimer && m_spinningDurationTimer->isActive()) {
        m_spinningDurationTimer->stop();
    }
}

void PantsEnemy::resumeTimers() {
    Enemy::resumeTimers();

    if (m_spinningCooldownTimer && !m_isSpinning) {
        m_spinningCooldownTimer->start();
    }
    if (m_isSpinning) {
        if (m_spinningUpdateTimer) {
            m_spinningUpdateTimer->start();
        }
        // 注意：m_spinningDurationTimer 是单次定时器，暂停后恢复比较复杂
        // 简化处理：暂停后恢复时如果正在旋转，继续旋转一小段时间
        if (m_spinningDurationTimer) {
            m_spinningDurationTimer->start(1000); // 恢复后再持续1秒
        }
    }
}
