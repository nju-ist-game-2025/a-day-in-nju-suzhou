#include "room.h"

Room::Room() {}

Room::Room(Player *p, bool u, bool d, bool l, bool r):
    up(u), down(d), left(l), right(r), door_size(100)
{
    player = p;
    change_x = 0;
    change_y = 0;

    keysPressed[Qt::Key_W] = false;
    keysPressed[Qt::Key_A] = false;
    keysPressed[Qt::Key_S] = false;
    keysPressed[Qt::Key_D] = false;
    int sz = door_size/2;
    changeTimer = new QTimer(this);
    connect(changeTimer, &QTimer::timeout, this, &Room::testChange);
    changeTimer->start(12);
}

void Room::keyPressEvent(QKeyEvent* event) {
    if (!event) return;
    if (keysPressed.count(event->key())) {
        keysPressed[event->key()] = true;
        event->accept();
        return;
    }
}

void Room::keyReleaseEvent(QKeyEvent* event) {
    if (!event) return;
    if (keysPressed.count(event->key())) {
        keysPressed[event->key()] = false;
    }
}

void Room::testChange() {
    int sz = door_size/2;
    int x = player->pos().x(), y = player->pos().y();
    if(up && y <= 40 && abs(x - 400) < sz && keysPressed[Qt::Key_W]) change_y = -1;
    if(down && y >= 560 && abs(x - 400) < sz && keysPressed[Qt::Key_S]) change_y = 1;
    if(left && x <= 40 && abs(y - 300) < sz && keysPressed[Qt::Key_A]) change_x = -1;
    if(right && x >= 760 && abs(y - 300) < sz && keysPressed[Qt::Key_D]) change_x = 1;
}
