#include "walker.h"
#include <QBrush>
#include <QDateTime>
#include <QDebug>
#include <QPen>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <QtMath>
#include "../core/configmanager.h"
#include "player.h"
#include "statuseffect.h"

// ========== PoisonTrail 静态成员初始化 ==========
QMap<Player*, qint64> PoisonTrail::s_playerPoisonCooldowns;
QMap<Enemy*, qint64> PoisonTrail::s_enemyEncourageCooldowns;

// ========== Walker 实现 ==========

Walker::Walker(const QPixmap& pic, double scale)
    : Enemy(pic, scale),
      m_directionTimer(nullptr),
      m_trailTimer(nullptr),
      m_currentDirection(1.0, 0.0),
      m_walkerSpeed(DEFAULT_WALKER_SPEED),
      m_dirChangeInterval(DEFAULT_DIR_CHANGE_INTERVAL),
      m_trailSpawnInterval(DEFAULT_TRAIL_SPAWN_INTERVAL),
      m_trailDuration(DEFAULT_TRAIL_DURATION),
      m_encourageDuration(DEFAULT_ENCOURAGE_DURATION),
      m_poisonDuration(DEFAULT_POISON_DURATION) {
    // 从配置文件读取Walker属性
    ConfigManager& config = ConfigManager::instance();
    setHealth(config.getEnemyInt("walker", "health", 8));
    setContactDamage(0);  // 无接触伤害！
    setVisionRange(config.getEnemyDouble("walker", "vision_range", DEFAULT_VISION_RANGE));
    setAttackRange(0);  // 无攻击范围（不主动攻击）
    m_walkerSpeed = config.getEnemyDouble("walker", "speed", DEFAULT_WALKER_SPEED);
    setSpeed(m_walkerSpeed);
    m_trailDuration = config.getEnemyInt("walker", "trail_duration", DEFAULT_TRAIL_DURATION);
    m_poisonDuration = config.getEnemyInt("walker", "poison_duration", DEFAULT_POISON_DURATION);
    m_encourageDuration = config.getEnemyInt("walker", "encourage_duration", DEFAULT_ENCOURAGE_DURATION);

    // 设置碰撞半径
    setCrashR(20);

    // 不使用基类的移动模式，使用自定义随机移动
    setMovementPattern(MOVE_DIRECT);  // 设置为DIRECT但会被executeMovement覆盖

    // 初始化随机方向
    m_currentDirection = getRandomDirection();

    // 初始化定时器
    initTimers();

    qDebug() << "Walker 创建完成 - 速度:" << m_walkerSpeed
             << " 方向切换间隔:" << m_dirChangeInterval << "ms"
             << " 接触伤害:" << contactDamage;
}

Walker::~Walker() {
    if (m_directionTimer) {
        m_directionTimer->stop();
        delete m_directionTimer;
        m_directionTimer = nullptr;
    }
    if (m_trailTimer) {
        m_trailTimer->stop();
        delete m_trailTimer;
        m_trailTimer = nullptr;
    }
}

void Walker::initTimers() {
    // 方向切换定时器
    m_directionTimer = new QTimer(this);
    connect(m_directionTimer, &QTimer::timeout, this, &Walker::changeDirection);
    m_directionTimer->start(m_dirChangeInterval);

    // 毒痕生成定时器
    m_trailTimer = new QTimer(this);
    connect(m_trailTimer, &QTimer::timeout, this, &Walker::spawnPoisonTrail);
    m_trailTimer->start(m_trailSpawnInterval);
}

void Walker::pauseTimers() {
    Enemy::pauseTimers();

    if (m_directionTimer && m_directionTimer->isActive()) {
        m_directionTimer->stop();
    }
    if (m_trailTimer && m_trailTimer->isActive()) {
        m_trailTimer->stop();
    }
}

void Walker::resumeTimers() {
    Enemy::resumeTimers();

    if (m_directionTimer && !m_directionTimer->isActive()) {
        m_directionTimer->start(m_dirChangeInterval);
    }
    if (m_trailTimer && !m_trailTimer->isActive()) {
        m_trailTimer->start(m_trailSpawnInterval);
    }
}

QPointF Walker::getRandomDirection() {
    // 生成随机角度（0-360度）
    double angle = QRandomGenerator::global()->bounded(360) * M_PI / 180.0;
    return QPointF(qCos(angle), qSin(angle));
}

void Walker::changeDirection() {
    if (m_isPaused)
        return;

    // 随机改变移动方向
    m_currentDirection = getRandomDirection();

    // 更新朝向（xdir用于图片翻转）
    if (m_currentDirection.x() > 0) {
        xdir = 1;
    } else if (m_currentDirection.x() < 0) {
        xdir = -1;
    }

    qDebug() << "Walker 改变方向:" << m_currentDirection;
}

