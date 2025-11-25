#include "nightmareboss.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QRandomGenerator>
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
#include "player.h"

NightmareBoss::NightmareBoss(const QPixmap& pic, double scale)
    : Boss(pic, scale),
      m_phase(1),
      m_isTransitioning(false),
      m_nightmareWrapTimer(nullptr),
      m_nightmareDescentTimer(nullptr),
      m_firstDescentTriggered(false) {
    // Nightmare Boss 一阶段属性
    setHealth(350);           // Boss血量
    setContactDamage(6);      // 接触伤害
    setVisionRange(400);      // 视野范围
    setAttackRange(70);       // 攻击判定范围
    setAttackCooldown(1200);  // 攻击冷却
    setSpeed(1.0);            // 移动速度

    crash_r = 35;       // 实际攻击范围
    damageScale = 0.7;  // 伤害减免

    // 一阶段：使用单段冲刺模式
    setMovementPattern(MOVE_DASH);
    setDashChargeTime(1000);  // 1秒蓄力
    setDashSpeed(6.0);        // 高速冲刺

    // 预加载二阶段图片（使用config中的boss尺寸）
    int bossSize = ConfigManager::instance().getSize("boss");
    m_phase2Pixmap = ResourceFactory::createBossImage(bossSize, 1, "nightmare2");

    qDebug() << "Nightmare Boss创建，一阶段模式，使用单段冲刺";
}

NightmareBoss::~NightmareBoss() {
    if (m_nightmareWrapTimer) {
        m_nightmareWrapTimer->stop();
        delete m_nightmareWrapTimer;
    }
    if (m_nightmareDescentTimer) {
        m_nightmareDescentTimer->stop();
        delete m_nightmareDescentTimer;
    }
    qDebug() << "Nightmare Boss被摧毁";
}

void NightmareBoss::takeDamage(int damage) {
    if (m_isTransitioning) {
        return;  // 转换期间无敌
    }

    // 触发闪烁效果
    flash();

    // 调用父类的伤害处理
    health -= static_cast<int>(damage * damageScale);

    if (health <= 0 && m_phase == 1) {
        // 一阶段死亡 - 触发亡语
        m_isTransitioning = true;
        health = 1;  // 保持存活，不触发dying信号

        qDebug() << "Nightmare Boss 一阶段死亡，触发亡语！";

        // 击杀场上所有小怪
        killAllEnemies();

        // 对玩家造成2点伤害
        if (player) {
            player->takeDamage(2);
            qDebug() << "亡语对玩家造成2点伤害";
        }

        // 发送信号显示遮罩和文字
        emit requestShowShadowOverlay("游戏还没有结束！", 3000);

        // 3秒后进入二阶段
        QTimer::singleShot(3000, this, &NightmareBoss::enterPhase2);
    } else if (health <= 0 && m_phase == 2) {
        // 二阶段死亡 - 真正死亡
        qDebug() << "Nightmare Boss 二阶段被击败！";

        // 停止所有定时器
        if (m_nightmareWrapTimer) {
            m_nightmareWrapTimer->stop();
        }
        if (m_nightmareDescentTimer) {
            m_nightmareDescentTimer->stop();
        }
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

        // 发出dying信号
        emit dying(this);

        // 从场景移除并删除
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
    }
}

void NightmareBoss::enterPhase2() {
    m_phase = 2;
    m_isTransitioning = false;

    qDebug() << "Nightmare Boss 进入二阶段！";

    // 更换图片（使用预加载的Nightmare2.png，已经是正确尺寸）
    setPixmap(m_phase2Pixmap);

    // 恢复满血
    setHealth(400);  // 二阶段血量更高

    // 增强属性 - 仍然是单段dash但更快
    setContactDamage(8);     // 提高接触伤害
    setSpeed(1.5);           // 提高移动速度
    setDashSpeed(8.0);       // 更快的冲刺
    setDashChargeTime(700);  // 更短的蓄力时间

    // 设置二阶段技能
    setupPhase2Skills();

    qDebug() << "二阶段属性强化完成，获得新技能";
}

void NightmareBoss::setupPhase2Skills() {
    // 技能1：噩梦缠绕 - 每20秒自动释放
    m_nightmareWrapTimer = new QTimer(this);
    m_nightmareWrapTimer->setInterval(20000);  // 20秒
    connect(m_nightmareWrapTimer, &QTimer::timeout, this, &NightmareBoss::onNightmareWrapTimeout);
    m_nightmareWrapTimer->start();

    // 技能2：噩梦降临 - 每60秒自动释放
    m_nightmareDescentTimer = new QTimer(this);
    m_nightmareDescentTimer->setInterval(60000);  // 60秒
    connect(m_nightmareDescentTimer, &QTimer::timeout, this, &NightmareBoss::onNightmareDescentTimeout);
    m_nightmareDescentTimer->start();

    // 首次技能1完成后立即释放技能2
    m_firstDescentTriggered = false;

    qDebug() << "二阶段技能已激活：噩梦缠绕（每20秒），噩梦降临（首次技能1后+每60秒）";
}

