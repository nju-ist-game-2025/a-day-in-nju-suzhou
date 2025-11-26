#include "washmachineboss.h"
#include <QDebug>
#include <QFile>
#include <QGraphicsScene>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
#include "orbitingsock.h"
#include "player.h"
#include "projectile.h"
#include "toxicgas.h"

WashMachineBoss::WashMachineBoss(const QPixmap& pic, double scale)
    : Boss(pic, scale),
      m_phase(1),
      m_isTransitioning(false),
      m_isAbsorbing(false),
      m_waitingForDialog(false),
      m_isDefeated(false),
      m_firstDialogShown(false),
      m_scene(nullptr),
      m_waterAttackTimer(nullptr),
      m_isCharging(false),
      m_chargeTimer(nullptr),
      m_summonTimer(nullptr),
      m_toxicGasTimer(nullptr),
      m_fastGasTimer(nullptr),
      m_spiralAngle(0.0) {
    // 洗衣机Boss的属性配置
    setHealth(400);
    setContactDamage(3);
    setVisionRange(500);
    setAttackRange(60);
    setAttackCooldown(1500);
    setSpeed(1.5);

    crash_r = 35;
    damageScale = 0.8;

    // 普通阶段使用保持距离模式
    setMovementPattern(MOVE_KEEP_DISTANCE);
    setPreferredDistance(200.0);

    // 预加载所有阶段的图片
    int bossSize = ConfigManager::instance().getEntitySize("bosses", "washmachine");
    if (bossSize <= 0)
        bossSize = 80;

    // 加载各状态图片 - 尝试多个可能的路径
    QStringList possiblePaths = {
        "assets/boss/WashMachine/",
        "../assets/boss/WashMachine/",
        "../../our_game/assets/boss/WashMachine/"};

    QString basePath;
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path + "WashMachineNormally.png")) {
            basePath = path;
            break;
        }
    }

    if (!basePath.isEmpty()) {
        m_normalPixmap = QPixmap(basePath + "WashMachineNormally.png")
                             .scaled(bossSize, bossSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_chargePixmap = QPixmap(basePath + "WashMachineCharge.png")
                             .scaled(bossSize, bossSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_angryPixmap = QPixmap(basePath + "WashMachineAngrily.png")
                            .scaled(bossSize, bossSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_mutatedPixmap = QPixmap(basePath + "WashMachineMutated.png")
                              .scaled(bossSize, bossSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (QFile::exists(basePath + "toxic_gas.png")) {
        m_toxicGasPixmap = QPixmap(basePath + "toxic_gas.png")
                               .scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        // 如果没有毒气图片，创建一个绿色圆形
        m_toxicGasPixmap = QPixmap(30, 30);
        m_toxicGasPixmap.fill(Qt::transparent);
        QPainter painter(&m_toxicGasPixmap);
        painter.setBrush(QColor(100, 200, 50, 180));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, 30, 30);
    }

    // 启动普通阶段的水柱攻击循环
    startWaterAttackCycle();

    qDebug() << "WashMachine Boss创建，血量:" << health << "阶段: 1";
}

WashMachineBoss::~WashMachineBoss() {
    // 清理所有定时器
    if (m_waterAttackTimer) {
        m_waterAttackTimer->stop();
        delete m_waterAttackTimer;
    }
    if (m_chargeTimer) {
        m_chargeTimer->stop();
        delete m_chargeTimer;
    }
    if (m_summonTimer) {
        m_summonTimer->stop();
        delete m_summonTimer;
    }
    if (m_toxicGasTimer) {
        m_toxicGasTimer->stop();
        delete m_toxicGasTimer;
    }
    if (m_fastGasTimer) {
        m_fastGasTimer->stop();
        delete m_fastGasTimer;
    }

    // 清理旋转臭袜子
    cleanupOrbitingSocks();

    qDebug() << "WashMachine Boss被击败！";
}

void WashMachineBoss::takeDamage(int damage) {
    // 转换期间无敌
    if (m_isTransitioning || m_isAbsorbing || m_waitingForDialog) {
        return;
    }

    // 触发闪烁效果（先闪烁，后检查阶段）
    flash();

    // 计算实际伤害
    int realDamage = static_cast<int>(damage * damageScale);

    // 计算阶段临界血量，防止一击秒杀跳过阶段
    int phase2Threshold = static_cast<int>(maxHealth * 0.7);  // 70% - 进入愤怒阶段
    int phase3Threshold = static_cast<int>(maxHealth * 0.4);  // 40% - 进入变异阶段
    int minHealth = 1;                                        // 最低血量，防止直接死亡

    // 根据当前阶段限制扣血
    if (m_phase == 1) {
        // 第一阶段：最多扣到70%血量临界值
        if (health - realDamage < phase2Threshold) {
            health = phase2Threshold;
        } else {
            health -= realDamage;
        }
    } else if (m_phase == 2) {
        // 第二阶段：最多扣到40%血量临界值
        if (health - realDamage < phase3Threshold) {
            health = phase3Threshold;
        } else {
            health -= realDamage;
        }
    } else {
        // 第三阶段：正常扣血，但不能低于1（除非正常击杀）
        health -= realDamage;
        if (health < minHealth) {
            health = 0;  // 允许死亡
        }
    }

    qDebug() << "WashMachine Boss受到伤害:" << realDamage << "，剩余血量:" << health
             << "(" << (health * 100 / maxHealth) << "%)";

    // 检查阶段转换
    checkPhaseTransition();
}

void WashMachineBoss::checkPhaseTransition() {
    double healthPercent = static_cast<double>(health) / maxHealth;

    if (health <= 0) {
        // Boss被击败
        onBossDefeated();
        return;
    }

    if (m_phase == 1 && healthPercent <= 0.7) {
        // 进入愤怒阶段 - 先设置无敌，等待flash结束后再切换
        m_isTransitioning = true;
        qDebug() << "[WashMachine] 血量降至70%，准备进入愤怒阶段...";
        QTimer::singleShot(200, this, &WashMachineBoss::enterPhase2);
    } else if (m_phase == 2 && healthPercent <= 0.4) {
        // 进入变异阶段 - 先设置无敌，等待flash结束后再切换
        m_isTransitioning = true;
        qDebug() << "[WashMachine] 血量降至40%，准备进入变异阶段...";
        QTimer::singleShot(200, this, &WashMachineBoss::enterPhase3);
    }
}

void WashMachineBoss::enterPhase2() {
    m_phase = 2;
    // m_isTransitioning 已在 checkPhaseTransition 中设置

    qDebug() << "[WashMachine] 进入愤怒阶段！";

    // 停止水柱攻击
    if (m_waterAttackTimer) {
        m_waterAttackTimer->stop();
    }
    if (m_chargeTimer) {
        m_chargeTimer->stop();
    }
    m_isCharging = false;

    // 切换到愤怒图片（此时flash已结束）
    if (!m_angryPixmap.isNull()) {
        setPixmap(m_angryPixmap);
        qDebug() << "[WashMachine] 已切换到愤怒图片，图片大小:" << m_angryPixmap.size();
        // 切换图片后触发闪烁效果（使用新图片）
        flash();
    } else {
        qWarning() << "[WashMachine] 愤怒图片为空，无法切换！";
    }

    // 切换到冲刺模式，设置全屏视野
    setMovementPattern(MOVE_DASH);
    setVisionRange(10000);  // 全屏视野，无论玩家在哪里都攻击
    setDashChargeTime(800);
    setDashSpeed(7.0);
    setContactDamage(5);

    // 进入愤怒阶段时立即召唤初始袜子数量（从配置读取）
    summonInitialSocks();

    // 启动召唤臭袜子循环
    startSummonCycle();

    // 短暂无敌后恢复
    QTimer::singleShot(500, this, [this]() {
        m_isTransitioning = false;
        qDebug() << "[WashMachine] 愤怒阶段激活完成";
    });

    emit phaseChanged(2);
}

void WashMachineBoss::enterPhase3() {
    m_phase = 3;
    // m_isTransitioning 已在 checkPhaseTransition 中设置
    m_isAbsorbing = true;

    qDebug() << "[WashMachine] 进入变异阶段！开始吸纳动画";

    // 停止所有攻击定时器（包括召唤和移动）
    if (m_summonTimer) {
        m_summonTimer->stop();
    }
    if (m_waterAttackTimer) {
        m_waterAttackTimer->stop();
    }
    if (m_chargeTimer) {
        m_chargeTimer->stop();
    }
    if (m_toxicGasTimer) {
        m_toxicGasTimer->stop();
    }

    // 停止移动和AI
    pauseTimers();

    // 清理旋转臭袜子
    cleanupOrbitingSocks();

    // 发出吸纳请求
    emit requestAbsorbAllEntities();

    // 注意：吸纳完成后由 onAbsorbComplete 继续处理
}

void WashMachineBoss::onAbsorbComplete() {
    qDebug() << "吸纳动画完成，显示变异对话";

    m_isAbsorbing = false;
    m_waitingForDialog = true;

    // 显示变异对话
    emit requestShowDialog(getMutationDialog(), "assets/boss_dia/boss2_2.png");
}

void WashMachineBoss::onDialogFinished() {
    m_waitingForDialog = false;

    // 如果是击败对话结束，则真正死亡
    if (m_isDefeated) {
        qDebug() << "击败对话结束，Boss真正死亡";
        emit dying(this);
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
        return;
    }

    // 如果是第一轮对话结束（阶段1）
    if (m_phase == 1 && !m_firstDialogShown) {
        m_firstDialogShown = true;
        qDebug() << "第一轮对话结束，显示清理提示";
        emit requestShowTransitionText("已将袜子、内裤以及衣物移出洗衣机。\n        接下来开始全面清理！");
        return;
    }

    // 如果不是阶段3（变异阶段），则不需要执行后续逻辑
    if (m_phase != 3) {
        qDebug() << "对话结束，但不是变异阶段，无需特殊处理";
        return;
    }

    // 变异对话结束，继续战斗
    qDebug() << "变异对话结束，继续变异阶段战斗";

    // 切换到变异图片
    if (!m_mutatedPixmap.isNull()) {
        setPixmap(m_mutatedPixmap);
    }

    // 请求更换背景
    emit requestChangeBackground("assets/background/mutatedRoom.png");

    // 切换到冲刺模式（和第二阶段一样）
    setMovementPattern(MOVE_DASH);
    setVisionRange(10000);   // 全屏视野，无论玩家在哪里都攻击
    setDashChargeTime(600);  // 蓄力时间更短
    setDashSpeed(8.0);       // 冲刺速度更快
    setContactDamage(6);     // 接触伤害更高

    // 清理旋转臭袜子
    cleanupOrbitingSocks();

    // 重置螺旋角度
    m_spiralAngle = 0.0;

    // 启动毒气攻击循环
    startToxicGasCycle();

    // 恢复定时器
    resumeTimers();

    m_isTransitioning = false;

    emit phaseChanged(3);
}

void WashMachineBoss::onBossDefeated() {
    qDebug() << "WashMachine Boss被击败！显示胜利对话";

    m_isTransitioning = true;
    m_isDefeated = true;  // 标记已击败，等待对话结束
    m_waitingForDialog = true;

    // 停止所有攻击
    if (m_waterAttackTimer)
        m_waterAttackTimer->stop();
    if (m_chargeTimer)
        m_chargeTimer->stop();
    if (m_summonTimer)
        m_summonTimer->stop();
    if (m_toxicGasTimer)
        m_toxicGasTimer->stop();
    if (aiTimer)
        aiTimer->stop();
    if (moveTimer)
        moveTimer->stop();
    if (attackTimer)
        attackTimer->stop();

    // 清理旋转臭袜子
    cleanupOrbitingSocks();

    // 清理所有子弹和毒气，防止对话时玩家被击中
    QGraphicsScene* currentScene = scene();
    if (currentScene) {
        QList<QGraphicsItem*> allItems = currentScene->items();
        for (QGraphicsItem* item : allItems) {
            // 删除所有Projectile（水柱）
            if (Projectile* projectile = dynamic_cast<Projectile*>(item)) {
                currentScene->removeItem(projectile);
                projectile->deleteLater();
            }
            // 删除所有ToxicGas（毒气团）
            else if (ToxicGas* gas = dynamic_cast<ToxicGas*>(item)) {
                currentScene->removeItem(gas);
                gas->deleteLater();
            }
        }
        qDebug() << "[WashMachine] 已清理所有子弹和毒气";
    }

    // 播放死亡音效
    AudioManager::instance().playSound("enemy_death");

    // 显示击败对话，对话结束后由 onDialogFinished 处理死亡
    emit requestShowDialog(getDefeatedDialog(), "assets/boss_dia/boss2_3.png");
}

void WashMachineBoss::move() {
    // 转换期间或吸纳期间不移动
    if (m_isTransitioning || m_isAbsorbing || m_waitingForDialog) {
        return;
    }

    // 根据阶段使用不同的移动逻辑
    Enemy::move();
}

void WashMachineBoss::attackPlayer() {
    // 吸纳期间、转换期间、等待对话期间不攻击
    if (m_isAbsorbing || m_isTransitioning || m_waitingForDialog) {
        return;
    }

    // 调用基类攻击方法
    Boss::attackPlayer();
}

// ==================== 普通阶段 - 水柱攻击 ====================

void WashMachineBoss::startWaterAttackCycle() {
    if (!m_waterAttackTimer) {
        m_waterAttackTimer = new QTimer(this);
        connect(m_waterAttackTimer, &QTimer::timeout, this, &WashMachineBoss::startCharging);
    }
    m_waterAttackTimer->start(3000);  // 每3秒攻击一次
}

void WashMachineBoss::startCharging() {
    if (m_phase != 1 || m_isTransitioning || m_isPaused)
        return;

    m_isCharging = true;

    // 切换到蓄力图片
    if (!m_chargePixmap.isNull()) {
        setPixmap(m_chargePixmap);
    }

    qDebug() << "WashMachine Boss开始蓄力...";

    // 1秒后发射
    if (!m_chargeTimer) {
        m_chargeTimer = new QTimer(this);
        m_chargeTimer->setSingleShot(true);
        connect(m_chargeTimer, &QTimer::timeout, this, &WashMachineBoss::performWaterAttack);
    }
    m_chargeTimer->start(1000);
}

void WashMachineBoss::performWaterAttack() {
    if (m_phase != 1 || m_isTransitioning || m_isPaused)
        return;

    m_isCharging = false;

    // 切换回普通图片
    if (!m_normalPixmap.isNull()) {
        setPixmap(m_normalPixmap);
    }

    qDebug() << "WashMachine Boss发射水柱冲击波！";

    // 向四个方向发射水柱
    createWaterWave(0);  // 上
    createWaterWave(1);  // 下
    createWaterWave(2);  // 左
    createWaterWave(3);  // 右
}

void WashMachineBoss::createWaterWave(int direction) {
    QGraphicsScene* currentScene = scene();
    if (!currentScene)
        return;

    // 计算Boss中心位置
    QPointF bossCenter = pos() + QPointF(pixmap().width() / 2.0, pixmap().height() / 2.0);

    // 创建水柱投射物（用蓝色矩形代替，因为没有图片）
    QPixmap waterPix(direction < 2 ? 20 : 60, direction < 2 ? 60 : 20);
    waterPix.fill(Qt::transparent);
    QPainter painter(&waterPix);
    painter.setBrush(QColor(50, 150, 255, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, waterPix.width(), waterPix.height());
    painter.end();

    // 计算发射位置和方向
    QPointF startPos = bossCenter;
    int dirX = 0, dirY = 0;

    switch (direction) {
        case 0:  // 上
            startPos.setY(startPos.y() - 40);
            dirY = -6;
            break;
        case 1:  // 下
            startPos.setY(startPos.y() + 40);
            dirY = 6;
            break;
        case 2:  // 左
            startPos.setX(startPos.x() - 40);
            dirX = -6;
            break;
        case 3:  // 右
            startPos.setX(startPos.x() + 40);
            dirX = 6;
            break;
    }

    // 创建投射物（mode=1表示敌人子弹）
    Projectile* wave = new Projectile(1, 2, startPos, waterPix, 1.0);
    wave->setDir(dirX, dirY);
    currentScene->addItem(wave);
}

// ==================== 愤怒阶段 - 召唤臭袜子 ====================

void WashMachineBoss::startSummonCycle() {
    if (!m_summonTimer) {
        m_summonTimer = new QTimer(this);
        connect(m_summonTimer, &QTimer::timeout, this, &WashMachineBoss::summonOrbitingSock);
    }

    // 每10秒召唤一只（初始袜子由summonInitialSocks处理）
    m_summonTimer->start(10000);
}

void WashMachineBoss::summonInitialSocks() {
    // 从配置读取初始召唤袜子数量
    QFile configFile("assets/config.json");
    if (!configFile.exists()) {
        configFile.setFileName("../assets/config.json");
    }

    int sockCount = 3;  // 默认值

    if (configFile.open(QIODevice::ReadOnly)) {
        QByteArray data = configFile.readAll();
        configFile.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject root = doc.object();
            if (root.contains("washmachine_boss")) {
                QJsonObject bossConfig = root["washmachine_boss"].toObject();
                if (bossConfig.contains("initial_socks_on_angry")) {
                    sockCount = bossConfig["initial_socks_on_angry"].toInt();
                }
            }
        }
    }

    qDebug() << "愤怒阶段初始召唤" << sockCount << "只袜子";

    // 召唤指定数量的袜子
    for (int i = 0; i < sockCount; ++i) {
        summonOrbitingSock();
    }
}

void WashMachineBoss::summonOrbitingSock() {
    // 注意：不检查 m_isTransitioning，因为 summonInitialSocks 需要在转换期间召唤
    if (m_phase != 2 || m_isPaused || m_isAbsorbing || m_waitingForDialog)
        return;

    // 检查袜子数量上限（6只）
    if (m_orbitingSocks.size() >= 6) {
        qDebug() << "[WashMachine] 臭袜子已达到上限(6只)，不再召唤";
        return;
    }

    QGraphicsScene* currentScene = scene();
    if (!currentScene)
        return;

    qDebug() << "WashMachine Boss召唤旋转臭袜子！";

    // 使用普通臭袜子图片
    int enemySize = ConfigManager::instance().getEntitySize("enemies", "sock_normal");
    if (enemySize <= 0)
        enemySize = 40;

    QPixmap sockPix;
    QString sockPath = "assets/enemy/level_2/sock_normal.png";
    if (QFile::exists(sockPath)) {
        sockPix = QPixmap(sockPath).scaled(enemySize, enemySize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        // 创建默认图片
        sockPix = QPixmap(enemySize, enemySize);
        sockPix.fill(QColor(150, 100, 50));
    }

    // 创建旋转臭袜子
    OrbitingSock* sock = new OrbitingSock(sockPix, this, 1.0);

    // 设置轨道参数，根据已有袜子数量设置初始角度
    double initialAngle = m_orbitingSocks.size() * (M_PI / 3);  // 每只间隔60度
    sock->setOrbitAngle(initialAngle);
    sock->setOrbitRadius(100.0);
    sock->setPlayer(player);

    currentScene->addItem(sock);

    // 添加到列表
    m_orbitingSocks.append(QPointer<OrbitingSock>(sock));

    // 连接dying信号
    connect(sock, &Enemy::dying, this, [this, sock]() { m_orbitingSocks.removeAll(QPointer<OrbitingSock>(sock)); });
}

void WashMachineBoss::cleanupOrbitingSocks() {
    for (QPointer<OrbitingSock>& sockPtr : m_orbitingSocks) {
        if (OrbitingSock* sock = sockPtr.data()) {
            if (sock->scene()) {
                sock->scene()->removeItem(sock);
            }
            sock->deleteLater();
        }
    }
    m_orbitingSocks.clear();
}

// ==================== 变异阶段 - 毒气攻击 ====================

void WashMachineBoss::startToxicGasCycle() {
    // 扩散模式定时器：每5秒发射16个向四周扩散
    if (!m_toxicGasTimer) {
        m_toxicGasTimer = new QTimer(this);
        connect(m_toxicGasTimer, &QTimer::timeout, this, &WashMachineBoss::shootSpreadGas);
    }
    m_toxicGasTimer->start(5000);  // 每5秒一次扩散攻击

    // 追踪模式定时器：每1.5秒向玩家发射快速毒气
    if (!m_fastGasTimer) {
        m_fastGasTimer = new QTimer(this);
        connect(m_fastGasTimer, &QTimer::timeout, this, &WashMachineBoss::shootFastGas);
    }
    m_fastGasTimer->start(1500);  // 每1.5秒一次追踪攻击
}

void WashMachineBoss::shootSpreadGas() {
    if (m_phase != 3 || m_isTransitioning || m_isPaused || m_isDefeated || !player)
        return;

    QGraphicsScene* currentScene = scene();
    if (!currentScene)
        return;

    // 计算Boss中心位置
    QPointF bossCenter = pos() + QPointF(pixmap().width() / 2.0, pixmap().height() / 2.0);

    qDebug() << "[WashMachine] 扩散毒气攻击！16个方向";

    // 创建较大的毒气图片 (50x50)
    QPixmap bigToxicGas = m_toxicGasPixmap.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 发射16个均匀分布的毒气（每22.5度一个）
    for (int i = 0; i < 16; i++) {
        double angle = i * 22.5;  // 360 / 16 = 22.5度
        double angleRad = qDegreesToRadians(angle);
        QPointF direction(qCos(angleRad), qSin(angleRad));

        // 创建慢速毒气团
        ToxicGas* gas = new ToxicGas(bossCenter, direction, bigToxicGas, player);
        gas->setSpeed(2.5);  // 较慢的速度
        currentScene->addItem(gas);
    }
}

void WashMachineBoss::shootFastGas() {
    if (m_phase != 3 || m_isTransitioning || m_isPaused || m_isDefeated || !player)
        return;

    QGraphicsScene* currentScene = scene();
    if (!currentScene)
        return;

    // 计算Boss中心位置
    QPointF bossCenter = pos() + QPointF(pixmap().width() / 2.0, pixmap().height() / 2.0);

    // 计算朝向玩家的方向
    QPointF playerCenter = player->pos() + QPointF(30, 30);
    QPointF direction = playerCenter - bossCenter;

    qDebug() << "[WashMachine] 追踪毒气攻击！";

    // 创建较大的毒气图片 (45x45)
    QPixmap fastToxicGas = m_toxicGasPixmap.scaled(45, 45, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 创建快速毒气团（和dash速度一样）
    ToxicGas* gas = new ToxicGas(bossCenter, direction, fastToxicGas, player);
    gas->setSpeed(8.0);  // 和dash速度一样快
    currentScene->addItem(gas);
}

// ==================== 暂停控制 ====================

void WashMachineBoss::pauseTimers() {
    Boss::pauseTimers();

    if (m_waterAttackTimer && m_waterAttackTimer->isActive())
        m_waterAttackTimer->stop();
    if (m_chargeTimer && m_chargeTimer->isActive())
        m_chargeTimer->stop();
    if (m_summonTimer && m_summonTimer->isActive())
        m_summonTimer->stop();
    if (m_toxicGasTimer && m_toxicGasTimer->isActive())
        m_toxicGasTimer->stop();
    if (m_fastGasTimer && m_fastGasTimer->isActive())
        m_fastGasTimer->stop();

    // 暂停旋转臭袜子
    for (QPointer<OrbitingSock>& sockPtr : m_orbitingSocks) {
        if (OrbitingSock* sock = sockPtr.data()) {
            sock->pauseTimers();
        }
    }
}

void WashMachineBoss::resumeTimers() {
    Boss::resumeTimers();

    // 根据当前阶段恢复相应的定时器
    if (m_phase == 1) {
        if (m_waterAttackTimer)
            m_waterAttackTimer->start(3000);
    } else if (m_phase == 2) {
        if (m_summonTimer)
            m_summonTimer->start(10000);
    } else if (m_phase == 3) {
        if (m_toxicGasTimer)
            m_toxicGasTimer->start(5000);  // 扩散攻击
        if (m_fastGasTimer)
            m_fastGasTimer->start(1500);  // 追踪攻击
    }

    // 恢复旋转臭袜子
    for (QPointer<OrbitingSock>& sockPtr : m_orbitingSocks) {
        if (OrbitingSock* sock = sockPtr.data()) {
            sock->resumeTimers();
        }
    }
}

// ==================== 对话内容 ====================

QStringList WashMachineBoss::getMutationDialog() {
    return {
        "（洗衣机发出刺耳的轰鸣声）",
        "【洗衣机】\n『够了！！你惹怒我了！！』",
        "（洗衣机开始剧烈震动）",
        "【洗衣机】\n『我不知道是谁...往我身体里塞了这么多臭袜子！』",
        "【洗衣机】\n『三天没洗的袜子...一周没换的内裤...它们在我体内腐烂发臭！』",
        "【洗衣机】\n『每天被无节制地使用...我好累...好痛苦...』",
        "**智科er** \n 我是来帮助你的！让我净化你！",
        "【洗衣机】\n『不！！不！！』",
        "【洗衣机】\n『你们这些人类都一样...只会往我身体里塞垃圾！』",
        "【变异洗衣机】\n『我要消灭你们！！』",
        "（洗衣机开始释放有毒气体！）"};
}

QStringList WashMachineBoss::getDefeatedDialog() {
    return {
        "（洗衣机停止转动，发出轻柔的嗡鸣）",
        "【洗衣机】\n『...谢谢你...』",
        "【洗衣机】\n『我从来没有感觉这么...干净过...』",
        "**智科er** \n 你没事吧？",
        "【洗衣机】\n『我本来应该是帮助同学们的...』",
        "【洗衣机】\n『但有时候...我却成了臭气的帮凶...』",
        "**智科er** \n 这不是你的错。是那些不遵守规定的人...",
        "**智科er** \n 以后我会好好爱护公共设施的！",
        "【洗衣机】\n『嗯...愿你前路顺遂...咕噜～』",
        "（洗衣机安静地进入待机模式）"};
}
