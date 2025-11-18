#include "room.h"

Room::Room() {}

Room::Room(Player *p, bool u, bool d, bool l, bool r):
    up(u), down(d), left(l), right(r), door_size(100), visiting(true)
{
    player = p;
    change_x = 0;
    change_y = 0;

    int sz = door_size/2;
    changeTimer = new QTimer(this);
    connect(changeTimer, &QTimer::timeout, this, &Room::testChange);
    changeTimer->start(100);
}

void Room::testChange() {
    if(!visiting) return;
    int sz = door_size/2;
    int x = player->pos().x(), y = player->pos().y();

    if(up && y <= 60 && abs(x - 370) < sz && player->keysPressed[Qt::Key_W]) change_y = -1;
    if(down && y >= 480 && abs(x - 370) < sz && player->keysPressed[Qt::Key_S]) change_y = 1;
    if(left && x <= 60 && abs(y - 270) < sz && player->keysPressed[Qt::Key_A]) change_x = -1;
    if(right && x >= 680 && abs(y - 270) < sz && player->keysPressed[Qt::Key_D]) change_x = 1;
}
