#include "player.h"
#include <QDateTime>
#include <QElapsedTimer>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QPen>
#include <QRandomGenerator>
#include <QtGlobal>
#include <cmath>
#include "../core/configmanager.h"
#include "constants.h"
#include "enemy.h"

namespace {
class TeleportEffectItem : public QObject, public QGraphicsEllipseItem {
   public:
    TeleportEffectItem(QGraphicsScene* scene, const QPointF& center, qreal radius = 35.0)
        : QObject(scene), QGraphicsEllipseItem(-radius, -radius, radius * 2, radius * 2) {
        if (!scene) {
            deleteLater();
            return;
        }

        setPen(QPen(QColor(120, 220, 255, 220), 3));
        setBrush(Qt::NoBrush);
        setZValue(900);
        setPos(center);
        scene->addItem(this);

        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this]() { advanceEffect(); });
        m_timer->start(16);
    }

   private:
    void advanceEffect() {
        m_elapsed += 16;
        qreal progress = qBound(0.0, m_elapsed / m_duration, 1.0);
        setOpacity(1.0 - progress);
        setScale(1.0 + progress * 0.5);

        if (m_elapsed >= m_duration) {
            if (scene()) {
                scene()->removeItem(this);
            }
            deleteLater();
        }
    }

    QTimer* m_timer = nullptr;
    qreal m_elapsed = 0.0;
    qreal m_duration = 220.0;
};

void spawnTeleportEffect(QGraphicsScene* scene, const QPointF& center) {
    if (!scene)
        return;
    new TeleportEffectItem(scene, center);
}
}  // namespace

