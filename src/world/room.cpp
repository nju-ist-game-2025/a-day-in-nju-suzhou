#include <QDebug>
#include "room.h"

Room::Room() {}

Room::Room(Player *p, bool u, bool d, bool l, bool r) : up(u), down(d), left(l), right(r), door_size(100) {
    player = p;
    change_x = 0;
    change_y = 0;

    // 默认门为关闭状态（不可通行），Level 会在合适时机打开
    openUp = false;
    openDown = false;
    openLeft = false;
    openRight = false;

    keysPressed[Qt::Key_W] = false;
    keysPressed[Qt::Key_A] = false;
    keysPressed[Qt::Key_S] = false;
    keysPressed[Qt::Key_D] = false;
    changeTimer = new QTimer(this);
    connect(changeTimer, &QTimer::timeout, this, &Room::testChange);
}

void Room::startChangeTimer() {
    if (changeTimer)
        changeTimer->start(12);
}

void Room::stopChangeTimer() {
    if (changeTimer)
        changeTimer->stop();
}

void Room::setDoorOpenUp(bool v) {
    if (!openUp && v) {
        // 门从关闭变为打开，发射开门信号（用于触发动画）
        emit doorOpened(0);  // 0 = 上
    }
    openUp = v;
}

void Room::setDoorOpenDown(bool v) {
    if (!openDown && v) {
        emit doorOpened(1);  // 1 = 下
    }
    openDown = v;
    qDebug() << "setDoorOpenDown 被调用，down=" << down << ", openDown=" << openDown;
}

void Room::setDoorOpenLeft(bool v) {
    if (!openLeft && v) {
        emit doorOpened(2);  // 2 = 左
    }
    openLeft = v;
}

void Room::setDoorOpenRight(bool v) {
    if (!openRight && v) {
        emit doorOpened(3);  // 3 = 右
    }
    openRight = v;
}

bool Room::isDoorOpenUp() const {
    return openUp;
}

bool Room::isDoorOpenDown() const {
    return openDown;
}

bool Room::isDoorOpenLeft() const {
    return openLeft;
}

bool Room::isDoorOpenRight() const {
    return openRight;
}

void Room::keyPressEvent(QKeyEvent *event) {
    if (!event)
        return;
    if (keysPressed.count(event->key())) {
        keysPressed[event->key()] = true;
        event->accept();
        return;
    }
}

void Room::keyReleaseEvent(QKeyEvent *event) {
    if (!event)
        return;
    if (keysPressed.count(event->key())) {
        keysPressed[event->key()] = false;
    }
}

void Room::testChange() {
    int sz = door_size / 2;
    int x = player->pos().x(), y = player->pos().y();

    // 重置切换方向
    change_x = 0;
    change_y = 0;

    if (up && openUp && y <= 40 && qAbs(x - 400) < sz && keysPressed[Qt::Key_W]) {
        change_y = -1;
        qDebug() << "检测到向上切换请求，玩家位置:" << x << "," << y;
    }
    if (down && openDown && y >= 560 && qAbs(x - 400) < sz && keysPressed[Qt::Key_S]) {
        change_y = 1;
        qDebug() << "检测到向下切换请求，玩家位置:" << x << "," << y;
    }
    if (left && openLeft && x <= 40 && qAbs(y - 300) < sz && keysPressed[Qt::Key_A]) {
        change_x = -1;
        qDebug() << "检测到向左切换请求，玩家位置:" << x << "," << y;
    }
    if (right && openRight && x >= 760 && qAbs(y - 300) < sz && keysPressed[Qt::Key_D]) {
        change_x = 1;
        qDebug() << "检测到向右切换请求，玩家位置:" << x << "," << y;
    }
}
