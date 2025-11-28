#include "yanglinenemy.h"
#include <QDateTime>
#include <QDebug>
#include <QGraphicsScene>
#include <QPainter>
#include <QTransform>
#include <QtMath>
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../ui/explosion.h"
#include "player.h"

YanglinEnemy::YanglinEnemy(const QPixmap& pic, double scale)
    : ScalingEnemy(pic, scale),
      m_isSpinning(false),
      m_spinningCooldownTimer(nullptr),
      m_spinningUpdateTimer(nullptr),
      m_spinningDurationTimer(nullptr),
      m_firstSpinningTimer(nullptr),
      m_rotationAngle(0.0),
      m_originalSpeed(2.0),
      m_isReturningToNormal(false),
      m_lastSpinningDamageTime(0) {
    // 从配置文件读取杨林属性
    ConfigManager& config = ConfigManager::instance();
    health = config.getEnemyInt("yanglin", "health", 200);
    maxHealth = health;
    contactDamage = config.getEnemyInt("yanglin", "contact_damage", 5);
    visionRange = config.getEnemyDouble("yanglin", "vision_range", 9999.0);
    attackRange = config.getEnemyDouble("yanglin", "attack_range", 60.0);
    attackCooldown = config.getEnemyInt("yanglin", "attack_cooldown", 800);
    speed = config.getEnemyDouble("yanglin", "speed", 2.0);
    m_originalSpeed = speed;

    // 确保变换原点在中心（继承自ScalingEnemy，但重新设置确保正确）
    setTransformOriginPoint(pixmap().width() / 2.0, pixmap().height() / 2.0);

    // 创建技能冷却定时器（30秒）
    m_spinningCooldownTimer = new QTimer(this);
    m_spinningCooldownTimer->setInterval(SPINNING_COOLDOWN);
    connect(m_spinningCooldownTimer, &QTimer::timeout, this, &YanglinEnemy::onSpinningTimer);

    // 创建旋转动画更新定时器
    m_spinningUpdateTimer = new QTimer(this);
    m_spinningUpdateTimer->setInterval(SPINNING_UPDATE_INTERVAL);
    connect(m_spinningUpdateTimer, &QTimer::timeout, this, &YanglinEnemy::onSpinningUpdate);

    // 创建技能持续时间定时器（5秒，单次触发）
    m_spinningDurationTimer = new QTimer(this);
    m_spinningDurationTimer->setSingleShot(true);
    m_spinningDurationTimer->setInterval(SPINNING_DURATION);
    connect(m_spinningDurationTimer, &QTimer::timeout, this, &YanglinEnemy::onSpinningEnd);

    // 开局10秒后释放第一次技能
    m_firstSpinningTimer = new QTimer(this);
    m_firstSpinningTimer->setSingleShot(true);
    m_firstSpinningTimer->setInterval(FIRST_SPINNING_DELAY);
    connect(m_firstSpinningTimer, &QTimer::timeout, this, &YanglinEnemy::onFirstSpinning);
    m_firstSpinningTimer->start();

    qDebug() << "创建杨林精英怪 - 血量:" << health << "接触伤害:" << contactDamage
             << "旋转伤害:" << SPINNING_DAMAGE << "基础旋转半径:" << BASE_SPINNING_RADIUS
             << "全图视野:" << visionRange << "当前缩放:" << getTotalScale();
}

YanglinEnemy::~YanglinEnemy() {
    if (m_spinningCooldownTimer) {
        m_spinningCooldownTimer->stop();
        delete m_spinningCooldownTimer;
        m_spinningCooldownTimer = nullptr;
    }
    if (m_spinningUpdateTimer) {
        m_spinningUpdateTimer->stop();
        delete m_spinningUpdateTimer;
        m_spinningUpdateTimer = nullptr;
    }
    if (m_spinningDurationTimer) {
        m_spinningDurationTimer->stop();
        delete m_spinningDurationTimer;
        m_spinningDurationTimer = nullptr;
    }
    if (m_firstSpinningTimer) {
        m_firstSpinningTimer->stop();
        delete m_firstSpinningTimer;
        m_firstSpinningTimer = nullptr;
    }
}

void YanglinEnemy::onFirstSpinning() {
    // 开局10秒后释放第一次技能
    qDebug() << "YanglinEnemy: 开局10秒，释放第一次旋转技能！";
    startSpinning();

    // 启动30秒冷却定时器
    m_spinningCooldownTimer->start();
}

void YanglinEnemy::onSpinningTimer() {
    // 每30秒触发
    if (!m_isSpinning && !m_isPaused) {
        startSpinning();
    }
}

void YanglinEnemy::startSpinning() {
    if (m_isSpinning || !scene())
        return;

    m_isSpinning = true;
    m_rotationAngle = 0.0;

    qDebug() << "YanglinEnemy: 开始释放旋转技能！（无可视圆，持续5秒）当前缩放:" << getTotalScale();

    // 保存当前移速并增加（与pants类似）
    m_originalSpeed = speed;
    speed = m_originalSpeed * SPINNING_SPEED_MULTIPLIER;

    // 启动旋转动画更新
    m_spinningUpdateTimer->start();

    // 启动技能持续时间计时（5秒）
    m_spinningDurationTimer->start();
}

void YanglinEnemy::stopSpinning() {
    if (!m_isSpinning)
        return;

    m_isSpinning = false;

    qDebug() << "YanglinEnemy: 旋转技能结束，当前角度:" << m_rotationAngle;

    // 恢复原始移速
    speed = m_originalSpeed;

    // 停止旋转动画
    m_spinningUpdateTimer->stop();

    // 平滑回转到0度（计算最短路径）
    // 如果当前角度大于180度，继续正转到360度（即0度）
    // 如果当前角度小于等于180度，反转回0度
    if (m_rotationAngle > 0) {
        // 启动回正动画
        m_isReturningToNormal = true;
        m_spinningUpdateTimer->start();  // 复用定时器来做回正动画
    }
}

