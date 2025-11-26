#include "nightmareboss.h"
#include <QDebug>
#include <QFont>
#include <QGraphicsScene>
#include <QPainter>
#include <QRadialGradient>
#include <QRandomGenerator>
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
#include "player.h"

NightmareBoss::NightmareBoss(const QPixmap &pic, double scale, QGraphicsScene * /*scene*/)
    : Boss(pic, scale),
      m_phase(1),
      m_isTransitioning(false),
      m_nightmareWrapTimer(nullptr),
      m_nightmareDescentTimer(nullptr),
      m_firstDescentTriggered(false),
      m_forceDashing(false),
      m_forceDashTarget(0, 0),
      m_forceDashTimer(nullptr),
      m_shadowOverlay(nullptr),
      m_shadowText(nullptr),
      m_shadowTimer(nullptr),
      m_visionUpdateTimer(nullptr),
      m_visionRadius(120)
{
    // 遮罩效果使用QGraphicsItem::scene()方法获取场景
    // 参数scene保留为兼容性参数，但不需要使用

    // Nightmare Boss 一阶段属性
    setHealth(250);          // Boss血量
    setContactDamage(3);     // 接触伤害
    setVisionRange(1000);    // 视野范围（全图视野）
    setAttackRange(70);      // 攻击判定范围
    setAttackCooldown(1200); // 攻击冷却
    setSpeed(1.0);           // 移动速度

    crash_r = 35;      // 实际攻击范围
    damageScale = 0.7; // 伤害减免

    // 一阶段：使用单段冲刺模式
    setMovementPattern(MOVE_DASH);
    setDashChargeTime(1000); // 1秒蓄力
    setDashSpeed(6.0);       // 高速冲刺

    // 预加载二阶段图片（使用config中的boss尺寸）
    int bossSize = ConfigManager::instance().getSize("boss");
    m_phase2Pixmap = ResourceFactory::createBossImage(bossSize, 1, "nightmare2");

    qDebug() << "Nightmare Boss创建，一阶段模式，使用单段冲刺";
}

NightmareBoss::~NightmareBoss()
{
    if (m_nightmareWrapTimer)
    {
        m_nightmareWrapTimer->stop();
        delete m_nightmareWrapTimer;
    }
    if (m_nightmareDescentTimer)
    {
        m_nightmareDescentTimer->stop();
        delete m_nightmareDescentTimer;
    }
    if (m_forceDashTimer)
    {
        m_forceDashTimer->stop();
        delete m_forceDashTimer;
        m_forceDashTimer = nullptr;
    }
    if (m_visionUpdateTimer)
    {
        m_visionUpdateTimer->stop();
        delete m_visionUpdateTimer;
        m_visionUpdateTimer = nullptr;
    }
    // 清理遮罩相关资源
    hideShadowOverlay();
    if (m_shadowTimer)
    {
        m_shadowTimer->stop();
        delete m_shadowTimer;
        m_shadowTimer = nullptr;
    }
    qDebug() << "Nightmare Boss被摧毁";
}

void NightmareBoss::takeDamage(int damage)
{
    if (m_isTransitioning)
    {
        return; // 转换期间无敌
    }

    // 触发闪烁效果
    flash();

    // 调用父类的伤害处理
    health -= static_cast<int>(damage * damageScale);

    if (health <= 0 && m_phase == 1)
    {
        // 一阶段死亡 - 触发亡语
        m_isTransitioning = true;
        health = 1; // 保持存活，不触发dying信号

        qDebug() << "Nightmare Boss 一阶段死亡，触发亡语！";

        // 击杀场上所有小怪
        killAllEnemies();

        // 发送信号显示遮罩和文字（使用内部方法）
        // 发出阶段1死亡触发信号（Level将监听以启动背景渐变）
        emit phase1DeathTriggered();
        showShadowOverlay("游戏还没有结束！", 3000);

        // 3秒后进入二阶段
        QTimer::singleShot(4000, this, &NightmareBoss::enterPhase2);
    }
    else if (health <= 0 && m_phase == 2)
    {
        // 二阶段死亡 - 真正死亡
        qDebug() << "Nightmare Boss 二阶段被击败！";

        // 停止所有定时器
        if (m_nightmareWrapTimer)
        {
            m_nightmareWrapTimer->stop();
        }
        if (m_nightmareDescentTimer)
        {
            m_nightmareDescentTimer->stop();
        }
        if (aiTimer)
        {
            aiTimer->stop();
        }
        if (moveTimer)
        {
            moveTimer->stop();
        }
        if (attackTimer)
        {
            attackTimer->stop();
        }

        // 播放死亡音效
        AudioManager::instance().playSound("enemy_death");

        // 发出dying信号
        emit dying(this);

        // 从场景移除并删除
        if (scene())
        {
            scene()->removeItem(this);
        }
        deleteLater();
    }
}