Player::Player(const QPixmap& pic_player, double scale)
    : redContainers(8), redHearts(8.0), blackHearts(0), shootCooldown(150), lastShootTime(0), bulletHurt(5), isDead(false), keys(1), m_frostChance(0), m_shieldCount(0), m_shieldSprite(nullptr) {
    setTransformationMode(Qt::SmoothTransformation);

    // 从配置文件读取玩家属性
    ConfigManager& config = ConfigManager::instance();
    redContainers = config.getPlayerInt("health", 8);
    redHearts = static_cast<double>(redContainers);
    speed = config.getPlayerDouble("speed", 5.0);
    shootCooldown = config.getPlayerInt("shoot_cooldown", 150);
    bulletHurt = config.getPlayerInt("bullet_hurt", 5);
    crash_r = config.getPlayerInt("crash_radius", 40);
    m_teleportCooldownMs = config.getPlayerInt("teleport_cooldown", 5000);
    m_teleportDistance = config.getPlayerDouble("teleport_distance", 120.0);
    m_ultimateCooldownMs = config.getPlayerInt("ultimate_cooldown", 60000);
    m_ultimateDurationMs = config.getPlayerInt("ultimate_duration", 10000);
    m_bulletScaleMultiplier = config.getPlayerDouble("ultimate_bullet_scale", 2.0);

    // 加载寒冰子弹图片并缩放到与普通子弹相同大小
    QPixmap frostPic("assets/items/bullet_frost.png");
    if (frostPic.isNull()) {
        qWarning() << "无法加载寒冰子弹图片";
    } else {
        // 缩放到与普通子弹相同的大小（20x20）
        m_frostBulletPic = frostPic.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        qDebug() << "寒冰子弹图片加载成功，大小:" << m_frostBulletPic.size();
    }

    // 如果scale是1.0，直接使用原始pixmap，否则按比例缩放
    invincible = false;
    if (scale == 1.0) {
        this->setPixmap(pic_player);
    } else {
        // 按比例缩放（保持宽高比）
        this->setPixmap(pic_player.scaled(
            pic_player.width() * scale,
            pic_player.height() * scale,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    }

    // 禁用缓存以避免留下轨迹
    setCacheMode(QGraphicsItem::NoCache);

    xdir = 0;
    ydir = 0;
    shootSpeed = 1.0;
    shootType = 0;
    damageScale = 1.0;
    curr_xdir = 0;
    curr_ydir = 1;           // 默认向下
    this->setPos(400, 300);  // 初始位置,根据实际需要后续修改

    hurt = 1;  // 后续可修改

    bombs = 1;

    // 初始化射击按键
    shootKeysPressed[Qt::Key_Up] = false;
    shootKeysPressed[Qt::Key_Down] = false;
    shootKeysPressed[Qt::Key_Left] = false;
    shootKeysPressed[Qt::Key_Right] = false;

    // 初始化移动按键
    keysPressed[Qt::Key_W] = false;
    keysPressed[Qt::Key_A] = false;
    keysPressed[Qt::Key_S] = false;
    keysPressed[Qt::Key_D] = false;
    keysPressed[Qt::Key_Space] = false;
    keysPressed[Qt::Key_E] = false;

    keysTimer = new QTimer(this);
    connect(keysTimer, &QTimer::timeout, this, &Player::move);
    keysTimer->start(16);

    crashTimer = new QTimer(this);
    connect(crashTimer, &QTimer::timeout, this, &Player::crashEnemy);
    crashTimer->start(50);

    // 射击检测定时器（持续检测射击按键状态）
    shootTimer = new QTimer(this);
    connect(shootTimer, &QTimer::timeout, this, &Player::checkShoot);
    shootTimer->start(16);  // 每16ms检测一次
    // 增伤技能初始即可使用
    m_lastUltimateTime = QDateTime::currentMSecsSinceEpoch() - m_ultimateCooldownMs;

    // 初始无敌时间，防止刚进入游戏时被判定碰撞闪烁
    invincible = true;
    // 确保 isFlashing 初始为 false
    isFlashing = false;
    QTimer::singleShot(1000, this, [this]() { invincible = false; });
}

void Player::keyPressEvent(QKeyEvent* event) {
    if (!event || isDead)  // 已死亡则不处理输入
        return;

    if (event->key() == Qt::Key_Q) {
        tryTeleport();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_E) {
        activateUltimate();
        event->accept();
        return;
    }

    // 处理移动按键（方向键）
    if (keysPressed.count(event->key())) {
        keysPressed[event->key()] = true;
        event->accept();
        return;
    }

    // 处理射击按键（WASD）- 只记录按键状态
    if (shootKeysPressed.count(event->key())) {
        shootKeysPressed[event->key()] = true;
        event->accept();
        return;
    }
}

void Player::keyReleaseEvent(QKeyEvent* event) {
    if (!event || isDead)  // 已死亡则不处理输入
        return;

    // 释放移动键
    if (keysPressed.count(event->key())) {
        keysPressed[event->key()] = false;
    }
    // 释放射击键
    if (shootKeysPressed.count(event->key())) {
        shootKeysPressed[event->key()] = false;
    }
}

void Player::checkShoot() {
    if (isDead)  // 已死亡则不射击
        return;

    if (m_isPaused)  // 暂停状态不射击
        return;

    // 检查是否有射击键被按下
    int shootKey = -1;

    // 按优先级检查射击键（后检查的优先级更高）
    if (shootKeysPressed[Qt::Key_Up])
        shootKey = Qt::Key_Up;
    if (shootKeysPressed[Qt::Key_Down])
        shootKey = Qt::Key_Down;
    if (shootKeysPressed[Qt::Key_Left])
        shootKey = Qt::Key_Left;
    if (shootKeysPressed[Qt::Key_Right])
        shootKey = Qt::Key_Right;

    // 如果有按键按下，检查冷却时间
    if (shootKey != -1) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        if (currentTime - lastShootTime >= shootCooldown) {
            shoot(shootKey);
            lastShootTime = currentTime;
        }
    }
}

void Player::shoot(int key) {
    // 检查是否已死亡、无法移动（昏睡等状态）、无法射击或场景不存在
    if (isDead || !m_canMove || !m_canShoot || !scene())
        return;

    // 检查子弹图片是否有效
    if (pic_bullet.isNull()) {
        qWarning() << "Player::shoot: pic_bullet is null, cannot shoot";
        return;
    }

    // 播放射击音效
    AudioManager::instance().playSound("player_shoot");
    // 计算子弹发射位置
    QPointF bulletPos = this->pos() + QPointF(pixmap().width() / 2 - 7.5, pixmap().height() / 2 - 30);

    // 根据寒冰概率决定是否发射寒冰子弹
    bool isFrostBullet = false;
    if (m_frostChance > 0 && !m_frostBulletPic.isNull()) {
        int roll = QRandomGenerator::global()->bounded(100);
        isFrostBullet = (roll < m_frostChance);
    }

    // 选择子弹图片
    const QPixmap& bulletPic = isFrostBullet ? m_frostBulletPic : pic_bullet;

    auto* bullet = new Projectile(0, bulletHurt, bulletPos, bulletPic);  // 使用可配置的玩家子弹伤害
    bullet->setSpeed(shootSpeed);
    bullet->setIsFrostBullet(isFrostBullet);  // 设置寒冰属性

    // 将子弹添加到场景中
    if (scene()) {
        scene()->addItem(bullet);
    } else {
        // 如果场景不存在，删除子弹防止内存泄漏
        delete bullet;
        return;
    }

    // 设置子弹方向和速度
    switch (key) {
        case Qt::Key_Up:
            bullet->setDir(0, -9);
            break;
        case Qt::Key_Down:
            bullet->setDir(0, 9);
            break;
        case Qt::Key_Left:
            bullet->setDir(-9, 0);
            break;
        case Qt::Key_Right:
            bullet->setDir(9, 0);
            break;
        default:
            break;
    }

    // 移除频繁的调试输出以避免性能问题
    // qDebug() << "射击音效已触发";
}

void Player::move() {
    if (isDead)  // 已死亡则不移动
        return;

    if (!m_canMove)  // 无法移动（昏睡等状态）
        return;

    if (m_isPaused)  // 暂停状态不移动
        return;

    xdir = 0;
    ydir = 0;

    if (keysPressed[Qt::Key_W])
        ydir--;
    if (keysPressed[Qt::Key_S])
        ydir++;
    if (keysPressed[Qt::Key_A])
        xdir--;
    if (keysPressed[Qt::Key_D])
        xdir++;

    // 边界检测
    double newX = pos().x() + xdir * speed;
    double newY = pos().y() + ydir * speed;
    QPointF clampedPos = clampPositionWithinRoom(QPointF(newX, newY));

    // 更新朝向图像
    if (xdir != 0 || ydir != 0) {
        if (ydir > 0) {
            if (!down.isNull())
                this->setPixmap(down);
            curr_ydir = 1;
            curr_xdir = 0;
        } else if (ydir < 0) {
            if (!up.isNull())
                this->setPixmap(up);
            curr_ydir = -1;
            curr_xdir = 0;
        } else if (xdir > 0) {
            if (!right.isNull())
                this->setPixmap(right);
            curr_xdir = 1;
            curr_ydir = 0;
        } else if (xdir < 0) {
            if (!left.isNull())
                this->setPixmap(left);
            curr_xdir = -1;
            curr_ydir = 0;
        }
    }

    this->setPos(clampedPos);

    // 更新护盾位置
    if (m_shieldSprite) {
        updateShieldDisplay();
    }
}

void Player::tryTeleport() {
    if (isDead || !m_canMove || m_isPaused)
        return;

    QPointF dir = currentMoveDirection();
    if (qFuzzyIsNull(dir.x()) && qFuzzyIsNull(dir.y()))
        return;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastTeleportTime < m_teleportCooldownMs)
        return;

    QPointF desiredPos = pos() + dir * m_teleportDistance;
    QPointF clampedPos = clampPositionWithinRoom(desiredPos);
    auto* scenePtr = scene();
    QRectF bounds = boundingRect();
    QPointF centerOffset(bounds.width() / 2.0, bounds.height() / 2.0);

    if (scenePtr) {
        spawnTeleportEffect(scenePtr, pos() + centerOffset);
    }

    AudioManager::instance().playSound("player_teleport");
    setPos(clampedPos);
    if (scenePtr) {
        spawnTeleportEffect(scenePtr, clampedPos + centerOffset);
    }
    m_lastTeleportTime = now;
}

