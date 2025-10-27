#include "player.h"
#include <QDateTime>
#include <QElapsedTimer>
#include "constants.h"
#include "enemy.h"

Player::Player(const QPixmap& pic_player, double scale)
    : redContainers(3), redHearts(3.0), blackHearts(0), soulHearts(0), invincible(false), shootCooldown(150), lastShootTime(0) {  // 默认150毫秒射击冷却
    setTransformationMode(Qt::SmoothTransformation);

    // 如果scale是1.0，直接使用原始pixmap，否则按比例缩放
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
    curr_xdir = 0;
    curr_ydir = 1;           // 默认向下
    this->setPos(400, 300);  // 初始位置,根据实际需要后续修改

    hurt = 1;      // 后续可修改
    crash_r = 40;  // 增大碰撞半径以匹配新的角色大小

    // 初始化移动按键
    keysPressed[Qt::Key_Up] = false;
    keysPressed[Qt::Key_Down] = false;
    keysPressed[Qt::Key_Left] = false;
    keysPressed[Qt::Key_Right] = false;

    // 初始化射击按键
    shootKeysPressed[Qt::Key_W] = false;
    shootKeysPressed[Qt::Key_A] = false;
    shootKeysPressed[Qt::Key_S] = false;
    shootKeysPressed[Qt::Key_D] = false;

    keysTimer = new QTimer();
    connect(keysTimer, &QTimer::timeout, this, &Player::move);
    keysTimer->start(16);

    crashTimer = new QTimer();
    connect(crashTimer, &QTimer::timeout, this, &Player::crashEnemy);
    crashTimer->start(50);

    // 射击检测定时器（持续检测射击按键状态）
    shootTimer = new QTimer();
    connect(shootTimer, &QTimer::timeout, this, &Player::checkShoot);
    shootTimer->start(16);  // 每16ms检测一次
}

void Player::keyPressEvent(QKeyEvent* event) {
    if (!event)
        return;

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
    // 检查是否有射击键被按下
    int shootKey = -1;

    // 按优先级检查射击键（后检查的优先级更高）
    if (shootKeysPressed[Qt::Key_W])
        shootKey = Qt::Key_W;
    if (shootKeysPressed[Qt::Key_S])
        shootKey = Qt::Key_S;
    if (shootKeysPressed[Qt::Key_A])
        shootKey = Qt::Key_A;
    if (shootKeysPressed[Qt::Key_D])
        shootKey = Qt::Key_D;

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
    // 计算子弹发射位置（从角色中心发射）
    QPointF bulletPos = this->pos() + QPointF(pixmap().width() / 2 - 7.5, pixmap().height() / 2 - 7.5);
    Projectile* bullet = new Projectile(0, this->hurt, bulletPos, pic_bullet);

    // 将子弹添加到场景中
    if (scene()) {
        scene()->addItem(bullet);
    }

    // 设置子弹方向和速度
    switch (key) {
        case Qt::Key_W:
            bullet->setDir(0, -9);
            break;
        case Qt::Key_S:
            bullet->setDir(0, 9);
            break;
        case Qt::Key_A:
            bullet->setDir(-9, 0);
            break;
        case Qt::Key_D:
            bullet->setDir(9, 0);
            break;
        default:
            break;
    }
}

void Player::move() {
    xdir = 0;
    ydir = 0;

    if (keysPressed[Qt::Key_Up] == true)
        ydir--;
    if (keysPressed[Qt::Key_Down] == true)
        ydir++;
    if (keysPressed[Qt::Key_Left] == true)
        xdir--;
    if (keysPressed[Qt::Key_Right] == true)
        xdir++;

    // 边界检测
    double newX = pos().x() + xdir * speed;
    double newY = pos().y() + ydir * speed;

    if (newX < 0)
        newX = 0;
    if (newX > room_bound_x - pixmap().width())
        newX = room_bound_x - pixmap().width();
    if (newY < 0)
        newY = 0;
    if (newY > room_bound_y - pixmap().height())
        newY = room_bound_y - pixmap().height();

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

    this->setPos(newX, newY);
}

void Player::takeDamage(int damage) {
    if (invincible)
        return;
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
    if (redHearts <= 0.0)
        die();
    else
        setInvincible();
}

// 死亡效果，有待UI同学的具体实现
void Player::die() {
    
}

// 短暂无敌效果，有待UI同学的具体实现
void Player::setInvincible() {
    invincible = true;
    QTimer::singleShot(1000, this, [this]() {
        invincible = false;
    });
}

void Player::crashEnemy() {
    foreach (QGraphicsItem* item, scene()->items()) {
        if (auto it = dynamic_cast<Enemy*>(item)) {
            if (abs(it->pos().x() - this->pos().x()) > it->crash_r + crash_r ||
                abs(it->pos().y() - this->pos().y()) > it->crash_r + crash_r)
                continue;
            else
                it->takeDamage(it->crash_hurt);
        }
    }
}