void NightmareBoss::enterPhase2()
{
    m_phase = 2;
    m_isTransitioning = false;

    qDebug() << "Nightmare Boss 进入二阶段！";

    // 更换图片（使用预加载的Nightmare2.png，已经是正确尺寸）
    setPixmap(m_phase2Pixmap);

    // 恢复满血
    setHealth(300); // 二阶段血量更高

    // 增强属性 - 仍然是单段dash但更快
    setContactDamage(5);    // 提高接触伤害
    setSpeed(1.2);          // 提高移动速度
    setDashSpeed(6.2);      // 更快的冲刺
    setDashChargeTime(700); // 更短的蓄力时间
    setVisionRange(500);

    // 设置二阶段技能
    setupPhase2Skills();

    qDebug() << "二阶段属性强化完成，获得新技能";
}

void NightmareBoss::setupPhase2Skills()
{
    // 技能1：噩梦缠绕 - 每20秒自动释放
    m_nightmareWrapTimer = new QTimer(this);
    m_nightmareWrapTimer->setInterval(20000); // 20秒
    connect(m_nightmareWrapTimer, &QTimer::timeout, this, &NightmareBoss::onNightmareWrapTimeout);
    m_nightmareWrapTimer->start();

    // 技能2：噩梦降临 - 每60秒自动释放
    m_nightmareDescentTimer = new QTimer(this);
    m_nightmareDescentTimer->setInterval(60000); // 60秒
    connect(m_nightmareDescentTimer, &QTimer::timeout, this, &NightmareBoss::onNightmareDescentTimeout);
    m_nightmareDescentTimer->start();

    // 首次技能1完成后立即释放技能2
    m_firstDescentTriggered = false;

    qDebug() << "二阶段技能已激活：噩梦缠绕（每20秒），噩梦降临（首次技能1后+每60秒）";
}

void NightmareBoss::killAllEnemies()
{
    if (!scene())
        return;

    // 先收集所有需要击杀的敌人，避免在遍历时修改容器
    QList<QGraphicsItem *> items = scene()->items();
    QVector<Enemy *> enemiesToKill;

    for (QGraphicsItem *item : items)
    {
        Enemy *enemy = dynamic_cast<Enemy *>(item);
        if (enemy && enemy != this)
        {
            enemiesToKill.append(enemy);
        }
    }

    qDebug() << "亡语准备击杀" << enemiesToKill.size() << "个小怪";

    // 逐个击杀敌人 - 通过造成致命伤害让它们自然死亡
    for (Enemy *enemy : enemiesToKill)
    {
        if (enemy)
        {
            // 造成足够大的伤害让敌人死亡，触发完整的死亡流程
            enemy->takeDamage(9999);
        }
    }
}

void NightmareBoss::move()
{
    // 强制冲刺期间不执行普通移动（由executeForceDash单独处理）
    if (m_forceDashing)
    {
        return;
    }

    // 一阶段和二阶段都使用标准单段dash（二阶段只是更快）
    Enemy::move();
}

void NightmareBoss::onNightmareWrapTimeout()
{
    performNightmareWrap();
}

void NightmareBoss::onNightmareDescentTimeout()
{
    performNightmareDescent();
}

void NightmareBoss::performNightmareWrap()
{
    if (!player || m_phase != 2)
        return; // 只有二阶段才能释放

    qDebug() << "释放技能1：噩梦缠绕";

    // 显示遮罩3秒 + 白色文字提示（使用内部方法）
    showShadowOverlay("噩梦缠绕！！\n（你已被剥夺视野）", 3000);

    // 3秒后瞬移到玩家身边
    QTimer::singleShot(3000, this, [this]()
                       {
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

        hideShadowOverlay(); });
}

