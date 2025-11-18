#ifndef ROOM_H
#define ROOM_H

#include "player.h"
#include <QPixmap>
#include <QTimer>
#include "enemy.h"
#include "chest.h"

class Room : public QGraphicsScene
{
    int door_size;
    int change_x;
    int change_y;
    QTimer *changeTimer;
    Player *player;
    bool up, down, left, right;//各个方向上是否有门存在
public:
    bool visiting;
    friend class Player;
    Room();
    Room(Player* p, bool u, bool d, bool l, bool r);
    QVector<Enemy*> currentEnemies;     // 当前房间敌人
    QVector<Chest*> currentChests;      // 当前房间宝箱
    int getChangeX() {return change_x;};
    int getChangeY() {return change_y;};
    void resetChangeDir() {change_x = 0; change_y = 0;};
    void testChange();
};

#endif // ROOM_H