void Walker::spawnPoisonTrail() {
    if (m_isPaused)
        return;

    if (!scene())
        return;

    // 在当前位置生成毒痕
    QRectF rect = boundingRect();
    QPointF center = pos() + QPointF(rect.width() / 2.0, rect.height() / 2.0);

    PoisonTrail* trail = new PoisonTrail(center, m_trailDuration, m_encourageDuration, m_poisonDuration);
    trail->setScene(scene());
    scene()->addItem(trail);

    // 设置Z值让毒痕在地面上显示（低于角色）
    trail->setZValue(-10);
}

void Walker::executeMovement() {
    if (m_isPaused)
        return;

    // 获取当前位置
    QPointF currentPos = pos();

    // 计算新位置
    QPointF newPos = currentPos + m_currentDirection * m_walkerSpeed;

    // 边界检测（800x600场景）
    QRectF rect = boundingRect();
    double sceneWidth = 800;
    double sceneHeight = 600;

    bool hitBoundary = false;

    // X边界检测
    if (newPos.x() < 0) {
        newPos.setX(0);
        m_currentDirection.setX(-m_currentDirection.x());
        hitBoundary = true;
    } else if (newPos.x() + rect.width() > sceneWidth) {
        newPos.setX(sceneWidth - rect.width());
        m_currentDirection.setX(-m_currentDirection.x());
        hitBoundary = true;
    }

    // Y边界检测
    if (newPos.y() < 0) {
        newPos.setY(0);
        m_currentDirection.setY(-m_currentDirection.y());
        hitBoundary = true;
    } else if (newPos.y() + rect.height() > sceneHeight) {
        newPos.setY(sceneHeight - rect.height());
        m_currentDirection.setY(-m_currentDirection.y());
        hitBoundary = true;
    }

    if (hitBoundary) {
        // 更新朝向
        if (m_currentDirection.x() > 0)
            xdir = 1;
        else if (m_currentDirection.x() < 0)
            xdir = -1;
    }

    // 设置新位置
    setPos(newPos);
}

// ========== PoisonTrail 实现 ==========

PoisonTrail::PoisonTrail(const QPointF& center, int duration, double encourageDur, double poisonDur, QObject* parent)
    : QObject(parent),
      QGraphicsEllipseItem(-TRAIL_RADIUS, -TRAIL_RADIUS, TRAIL_RADIUS * 2, TRAIL_RADIUS * 2),
      m_fadeTimer(nullptr),
      m_checkTimer(nullptr),
      m_totalDuration(duration),
      m_elapsedTime(0),
      m_encourageDuration(encourageDur),
      m_poisonDuration(poisonDur) {
    // 设置位置
    setPos(center);

    // 创建墨绿色渐变效果
    QRadialGradient gradient(0, 0, TRAIL_RADIUS);
    gradient.setColorAt(0, QColor(0, 80, 20, 200));     // 中心：深墨绿色，较透明
    gradient.setColorAt(0.5, QColor(0, 100, 30, 150));  // 中间：墨绿色
    gradient.setColorAt(1, QColor(0, 60, 15, 50));      // 边缘：暗绿色，更透明

    setBrush(QBrush(gradient));
    setPen(Qt::NoPen);

    // 淡出动画定时器
    m_fadeTimer = new QTimer(this);
    connect(m_fadeTimer, &QTimer::timeout, this, &PoisonTrail::updateFade);
    m_fadeTimer->start(50);  // 每50ms更新一次透明度

    // 碰撞检测定时器
    m_checkTimer = new QTimer(this);
    connect(m_checkTimer, &QTimer::timeout, this, &PoisonTrail::checkCollisions);
    m_checkTimer->start(100);  // 每100ms检测一次碰撞
}

PoisonTrail::~PoisonTrail() {
    if (m_fadeTimer) {
        m_fadeTimer->stop();
    }
    if (m_checkTimer) {
        m_checkTimer->stop();
    }
}

void PoisonTrail::setScene(QGraphicsScene* scene) {
    Q_UNUSED(scene);
    // 场景设置已通过 scene()->addItem() 完成
}

void PoisonTrail::updateFade() {
    m_elapsedTime += 50;

    if (m_elapsedTime >= m_totalDuration) {
        onExpired();
        return;
    }

    // 计算淡出进度（后半段开始淡出）
    double fadeStartTime = m_totalDuration * 0.5;
    if (m_elapsedTime > fadeStartTime) {
        double fadeProgress = (m_elapsedTime - fadeStartTime) / (m_totalDuration - fadeStartTime);
        double opacity = 1.0 - fadeProgress;

        // 更新渐变透明度
        QRadialGradient gradient(0, 0, TRAIL_RADIUS);
        gradient.setColorAt(0, QColor(0, 80, 20, static_cast<int>(200 * opacity)));
        gradient.setColorAt(0.5, QColor(0, 100, 30, static_cast<int>(150 * opacity)));
        gradient.setColorAt(1, QColor(0, 60, 15, static_cast<int>(50 * opacity)));
        setBrush(QBrush(gradient));
    }
}