void NightmareBoss::performNightmareDescent()
{
    if (m_phase != 2)
        return; // 只有二阶段才能释放

    qDebug() << "释放技能2：噩梦降临";

    // 梦魇暂停移动
    pauseTimers();
    xdir = 0;
    ydir = 0;
    m_isDashing = false;
    m_dashChargeCounter = 0;

    // 准备召唤的敌人列表：8个clock_normal + 20个clock_boom
    QVector<QPair<QString, int>> enemiesToSpawn;
    enemiesToSpawn.append(qMakePair(QString("clock_normal"), 4));
    enemiesToSpawn.append(qMakePair(QString("pillow"), 4));
    enemiesToSpawn.append(qMakePair(QString("clock_boom"), 20));

    // 发送信号请求Level召唤敌人
    emit requestSpawnEnemies(enemiesToSpawn);

    // 1秒后开始强制冲刺
    QTimer::singleShot(1000, this, &NightmareBoss::startForceDash);

    // 2秒后恢复移动（强制冲刺会在此期间执行）
    QTimer::singleShot(2000, this, [this]()
                       {
        resumeTimers();
        qDebug() << "梦魇恢复移动，召唤完成"; });
}

// 开始强制冲刺（噩梦降临后1秒触发）
void NightmareBoss::startForceDash()
{
    if (!player)
        return;

    m_forceDashing = true;
    m_forceDashTarget = player->pos(); // 锁定玩家当前位置

    qDebug() << "噩梦降临：开始强制冲刺！目标位置:" << m_forceDashTarget;

    // 创建独立的强制冲刺定时器（不受pauseTimers影响）
    if (!m_forceDashTimer)
    {
        m_forceDashTimer = new QTimer(this);
        connect(m_forceDashTimer, &QTimer::timeout, this, &NightmareBoss::executeForceDash);
    }
    m_forceDashTimer->start(16); // 约60fps的更新频率
}

// 执行强制冲刺每帧移动
void NightmareBoss::executeForceDash()
{
    if (!m_forceDashing || !scene())
    {
        if (m_forceDashTimer)
        {
            m_forceDashTimer->stop();
        }
        return;
    }

    QPointF currentPos = pos();
    double dx = m_forceDashTarget.x() - currentPos.x();
    double dy = m_forceDashTarget.y() - currentPos.y();
    double dist = qSqrt(dx * dx + dy * dy);

    // 到达目标或距离很近时停止
    if (dist < 15)
    {
        m_forceDashing = false;
        if (m_forceDashTimer)
        {
            m_forceDashTimer->stop();
        }
        qDebug() << "噩梦降临强制冲刺完成，到达目标位置";
        return;
    }

    // 计算移动向量（高速冲刺）
    double dashSpeed = 15.0; // 每帧移动距离
    double moveX = (dx / dist) * dashSpeed;
    double moveY = (dy / dist) * dashSpeed;

    // 计算新位置
    double newX = currentPos.x() + moveX;
    double newY = currentPos.y() + moveY;

    // 边界检测
    QRectF pixmapRect = pixmap().rect();
    if (newX < 0)
        newX = 0;
    if (newX + pixmapRect.width() > 800)
        newX = 800 - pixmapRect.width();
    if (newY < 0)
        newY = 0;
    if (newY + pixmapRect.height() > 600)
        newY = 600 - pixmapRect.height();

    // 直接设置位置（绕过Entity::setPos的翻转逻辑）
    QGraphicsItem::setPos(newX, newY);

    // 更新朝向
    if (qAbs(dx) > qAbs(dy))
    {
        curr_xdir = (dx > 0) ? 1 : -1;
        curr_ydir = 0;
    }
    else
    {
        curr_xdir = 0;
        curr_ydir = (dy > 0) ? 1 : -1;
    }
}

void NightmareBoss::pauseTimers()
{
    // 调用父类的暂停方法
    Boss::pauseTimers();

    // 暂停二阶段技能定时器
    if (m_nightmareWrapTimer && m_nightmareWrapTimer->isActive())
    {
        m_nightmareWrapTimer->stop();
    }
    if (m_nightmareDescentTimer && m_nightmareDescentTimer->isActive())
    {
        m_nightmareDescentTimer->stop();
    }
}

void NightmareBoss::resumeTimers()
{
    // 调用父类的恢复方法
    Boss::resumeTimers();

    // 恢复二阶段技能定时器（只有在二阶段才恢复）
    if (m_phase == 2)
    {
        if (m_nightmareWrapTimer)
        {
            m_nightmareWrapTimer->start(20000);
        }
        if (m_nightmareDescentTimer)
        {
            m_nightmareDescentTimer->start(60000);
        }
    }
}

// ============== 遮罩效果方法 ==============