void YanglinEnemy::onSpinningUpdate() {
    // 处理回正动画
    if (m_isReturningToNormal) {
        // 计算回正方向（选择最短路径）
        if (m_rotationAngle > 180.0) {
            // 继续正转到360度
            m_rotationAngle += ROTATION_SPEED;
            if (m_rotationAngle >= 360.0) {
                m_rotationAngle = 0.0;
                m_isReturningToNormal = false;
                m_spinningUpdateTimer->stop();
                qDebug() << "YanglinEnemy: 回正完成";
            }
        } else {
            // 反转回0度
            m_rotationAngle -= ROTATION_SPEED;
            if (m_rotationAngle <= 0.0) {
                m_rotationAngle = 0.0;
                m_isReturningToNormal = false;
                m_spinningUpdateTimer->stop();
                qDebug() << "YanglinEnemy: 回正完成";
            }
        }
        setRotation(m_rotationAngle);
        return;
    }

    if (!m_isSpinning)
        return;

    // 更新旋转角度（使用setRotation而不是setPixmap，保持缩放效果）
    m_rotationAngle += ROTATION_SPEED;
    if (m_rotationAngle >= 360.0) {
        m_rotationAngle -= 360.0;
    }
    setRotation(m_rotationAngle);

    // 检测旋转伤害
    checkSpinningDamage();
}

void YanglinEnemy::onSpinningEnd() {
    stopSpinning();
}

double YanglinEnemy::getCurrentSpinningRadius() const {
    // 旋转伤害半径随缩放变化
    // getTotalScale() 返回 m_baseScale * m_currentScale
    return BASE_SPINNING_RADIUS * getTotalScale();
}

void YanglinEnemy::checkSpinningDamage() {
    if (!m_isSpinning || !player || !scene())
        return;

    // 伤害间隔检测（0.5秒，与pants相同）
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - m_lastSpinningDamageTime < 500)
        return;

    // 计算玩家中心
    QRectF playerRect = player->boundingRect();
    QPointF playerCenter = player->pos() + QPointF(playerRect.width() / 2, playerRect.height() / 2);

    // 计算杨林中心（考虑缩放）
    QRectF myRect = boundingRect();
    double currentScale = getTotalScale();
    QPointF myCenter = pos() + QPointF(myRect.width() * currentScale / 2, myRect.height() * currentScale / 2);

    double dx = playerCenter.x() - myCenter.x();
    double dy = playerCenter.y() - myCenter.y();
    double distance = qSqrt(dx * dx + dy * dy);

    // 获取当前缩放下的旋转伤害半径
    double currentRadius = getCurrentSpinningRadius();

    // 如果玩家在旋转圆范围内
    if (distance <= currentRadius) {
        player->takeDamage(SPINNING_DAMAGE);
        m_lastSpinningDamageTime = currentTime;
        qDebug() << "YanglinEnemy: 旋转技能命中玩家，造成" << SPINNING_DAMAGE
                 << "点伤害，当前半径:" << currentRadius << "当前缩放:" << currentScale;
    }
}

void YanglinEnemy::takeDamage(int damage) {
    int realDamage = qMax(1, damage);
    if (health - realDamage <= 0) {
        // 即将死亡，停止所有技能
        m_isSpinning = false;
        if (m_spinningUpdateTimer)
            m_spinningUpdateTimer->stop();
        if (m_spinningCooldownTimer)
            m_spinningCooldownTimer->stop();
        if (m_spinningDurationTimer)
            m_spinningDurationTimer->stop();
        if (m_firstSpinningTimer)
            m_firstSpinningTimer->stop();
    }

    // 调用父类的takeDamage（ScalingEnemy会处理闪烁和死亡）
    ScalingEnemy::takeDamage(damage);
}

void YanglinEnemy::attackPlayer() {
    if (!player)
        return;

    if (m_isPaused)
        return;

    // 旋转状态下不造成普通接触伤害（旋转伤害单独计算，与pants相同）
    if (m_isSpinning)
        return;

    // 使用像素级碰撞检测
    if (Entity::pixelCollision(this, player)) {
        player->takeDamage(contactDamage);
    }
}

void YanglinEnemy::pauseTimers() {
    ScalingEnemy::pauseTimers();

    if (m_spinningCooldownTimer && m_spinningCooldownTimer->isActive()) {
        m_spinningCooldownTimer->stop();
    }
    if (m_spinningUpdateTimer && m_spinningUpdateTimer->isActive()) {
        m_spinningUpdateTimer->stop();
    }
    if (m_spinningDurationTimer && m_spinningDurationTimer->isActive()) {
        m_spinningDurationTimer->stop();
    }
    if (m_firstSpinningTimer && m_firstSpinningTimer->isActive()) {
        m_firstSpinningTimer->stop();
    }
}

void YanglinEnemy::resumeTimers() {
    ScalingEnemy::resumeTimers();

    if (m_spinningCooldownTimer && !m_isSpinning) {
        m_spinningCooldownTimer->start();
    }
    if (m_isSpinning) {
        if (m_spinningUpdateTimer) {
            m_spinningUpdateTimer->start();
        }
        if (m_spinningDurationTimer) {
            m_spinningDurationTimer->start(1000);  // 恢复后再持续1秒
        }
    }
}