QPointF Player::currentMoveDirection() const {
    QPointF dir(0, 0);
    if (keysPressed.value(Qt::Key_W, false))
        dir.ry() -= 1;
    if (keysPressed.value(Qt::Key_S, false))
        dir.ry() += 1;
    if (keysPressed.value(Qt::Key_A, false))
        dir.rx() -= 1;
    if (keysPressed.value(Qt::Key_D, false))
        dir.rx() += 1;

    double length = std::hypot(dir.x(), dir.y());
    if (length <= 0.0)
        return {0, 0};

    return {dir.x() / length, dir.y() / length};
}

QPointF Player::clampPositionWithinRoom(const QPointF& candidate) const {
    double newX = candidate.x();
    double newY = candidate.y();

    double doorMargin = 20.0;
    double doorSize = 100.0;

    if (newY < 0) {
        if (qAbs(newX + pixmap().width() / 2 - 400) < doorSize)
            newY = qMax(newY, -doorMargin);
        else
            newY = 0;
    }

    if (newY > room_bound_y - pixmap().height()) {
        if (qAbs(newX + pixmap().width() / 2 - 400) < doorSize)
            newY = qMin(newY, (double)(room_bound_y - pixmap().height()) + doorMargin);
        else
            newY = room_bound_y - pixmap().height();
    }

    if (newX < 0) {
        if (qAbs(newY + pixmap().height() / 2 - 300) < doorSize)
            newX = qMax(newX, -doorMargin);
        else
            newX = 0;
    }

    if (newX > room_bound_x - pixmap().width()) {
        if (qAbs(newY + pixmap().height() / 2 - 300) < doorSize)
            newX = qMin(newX, (double)(room_bound_x - pixmap().width()) + doorMargin);
        else
            newX = room_bound_x - pixmap().width();
    }

    return {newX, newY};
}