// 创建带有玩家视野圆形区域的遮罩（使用shadow.png作为背景）
QPixmap NightmareBoss::createShadowWithVision(const QPointF &playerPos)
{
    // 创建一个带有alpha通道的结果图片
    QPixmap result(800, 600);
    result.fill(Qt::transparent); // 先填充透明

    // 加载shadow.png图片作为遮罩背景
    QPixmap shadowPixmap("assets/boss/Nightmare/shadow.png");
    if (shadowPixmap.isNull())
    {
        qWarning() << "无法加载shadow.png，使用黑色矩形代替";
        shadowPixmap = QPixmap(800, 600);
        shadowPixmap.fill(QColor(0, 0, 0, 220));
    }
    else
    {
        // 缩放到全屏大小
        shadowPixmap = shadowPixmap.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    // 第一步：将shadow.png绘制到result上
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(0, 0, shadowPixmap);

    // 第二步：在玩家位置"挖"一个透明的圆形区域
    // 使用CompositionMode_Clear直接清除该区域为完全透明
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setBrush(Qt::SolidPattern);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(playerPos, m_visionRadius, m_visionRadius);

    painter.end();

    return result;
}

void NightmareBoss::showShadowOverlay(const QString &text, int duration)
{
    if (!scene() || !player)
        return;

    // 如果已有遮罩，先清理
    hideShadowOverlay();

    // 获取玩家位置并创建带视野的遮罩
    QPointF playerPos = player->pos() + QPointF(30, 30); // 调整到玩家中心
    QPixmap shadowPixmap = createShadowWithVision(playerPos);

    // 创建遮罩
    m_shadowOverlay = new QGraphicsPixmapItem(shadowPixmap);
    m_shadowOverlay->setPos(0, 0);
    m_shadowOverlay->setZValue(10000); // 最高层级
    scene()->addItem(m_shadowOverlay);

    // 如果有文字，显示白色文字
    if (!text.isEmpty())
    {
        m_shadowText = new QGraphicsTextItem(text);
        m_shadowText->setDefaultTextColor(Qt::white);
        m_shadowText->setFont(QFont("Arial", 24, QFont::Bold));

        // 居中显示
        QRectF textRect = m_shadowText->boundingRect();
        m_shadowText->setPos((800 - textRect.width()) / 2, (600 - textRect.height()) / 2);
        m_shadowText->setZValue(10001);
        scene()->addItem(m_shadowText);
    }

    // 启动视野更新定时器（跟随玩家位置）
    if (!m_visionUpdateTimer)
    {
        m_visionUpdateTimer = new QTimer(this);
        connect(m_visionUpdateTimer, &QTimer::timeout, this, &NightmareBoss::updateShadowVision);
    }
    m_visionUpdateTimer->start(50); // 每50ms更新一次视野位置

    qDebug() << "显示遮罩（带玩家视野），文字:" << text << "持续时间:" << duration << "ms";

    // 如果指定了持续时间，设置定时器自动隐藏
    if (duration > 0)
    {
        if (!m_shadowTimer)
        {
            m_shadowTimer = new QTimer(this);
            m_shadowTimer->setSingleShot(true);
            connect(m_shadowTimer, &QTimer::timeout, this, &NightmareBoss::hideShadowOverlay);
        }
        m_shadowTimer->start(duration);
    }
}

// 更新玩家视野区域（跟随玩家移动）
void NightmareBoss::updateShadowVision()
{
    if (!m_shadowOverlay || !player || !scene())
        return;

    // 获取玩家当前位置并重新生成遮罩
    QPointF playerPos = player->pos() + QPointF(30, 30); // 调整到玩家中心
    QPixmap newShadow = createShadowWithVision(playerPos);
    m_shadowOverlay->setPixmap(newShadow);
}

void NightmareBoss::hideShadowOverlay()
{
    // 停止视野更新定时器
    if (m_visionUpdateTimer)
    {
        m_visionUpdateTimer->stop();
    }

    if (m_shadowOverlay)
    {
        if (scene())
        {
            scene()->removeItem(m_shadowOverlay);
        }
        delete m_shadowOverlay;
        m_shadowOverlay = nullptr;
    }

    if (m_shadowText)
    {
        if (scene())
        {
            scene()->removeItem(m_shadowText);
        }
        delete m_shadowText;
        m_shadowText = nullptr;
    }

    if (m_shadowTimer)
    {
        m_shadowTimer->stop();
    }

    qDebug() << "隐藏遮罩";
}