void PoisonTrail::onExpired() {
    // 停止定时器
    if (m_fadeTimer) {
        m_fadeTimer->stop();
    }
    if (m_checkTimer) {
        m_checkTimer->stop();
    }

    // 从场景移除并删除
    if (scene()) {
        scene()->removeItem(this);
    }
    deleteLater();
}

void PoisonTrail::checkCollisions() {
    if (!scene())
        return;

    // 获取与毒痕重叠的所有物品
    QList<QGraphicsItem*> collidingItems = this->collidingItems();

    for (QGraphicsItem* item : collidingItems) {
        // 检查是否是玩家
        if (Player* player = dynamic_cast<Player*>(item)) {
            if (canApplyPoisonTo(player)) {
                applyPoisonToPlayer(player);
                markPoisonApplied(player);
            }
        }
        // 检查是否是敌人（非Walker）
        else if (Enemy* enemy = dynamic_cast<Enemy*>(item)) {
            // 排除Walker类型
            if (dynamic_cast<Walker*>(enemy) == nullptr) {
                if (canApplyEncourageTo(enemy)) {
                    applyEncourageToEnemy(enemy);
                    markEncourageApplied(enemy);
                }
            }
        }
    }
}

bool PoisonTrail::canApplyPoisonTo(Player* player) {
    if (!player)
        return false;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (s_playerPoisonCooldowns.contains(player)) {
        qint64 lastApplyTime = s_playerPoisonCooldowns[player];
        if (currentTime - lastApplyTime < EFFECT_COOLDOWN_MS) {
            return false;  // 仍在冷却中
        }
    }
    return true;
}

bool PoisonTrail::canApplyEncourageTo(Enemy* enemy) {
    if (!enemy)
        return false;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (s_enemyEncourageCooldowns.contains(enemy)) {
        qint64 lastApplyTime = s_enemyEncourageCooldowns[enemy];
        if (currentTime - lastApplyTime < EFFECT_COOLDOWN_MS) {
            return false;  // 仍在冷却中
        }
    }
    return true;
}

void PoisonTrail::markPoisonApplied(Player* player) {
    s_playerPoisonCooldowns[player] = QDateTime::currentMSecsSinceEpoch();
}

void PoisonTrail::markEncourageApplied(Enemy* enemy) {
    s_enemyEncourageCooldowns[enemy] = QDateTime::currentMSecsSinceEpoch();
}

void PoisonTrail::clearCooldowns() {
    s_playerPoisonCooldowns.clear();
    s_enemyEncourageCooldowns.clear();
}

void PoisonTrail::applyPoisonToPlayer(Player* player) {
    if (!player)
        return;

    // 检查玩家是否有效（血量大于0）
    if (player->getCurrentHealth() <= 0)
        return;

    // 如果玩家处于无敌状态（如被吸纳），不施加中毒
    if (player->isInvincible()) {
        qDebug() << "Walker毒痕：玩家无敌，跳过中毒效果";
        return;
    }

    qDebug() << "Walker毒痕：对玩家施加中毒效果";

    // 100%触发中毒效果
    int duration = static_cast<int>(m_poisonDuration);
    // 限制中毒时长不超过玩家当前血量
    if (duration > static_cast<int>(player->getCurrentHealth())) {
        duration = static_cast<int>(player->getCurrentHealth());
    }
    if (duration < 1)
        duration = 1;

    // 伤害1 = 每秒扣0.5颗心（持续3秒共扣1.5心）
    PoisonEffect* effect = new PoisonEffect(player, duration, 1);
    effect->applyTo(player);

    // 显示毒痕中毒提示（位置稍微偏移避免重叠）
    StatusEffect::showFloatText(player->scene(), QString("中毒！"), player->pos() + QPointF(0, -15), QColor(0, 100, 30));
}

void PoisonTrail::applyEncourageToEnemy(Enemy* enemy) {
    if (!enemy)
        return;

    qDebug() << "Walker毒痕：对敌人施加鼓舞效果 (+50%移速)";

    // +50%移速效果，持续3秒，使用专用的EncourageEffect（自带文字提示）
    EncourageEffect* effect = new EncourageEffect(m_encourageDuration, 1.5);
    effect->applyTo(enemy);
}
