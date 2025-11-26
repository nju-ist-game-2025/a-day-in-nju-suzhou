#include "player.h"
#include <QDateTime>
#include <QElapsedTimer>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QPen>
#include <QtGlobal>
#include <cmath>
#include "constants.h"
#include "enemy.h"

namespace
{
class TeleportEffectItem : public QObject, public QGraphicsEllipseItem
{
public:
    TeleportEffectItem(QGraphicsScene *scene, const QPointF &center, qreal radius = 35.0)
        : QObject(scene), QGraphicsEllipseItem(-radius, -radius, radius * 2, radius * 2)
    {
        if (!scene)
        {
            deleteLater();
            return;
        }

            setPen(QPen(QColor(120, 220, 255, 220), 3));
            setBrush(Qt::NoBrush);
            setZValue(900);
            setPos(center);
            scene->addItem(this);

        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this]()
                { advanceEffect(); });
        m_timer->start(16);
    }

private:
    void advanceEffect() {
        m_elapsed += 16;
        qreal progress = qBound(0.0, m_elapsed / m_duration, 1.0);
        setOpacity(1.0 - progress);
        setScale(1.0 + progress * 0.5);

        if (m_elapsed >= m_duration)
        {
            if (scene())
            {
                scene()->removeItem(this);
            }
            deleteLater();
        }
    }

    QTimer *m_timer = nullptr;
    qreal m_elapsed = 0.0;
    qreal m_duration = 220.0;
};

void spawnTeleportEffect(QGraphicsScene *scene, const QPointF &center) {
    if (!scene)
        return;
    new TeleportEffectItem(scene, center);
}
} // namespace

Player::Player(const QPixmap& pic_player, double scale)
    : redContainers(5), redHearts(5.0), blackHearts(0), soulHearts(0), shootCooldown(150), lastShootTime(0), bulletHurt(2), isDead(false), keys(0) {  // 默认150毫秒射击冷却，子弹伤害默认2
    setTransformationMode(Qt::SmoothTransformation);

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
    speed = 5.0;
    shootSpeed = 1.0;
    shootType = 0;
    damageScale = 1.0;
    curr_xdir = 0;
    curr_ydir = 1;           // 默认向下
    this->setPos(400, 300);  // 初始位置,根据实际需要后续修改

    hurt = 1;      // 后续可修改
    crash_r = 40;  // 增大碰撞半径以匹配新的角色大小

    bombs = 0;

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
    // 计算子弹发射位置（从角色中心发射）
    QPointF bulletPos = this->pos() + QPointF(pixmap().width() / 2 - 7.5, pixmap().height() / 2 - 7.5);
    // if(shootType == 0) //改变为发射激光模式，需要ui的图片实现
    auto* bullet = new Projectile(0, bulletHurt, bulletPos, pic_bullet);  // 使用可配置的玩家子弹伤害
    bullet->setSpeed(shootSpeed);

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
        return QPointF(0, 0);

    return QPointF(dir.x() / length, dir.y() / length);
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

    return QPointF(newX, newY);
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

    double remaining = static_cast<double>(getTeleportRemainingMs());
    double total = static_cast<double>(m_teleportCooldownMs);
    double ratio = 1.0 - qBound(0.0, remaining / total, 1.0);
    return qBound(0.0, ratio, 1.0);
}

bool Player::isTeleportReady() const {
    return getTeleportRemainingMs() <= 0;
}

void Player::takeDamage(int damage) {
    if (isDead || invincible)  // 已死亡或无敌则不受伤
        return;

    if (damage <= 0)  // 0伤害或负伤害直接返回，避免触发闪烁和无敌
        return;

    flash();
    double oldHealth = redHearts;
    damage *= damageScale;

    while (damage > 0 && soulHearts > 0) {
        soulHearts--;
        damage -= 2;
    }
    while (damage > 0 && blackHearts > 0) {
        blackHearts--;
        damage -= 2;
    }
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
    if (redHearts <= 0.0)
        die();
    else
        setInvincible();
}

void Player::forceTakeDamage(int damage) {
    if (isDead)  // 已死亡则不受伤，但无视无敌状态
        return;

    if (damage <= 0)  // 0伤害或负伤害直接返回
        return;

    flash();
    double oldHealth = redHearts;
    damage *= damageScale;

    while (damage > 0 && soulHearts > 0) {
        soulHearts--;
        damage -= 2;
    }
    while (damage > 0 && blackHearts > 0) {
        blackHearts--;
        damage -= 2;
    }
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
    if (redHearts <= 0.0)
        die();
    else
        setInvincible();
}

// 死亡效果
void Player::die() {
    if (isDead)  // 避免重复触发
        return;

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

    // 使用collidingItems代替遍历整个场景
    QList<QGraphicsItem*> collisions = collidingItems();
    for (QGraphicsItem* item : collisions) {
        if (auto it = dynamic_cast<Enemy*>(item)) {
            // 使用图片中心点计算距离，而不是左上角pos()
            QRectF playerRect = boundingRect();
            QRectF enemyRect = it->boundingRect();
            QPointF playerCenter = pos() + QPointF(playerRect.width() / 2, playerRect.height() / 2);
            QPointF enemyCenter = it->pos() + QPointF(enemyRect.width() / 2, enemyRect.height() / 2);

            double dx = abs(enemyCenter.x() - playerCenter.x());
            double dy = abs(enemyCenter.y() - playerCenter.y());

            if (dx <= it->crash_r + crash_r && dy <= it->crash_r + crash_r) {
                this->takeDamage(it->getContactDamage());
                // 触发敌人的特殊接触效果（惊吓/昏迷等）
                it->onContactWithPlayer(this);
                break;  // 一次只处理一个碰撞
            }
        }
    }
}

void Player::placeBomb() {
    if (bombs <= 0)
        return;
    auto posi = this->pos();
    QTimer::singleShot(2000, this, [this, posi]() {
        foreach (QGraphicsItem* item, scene()->items()) {
            if (auto it = dynamic_cast<Enemy*>(item)) {
                if (abs(it->pos().x() - posi.x()) > bomb_r ||
                    abs(it->pos().y() - posi.y()) > bomb_r)
                    continue;
                else
                    it->takeDamage(bombHurt);
            }
        } });
}

void Player::focusOutEvent(QFocusEvent* event) {
    QGraphicsItem::focusOutEvent(event);
    setFocus();
}
