#include "player.h"
#include <QDateTime>
#include <QElapsedTimer>
#include "constants.h"
#include "enemy.h"

Player::Player(const QPixmap& pic_player, double scale)
    : redContainers(3), redHearts(3.0), blackHearts(0), soulHearts(0), shootCooldown(150), lastShootTime(0), bulletHurt(2), isDead(false) {  // 默认150毫秒射击冷却，子弹伤害默认2
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
    if (!event || isDead)  // 已死亡则不处理输入
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
    // 计算子弹发射位置（从角色中心发射）
    QPointF bulletPos = this->pos() + QPointF(pixmap().width() / 2 - 7.5, pixmap().height() / 2 - 7.5);
    // if(shootType == 0) //改变为发射激光模式，需要ui的图片实现
    Projectile* bullet = new Projectile(0, bulletHurt, bulletPos, pic_bullet);  // 使用可配置的玩家子弹伤害
    bullet->setSpeed(shootSpeed);

    // 将子弹添加到场景中
    if (scene()) {
        scene()->addItem(bullet);
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
}

void Player::move() {
    if (isDead)  // 已死亡则不移动
        return;

    xdir = 0;
    ydir = 0;

    if (keysPressed[Qt::Key_W] == true)
        ydir--;
    if (keysPressed[Qt::Key_S] == true)
        ydir++;
    if (keysPressed[Qt::Key_A] == true)
        xdir--;
    if (keysPressed[Qt::Key_D] == true)
        xdir++;

    // 边界检测
    double newX = pos().x() + xdir * speed;
    double newY = pos().y() + ydir * speed;

    // 在门的区域允许玩家移动到更靠边的位置
    double doorMargin = 20.0;  // 门附近允许的额外边距
    double doorSize = 100.0;   // 门的宽度（用于判断是否在门附近）

    // 上边界：在门附近允许到达 y = -doorMargin
    if (newY < 0) {
        if (qAbs(newX + pixmap().width() / 2 - 400) < doorSize)  // 在上门附近
            newY = qMax(newY, -doorMargin);
        else
            newY = 0;
    }

    // 下边界：在门附近允许到达 y = room_bound_y - pixmap().height() + doorMargin
    if (newY > room_bound_y - pixmap().height()) {
        if (qAbs(newX + pixmap().width() / 2 - 400) < doorSize)  // 在下门附近
            newY = qMin(newY, (double)(room_bound_y - pixmap().height()) + doorMargin);
        else
            newY = room_bound_y - pixmap().height();
    }

    // 左边界：在门附近允许到达 x = -doorMargin
    if (newX < 0) {
        if (qAbs(newY + pixmap().height() / 2 - 300) < doorSize)  // 在左门附近
            newX = qMax(newX, -doorMargin);
        else
            newX = 0;
    }

    // 右边界：在门附近允许到达 x = room_bound_x - pixmap().width() + doorMargin
    if (newX > room_bound_x - pixmap().width()) {
        if (qAbs(newY + pixmap().height() / 2 - 300) < doorSize)  // 在右门附近
            newX = qMin(newX, (double)(room_bound_x - pixmap().width()) + doorMargin);
        else
            newX = room_bound_x - pixmap().width();
    }

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
    if (isDead || invincible)  // 已死亡或无敌则不受伤
        return;

    flash();
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
    QTimer::singleShot(1000, this, [this]() {
        invincible = false;
    });
}

void Player::crashEnemy() {
    if (isDead)  // 已死亡则不检测碰撞
        return;

    foreach (QGraphicsItem* item, scene()->items()) {
        if (auto it = dynamic_cast<Enemy*>(item)) {
            if (abs(it->pos().x() - this->pos().x()) > it->crash_r + crash_r ||
                abs(it->pos().y() - this->pos().y()) > it->crash_r + crash_r)
                continue;
            else
                this->takeDamage(it->getContactDamage());
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
        }
    });
}

void Player::focusOutEvent(QFocusEvent* event) {
    QGraphicsItem::focusOutEvent(event);
    setFocus();
}