int Player::getTeleportRemainingMs() const {
    if (m_lastTeleportTime == 0)
        return 0;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int remaining = m_teleportCooldownMs - static_cast<int>(now - m_lastTeleportTime);
    return qMax(0, remaining);
}

double Player::getTeleportReadyRatio() const {
    if (m_teleportCooldownMs <= 0)
        return 1.0;

    auto remaining = static_cast<double>(getTeleportRemainingMs());
    auto total = static_cast<double>(m_teleportCooldownMs);
    double ratio = 1.0 - qBound(0.0, remaining / total, 1.0);
    return qBound(0.0, ratio, 1.0);
}

bool Player::isTeleportReady() const {
    return getTeleportRemainingMs() <= 0;
}

void Player::activateUltimate() {
    if (isDead || m_isPaused || !m_canMove)
        return;

    if (m_isUltimateActive)
        return;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastUltimateTime < m_ultimateCooldownMs)
        return;

    m_ultimateOriginalBulletHurt = bulletHurt;
    m_originalBulletPic = pic_bullet;  // 保存原始子弹图片

    // 伤害变为2倍
    bulletHurt = qMax(1, bulletHurt * 2);

    // 子弹变大1.5倍
    if (!pic_bullet.isNull()) {
        pic_bullet = pic_bullet.scaled(
            pic_bullet.width() * m_bulletScaleMultiplier,
            pic_bullet.height() * m_bulletScaleMultiplier,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
    }

    m_isUltimateActive = true;
    m_lastUltimateTime = now;

    if (!m_ultimateTimer) {
        m_ultimateTimer = new QTimer(this);
        m_ultimateTimer->setSingleShot(true);
        connect(m_ultimateTimer, &QTimer::timeout, this, &Player::endUltimate);
    }
    m_ultimateTimer->start(m_ultimateDurationMs);

    AudioManager::instance().playSound("player_teleport");
}

void Player::endUltimate() {
    if (!m_isUltimateActive)
        return;

    m_isUltimateActive = false;
    if (m_ultimateTimer)
        m_ultimateTimer->stop();

    // 恢复原始伤害
    if (m_ultimateOriginalBulletHurt > 0)
        bulletHurt = m_ultimateOriginalBulletHurt;

    // 恢复原始子弹图片
    if (!m_originalBulletPic.isNull())
        pic_bullet = m_originalBulletPic;
}

int Player::getUltimateRemainingMs() const {
    if (m_isUltimateActive)
        return 0;

    if (m_lastUltimateTime == 0)
        return 0;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int remaining = m_ultimateCooldownMs - static_cast<int>(now - m_lastUltimateTime);
    return qMax(0, remaining);
}

double Player::getUltimateReadyRatio() const {
    if (m_ultimateCooldownMs <= 0)
        return 1.0;
    if (m_isUltimateActive)
        return 1.0;

    double remaining = static_cast<double>(getUltimateRemainingMs());
    double total = static_cast<double>(m_ultimateCooldownMs);
    double ratio = 1.0 - qBound(0.0, remaining / total, 1.0);
    return qBound(0.0, ratio, 1.0);
}

int Player::getUltimateActiveRemainingMs() const {
    if (!m_isUltimateActive || !m_ultimateTimer)
        return 0;
    return qMax(0, m_ultimateTimer->remainingTime());
}