void NightmareBoss::killAllEnemies() {
    if (!scene())
        return;

    QList<QGraphicsItem*> items = scene()->items();
    for (QGraphicsItem* item : items) {
        Enemy* enemy = dynamic_cast<Enemy*>(item);
        if (enemy && enemy != this) {
            // 直接删除小怪，不触发其dying信号
            enemy->setHealth(0);
            emit enemy->dying(enemy);  // 确保Level能清理引用
            qDebug() << "亡语击杀小怪";
        }
    }
}

void NightmareBoss::move() {
    // 一阶段和二阶段都使用标准单段dash（二阶段只是更快）
    Enemy::move();
}

void NightmareBoss::onNightmareWrapTimeout() {
    performNightmareWrap();
}

void NightmareBoss::onNightmareDescentTimeout() {
    performNightmareDescent();
}

void NightmareBoss::performNightmareWrap() {
    if (!player || m_phase != 2)
        return;  // 只有二阶段才能释放

    qDebug() << "释放技能1：噩梦缠绕";

    // 显示遮罩3秒 + 白色文字提示
    emit requestShowShadowOverlay("噩梦缠绕！！\n（你已被剥夺视野）", 3000);

    // 3秒后瞬移到玩家身边
    QTimer::singleShot(3000, this, [this]() {
        if (player) {
            QPointF playerPos = player->pos();
            // 在玩家周围随机位置（距离80-150像素，保持一定距离）
            int distance = QRandomGenerator::global()->bounded(80, 150);
            double angle = QRandomGenerator::global()->bounded(360) * M_PI / 180.0;
            
            double teleportX = playerPos.x() + distance * qCos(angle);
            double teleportY = playerPos.y() + distance * qSin(angle);
            
            // 确保瞬移位置在场景范围内
            teleportX = qBound(50.0, teleportX, 750.0);
            teleportY = qBound(50.0, teleportY, 550.0);
            
            setPos(teleportX, teleportY);
            
            // 打断dash进程
            m_isDashing = false;
            m_dashChargeCounter = 0;
            m_dashDuration = 0;
            xdir = 0;
            ydir = 0;
            
            qDebug() << "瞬移到玩家附近（距离" << distance << "）:" << teleportX << "," << teleportY << "，dash进程已打断";
            
            // 首次技能1完成后，立即释放技能2
            if (!m_firstDescentTriggered) {
                m_firstDescentTriggered = true;
                performNightmareDescent();
            }
        }
        
        emit requestHideShadowOverlay(); });
}

void NightmareBoss::performNightmareDescent() {
    if (m_phase != 2)
        return;  // 只有二阶段才能释放

    qDebug() << "释放技能2：噩梦降临";

    // 梦魇暂停移动1秒
    pauseTimers();
    xdir = 0;
    ydir = 0;
    m_isDashing = false;
    m_dashChargeCounter = 0;

    // 准备召唤的敌人列表：8个clock_normal + 20个clock_boom
    QVector<QPair<QString, int>> enemiesToSpawn;
    enemiesToSpawn.append(qMakePair(QString("clock_normal"), 8));
    enemiesToSpawn.append(qMakePair(QString("clock_boom"), 20));

    // 发送信号请求Level召唤敌人
    emit requestSpawnEnemies(enemiesToSpawn);

    // 1秒后恢复移动
    QTimer::singleShot(1000, this, [this]() {
        resumeTimers();
        qDebug() << "梦魇恢复移动，召唤完成"; });
}

void NightmareBoss::pauseTimers() {
    // 调用父类的暂停方法
    Boss::pauseTimers();

    // 暂停二阶段技能定时器
    if (m_nightmareWrapTimer && m_nightmareWrapTimer->isActive()) {
        m_nightmareWrapTimer->stop();
    }
    if (m_nightmareDescentTimer && m_nightmareDescentTimer->isActive()) {
        m_nightmareDescentTimer->stop();
    }
}

void NightmareBoss::resumeTimers() {
    // 调用父类的恢复方法
    Boss::resumeTimers();

    // 恢复二阶段技能定时器（只有在二阶段才恢复）
    if (m_phase == 2) {
        if (m_nightmareWrapTimer) {
            m_nightmareWrapTimer->start(20000);
        }
        if (m_nightmareDescentTimer) {
            m_nightmareDescentTimer->start(60000);
        }
    }
}
