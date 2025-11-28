#include "invigilator.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QtMath>
#include "../core/configmanager.h"
#include "player.h"
#include "teacherboss.h"

Invigilator::Invigilator(const QPixmap& normalPic, const QPixmap& angryPic, TeacherBoss* master, double scale)
    : Enemy(normalPic, scale),
      m_master(master),
      m_state(PATROL),
      m_normalPixmap(normalPic),
      m_angryPixmap(angryPic),
      m_patrolAngle(0.0),
      m_patrolRadius(100.0),
      m_patrolSpeed(0.03),
      m_patrolTimer(nullptr),
      m_detectionRange(150.0) {
    // 从配置文件读取监考员属性
    ConfigManager& config = ConfigManager::instance();
    setHealth(config.getEnemyInt("invigilator", "health", 15));
    setContactDamage(config.getEnemyInt("invigilator", "contact_damage", 1));
    setVisionRange(config.getEnemyDouble("invigilator", "vision_range", 200));
    setAttackRange(config.getEnemyDouble("invigilator", "attack_range", 30));
    setSpeed(config.getEnemyDouble("invigilator", "speed", 2.0));
    m_patrolRadius = config.getEnemyDouble("invigilator", "patrol_radius", 100.0);
    m_detectionRange = config.getEnemyDouble("invigilator", "detection_range", 150.0);

    // 标记为被召唤的敌人
    setIsSummoned(true);

    // 停止默认AI，使用自定义巡逻
    if (aiTimer)
        aiTimer->stop();
    if (moveTimer)
        moveTimer->stop();

    // 创建巡逻定时器
    m_patrolTimer = new QTimer(this);
    connect(m_patrolTimer, &QTimer::timeout, this, &Invigilator::onPatrolTimer);
    m_patrolTimer->start(30);  // 约33fps

    qDebug() << "[Invigilator] 监考员创建";
}

Invigilator::~Invigilator() {
    if (m_patrolTimer) {
        m_patrolTimer->stop();
        delete m_patrolTimer;
        m_patrolTimer = nullptr;
    }
    qDebug() << "[Invigilator] 监考员销毁";
}

void Invigilator::onPatrolTimer() {
    if (m_isPaused)
        return;

    // 检测玩家
    checkPlayerDetection();

    // 根据状态移动
    if (m_state == PATROL) {
        updatePatrol();
    } else {
        // 追击状态使用默认的Enemy::move
        move();
    }
}

void Invigilator::checkPlayerDetection() {
    if (!player || m_state == CHASE)
        return;

    // 计算与玩家的距离
    QPointF myCenter = pos() + QPointF(boundingRect().width() / 2, boundingRect().height() / 2);
    QPointF playerCenter = player->pos() + QPointF(player->boundingRect().width() / 2,
                                                   player->boundingRect().height() / 2);

    double dx = playerCenter.x() - myCenter.x();
    double dy = playerCenter.y() - myCenter.y();
    double distance = qSqrt(dx * dx + dy * dy);

    // 如果玩家在检测范围内，切换到追击状态
    if (distance < m_detectionRange) {
        switchToChase();
    }
}

void Invigilator::switchToChase() {
    m_state = CHASE;

    // 切换到愤怒图片
    if (!m_angryPixmap.isNull()) {
        setPixmap(m_angryPixmap);
    }

    // 设置为冲刺追击模式
    setMovementPattern(MOVE_DASH);
    setDashSpeed(5.0);
    setDashChargeTime(300);
    setVisionRange(1000);  // 扩大视野

    // 恢复默认AI
    if (aiTimer)
        aiTimer->start();
    if (moveTimer)
        moveTimer->start();

    qDebug() << "[Invigilator] 监考员发现玩家，进入追击状态！";
}

void Invigilator::updatePatrol() {
    if (!m_master)
        return;

    // 更新巡逻角度
    m_patrolAngle += m_patrolSpeed;
    if (m_patrolAngle >= 2 * M_PI) {
        m_patrolAngle -= 2 * M_PI;
    }

    // 计算围绕Boss的位置
    QPointF masterPos = m_master->pos();
    QPointF masterCenter = masterPos + QPointF(m_master->boundingRect().width() / 2,
                                               m_master->boundingRect().height() / 2);

    double newX = masterCenter.x() + m_patrolRadius * qCos(m_patrolAngle) - boundingRect().width() / 2;
    double newY = masterCenter.y() + m_patrolRadius * qSin(m_patrolAngle) - boundingRect().height() / 2;

    // 限制在场景内
    newX = qBound(0.0, newX, 800.0 - boundingRect().width());
    newY = qBound(0.0, newY, 600.0 - boundingRect().height());

    setPos(newX, newY);
}

void Invigilator::move() {
    if (m_isPaused)
        return;

    if (m_state == PATROL) {
        // 巡逻模式由定时器处理
        return;
    }

    // 追击模式使用父类移动
    Enemy::move();
}

void Invigilator::pauseTimers() {
    Enemy::pauseTimers();
    if (m_patrolTimer)
        m_patrolTimer->stop();
}

void Invigilator::resumeTimers() {
    Enemy::resumeTimers();
    if (m_patrolTimer && m_state == PATROL) {
        m_patrolTimer->start();
    }
}