double Player::getUltimateActiveRatio() const {
    if (!m_isUltimateActive || m_ultimateDurationMs <= 0)
        return 0.0;

    auto remaining = static_cast<double>(getUltimateActiveRemainingMs());
    double ratio = 1.0 - qBound(0.0, remaining / static_cast<double>(m_ultimateDurationMs), 1.0);
    return qBound(0.0, ratio, 1.0);
}

bool Player::isUltimateReady() const {
    return !m_isUltimateActive && getUltimateRemainingMs() <= 0;
}

bool Player::isUltimateActive() const {
    return m_isUltimateActive;
}

void Player::takeDamage(int damage) {
    if (isDead || invincible)  // 已死亡或无敌则不受伤
        return;

    if (damage <= 0)  // 0伤害或负伤害直接返回，避免触发闪烁和无敌
        return;

    // 先检查护盾
    if (m_shieldCount > 0) {
        m_shieldCount--;
        updateShieldDisplay();
        setInvincible();  // 护盾抵消后也给予短暂无敌
        qDebug() << "护盾抵消伤害，剩余护盾:" << m_shieldCount;
        return;
    }

    flash();
    double oldHealth = redHearts;
    damage *= damageScale;

    // 注意：黑心不再作为护盾使用，改为复活机制
    while (damage > 0 && redHearts > 0.0) {
        redHearts -= 0.5;
        damage--;
    }
    if (redHearts != oldHealth) {
        emit healthChanged(redHearts, getMaxHealth());
        if (redHearts < oldHealth) {
            emit playerDamaged();
        }
    }
    if (redHearts <= 0.0) {
        // 尝试黑心复活
        if (tryBlackHeartRevive()) {
            return;  // 复活成功，不执行死亡
        }
        die();
    } else {
        setInvincible();
    }
}

void Player::forceTakeDamage(int damage) {
    if (isDead)  // 已死亡则不受伤，但无视无敌状态
        return;

    if (damage <= 0)  // 0伤害或负伤害直接返回
        return;

    // 先检查护盾（强制伤害也可被护盾抵消）
    if (m_shieldCount > 0) {
        m_shieldCount--;
        updateShieldDisplay();
        setInvincible();
        qDebug() << "护盾抵消强制伤害，剩余护盾:" << m_shieldCount;
        return;
    }

    flash();
    double oldHealth = redHearts;
    damage *= damageScale;

    // 黑心不再作为护盾
    while (damage > 0 && redHearts > 0.0) {
        redHearts -= 0.5;
        damage--;
    }
    if (redHearts != oldHealth) {
        emit healthChanged(redHearts, getMaxHealth());
        if (redHearts < oldHealth) {
            emit playerDamaged();
        }
    }
    if (redHearts <= 0.0) {
        // 尝试黑心复活
        if (tryBlackHeartRevive()) {
            return;
        }
        die();
    } else {
        setInvincible();
    }
}

// 死亡效果
void Player::die() {
    if (isDead) {  // 避免重复触发
        return;
    }
    endUltimate();

    isDead = true;

    // 暂未使用实际的player_death.wav
    AudioManager::instance().playSound("player_death");
    qDebug() << "玩家死亡音效已触发";

    // 停止所有定时器
    if (keysTimer) {
        keysTimer->stop();
    }
    if (crashTimer) {
        crashTimer->stop();
    }
    if (shootTimer) {
        shootTimer->stop();
    }

    // 不要从场景中移除，让 GameView::initGame() 统一清理
    // 只需隐藏玩家即可
    setVisible(false);

    emit playerDied();  // 发出玩家死亡信号（只发一次）
}

// 短暂无敌效果，有待UI同学的具体实现
void Player::setInvincible() {
    invincible = true;
    QTimer::singleShot(1000, this, [this]() { invincible = false; });
}

// 持久无敌（需要手动取消）
void Player::setPermanentInvincible(bool inv) {
    invincible = inv;
    qDebug() << "[Player] 持久无敌设置为:" << inv;
}

void Player::crashEnemy() {
    if (isDead || !scene())  // 已死亡或无场景则不检测碰撞
        return;

    if (m_isPaused)  // 暂停状态不检测碰撞
        return;

    // 无敌状态下不检测碰撞（也不触发敌人的特殊效果）
    if (invincible)
        return;

    // 使用collidingItems代替遍历整个场景
    QList<QGraphicsItem*> collisions = collidingItems();
    for (QGraphicsItem* item : collisions) {
        if (auto enemy = dynamic_cast<Enemy*>(item)) {
            // 使用像素级碰撞检测
            if (Entity::pixelCollision(this, enemy)) {
                this->takeDamage(enemy->getContactDamage());
                // 触发敌人的特殊接触效果（惊吓/昏迷等）
                enemy->onContactWithPlayer(this);
                break;  // 一次只处理一个碰撞
            }
        }
    }
}

