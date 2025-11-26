#include "orbitingsock.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QtMath>
#include "washmachineboss.h"

OrbitingSock::OrbitingSock(const QPixmap &pic, WashMachineBoss *master, double scale)
        : SockEnemy(pic, scale),
          m_master(master),
          m_orbitAngle(0.0),
          m_orbitRadius(100.0),
          m_orbitSpeed(0.05),  // 约2秒一圈 (2*PI / (2000ms / 16ms) ≈ 0.05)
          m_orbitTimer(nullptr) {
    // 设置臭袜子属性
    setHealth(15);
    setContactDamage(2);
    setVisionRange(300.0);
    setAttackRange(40.0);
    setAttackCooldown(800);
    setSpeed(0);  // 不需要主动移动，靠轨道运动

    // 标记为召唤的敌人，不触发bonus
    setIsSummoned(true);

    // 停止父类的AI定时器，我们使用自己的轨道定时器
    if (aiTimer) {
        aiTimer->stop();
    }
    if (moveTimer) {
        moveTimer->stop();
    }

    // 创建轨道更新定时器
    m_orbitTimer = new QTimer(this);
    connect(m_orbitTimer, &QTimer::timeout, this, &OrbitingSock::updateOrbit);
    m_orbitTimer->start(16);  // 约60fps

    qDebug() << "OrbitingSock created, orbiting around WashMachineBoss";
}

OrbitingSock::~OrbitingSock() {
    if (m_orbitTimer) {
        m_orbitTimer->stop();
        delete m_orbitTimer;
        m_orbitTimer = nullptr;
    }
    qDebug() << "OrbitingSock destroyed";
}

void OrbitingSock::updateOrbit() {
    if (m_isPaused)
        return;

    if (!m_master || !m_master->scene()) {
        // Boss已死亡或不在场景中，销毁自己
        if (scene()) {
            emit dying(this);
            scene()->removeItem(this);
            deleteLater();
        }
        return;
    }

    // 更新轨道角度
    m_orbitAngle += m_orbitSpeed;
    if (m_orbitAngle > 2 * M_PI) {
        m_orbitAngle -= 2 * M_PI;
    }

    // 计算新位置（围绕Boss中心）
    QPointF bossCenter = m_master->pos() + QPointF(m_master->pixmap().width() / 2.0,
                                                   m_master->pixmap().height() / 2.0);

    double newX = bossCenter.x() + m_orbitRadius * qCos(m_orbitAngle) - pixmap().width() / 2.0;
    double newY = bossCenter.y() + m_orbitRadius * qSin(m_orbitAngle) - pixmap().height() / 2.0;

    setPos(newX, newY);

    // 更新朝向（始终面向运动方向）
    curr_xdir = (qCos(m_orbitAngle + M_PI / 2) > 0) ? 1 : -1;
    curr_ydir = (qSin(m_orbitAngle + M_PI / 2) > 0) ? 1 : -1;
}

void OrbitingSock::move() {
    // 轨道袜子不使用常规移动，由 updateOrbit 处理
    // 但仍需检查攻击
    if (m_isPaused)
        return;

    // 攻击检测仍由父类的 attackTimer 处理
}

void OrbitingSock::pauseTimers() {
    SockEnemy::pauseTimers();
    if (m_orbitTimer && m_orbitTimer->isActive()) {
        m_orbitTimer->stop();
    }
}

void OrbitingSock::resumeTimers() {
    SockEnemy::resumeTimers();
    if (m_orbitTimer) {
        m_orbitTimer->start(16);
    }
}
