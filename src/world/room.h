#ifndef ROOM_H
#define ROOM_H

#include <QPixmap>
#include <QTimer>
#include <QPointer>
#include "chest.h"
#include "enemy.h"
#include "player.h"

class Room : public QGraphicsScene
{
    Q_OBJECT

    int door_size;
    int change_x;
    int change_y;
    QTimer *changeTimer;
    Player *player;
    bool up, down, left, right; // 各个方向上是否有门存在
    // 门的是否“打开”状态（是否可通行）
    bool openUp;
    bool openDown;
    bool openLeft;
    bool openRight;
    bool m_isBattleRoom;  // 是否是战斗房间
    bool m_battleStarted; // 战斗是否已开始（首次进入触发）

public:
    Room();
    ~Room(); // 析构函数，清理敌人和宝箱

    Room(Player *p, bool u, bool d, bool l, bool r);

    QVector<QPointer<Enemy>> currentEnemies; // 当前房间敌人
    QVector<QPointer<Chest>> currentChests;  // 当前房间宝箱
    int getChangeX() { return change_x; };

    int getChangeY() { return change_y; };

    void resetChangeDir()
    {
        change_x = 0;
        change_y = 0;
    };

    // 控制每个房间的切换检测定时器（Level 负责启动/停止）
    void startChangeTimer();

    void stopChangeTimer();

    // 门控制
    void setDoorOpenUp(bool v);

    void setDoorOpenDown(bool v);

    void setDoorOpenLeft(bool v);

    void setDoorOpenRight(bool v);

    bool isDoorOpenUp() const;

    bool isDoorOpenDown() const;

    bool isDoorOpenLeft() const;

    bool isDoorOpenRight() const;

    // 战斗房间控制
    void setBattleRoom(bool isBattle);
    bool isBattleRoom() const;
    void startBattle();
    bool isBattleStarted() const;
    bool canLeaveRoom() const; // 检查是否可以离开房间

    void testChange();

signals:

    /**
     * @brief 门打开信号（用于触发开门动画）
     * @param direction 门的方向：0=上，1=下，2=左，3=右
     */
    void doorOpened(int direction);
};

#endif // ROOM_H