void Player::placeBomb() {
    if (bombs <= 0)
        return;
    auto posi = this->pos();
    QTimer::singleShot(500, this, [this, posi]() {
        foreach (QGraphicsItem* item, scene()->items()) {
            if (auto it = dynamic_cast<Enemy*>(item)) {
                if (abs(it->pos().x() - posi.x()) > bomb_r ||
                    abs(it->pos().y() - posi.y()) > bomb_r)
                    continue;
                else {
                    it->takeDamage(bombHurt);
                }
            }
        }
    });
    bombs--;
}

void Player::focusOutEvent(QFocusEvent* event) {
    QGraphicsItem::focusOutEvent(event);
    setFocus();
}

// ============ 新道具系统方法 ============

void Player::addFrostChance(int amount) {
    m_frostChance = qMin(60, m_frostChance + amount);
    qDebug() << "寒冰子弹概率增加到:" << m_frostChance << "%";
}

void Player::addShield(int count) {
    m_shieldCount += count;
    updateShieldDisplay();
    qDebug() << "护盾增加，当前护盾数:" << m_shieldCount;
}

void Player::removeShield(int count) {
    m_shieldCount = qMax(0, m_shieldCount - count);
    updateShieldDisplay();
    qDebug() << "护盾减少，当前护盾数:" << m_shieldCount;
}

void Player::updateShieldDisplay() {
    if (!scene())
        return;

    if (m_shieldCount > 0) {
        // 如果护盾图案不存在，创建它
        if (!m_shieldSprite) {
            QPixmap shieldPix("assets/items/shield.png");
            // 如果加载失败，尝试备用路径
            if (shieldPix.isNull()) {
                shieldPix = QPixmap("assets/props/shield.png");
            }
            if (shieldPix.isNull()) {
                qWarning() << "无法加载护盾图片";
                return;
            }

            // 缩放到角色1.5倍大小
            int shieldSize = static_cast<int>(pixmap().width() * 1.5);
            shieldPix = shieldPix.scaled(shieldSize, shieldSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            m_shieldSprite = new QGraphicsPixmapItem(shieldPix);
            m_shieldSprite->setZValue(zValue() + 1);  // 显示在玩家上方
            m_shieldSprite->setOpacity(0.7);          // 半透明
            scene()->addItem(m_shieldSprite);
        }

        // 更新护盾位置（居中在玩家身上）
        if (m_shieldSprite) {
            QPointF shieldPos = pos() + QPointF(
                                            (pixmap().width() - m_shieldSprite->pixmap().width()) / 2,
                                            (pixmap().height() - m_shieldSprite->pixmap().height()) / 2);
            m_shieldSprite->setPos(shieldPos);
        }
    } else {
        // 护盾为0，移除图案
        if (m_shieldSprite) {
            if (m_shieldSprite->scene()) {
                scene()->removeItem(m_shieldSprite);
            }
            delete m_shieldSprite;
            m_shieldSprite = nullptr;
        }
    }
}

bool Player::tryBlackHeartRevive() {
    if (blackHearts <= 0) {
        return false;  // 没有黑心，无法复活
    }

    qDebug() << "触发黑心复活！黑心数:" << blackHearts;

    // 计算恢复的血量（每个黑心 = 6点血量，最多填满血量上限）
    int healAmount = blackHearts * 6;
    double newHealth = qMin(static_cast<double>(healAmount), static_cast<double>(redContainers));

    // 清空黑心
    int usedBlackHearts = blackHearts;
    blackHearts = 0;

    // 设置血量
    redHearts = newHealth;

    // 设置短暂无敌状态（会自动取消）
    setInvincible();

    // 发出信号通知UI播放动画
    emit blackHeartReviveStarted();

    // 发出血量变化信号
    emit healthChanged(redHearts, getMaxHealth());

    qDebug() << "黑心复活成功！使用黑心:" << usedBlackHearts << "，恢复血量:" << newHealth;

    return true;
}
