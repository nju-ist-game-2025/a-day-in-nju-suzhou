#ifndef LEVEL_H
#define LEVEL_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include "room.h"

class Player;
class Enemy;
class Boss;
class Chest;
class QGraphicsScene;

class Level : public QObject
{
    Q_OBJECT

public:
    explicit Level(Player *player, QGraphicsScene *scene, QObject *parent = nullptr);
    ~Level() override;

    // 初始化关卡
    void init(int levelNumber);

    // 获取当前关卡号
    int currentLevel() const { return m_levelNumber; }

    // 检查是否所有房间都已完成
    bool isAllRoomsCompleted() const;

    // 进入下一个房间
    bool enterNextRoom();

    // 获取当前房间
    Room* currentRoom() const;

    void loadRoom(int roomIndex);
signals:
    // 关卡完成信号
    void levelCompleted(int levelNumber);
    // 玩家进入新房间信号
    void roomEntered(int roomIndex);

private:
    // 生成关卡房间布局
    void generateRooms();
    // 初始化当前房间
    void initCurrentRoom(Room* room);
    // 清理当前房间实体
    void clearCurrentRoomEntities();
    // 生成房间内的敌人
    void spawnEnemiesInRoom(int roomIndex);
    // 生成房间内的宝箱
    void spawnChestsInRoom(int roomIndex);

    int m_levelNumber;               // 当前关卡号
    QVector<Room*> m_rooms;          // 关卡中的房间集合
    int m_currentRoomIndex;          // 当前房间索引
    Player* m_player;                // 玩家引用
    QGraphicsScene* m_scene;         // 游戏场景引用
    QVector<Enemy*> m_currentEnemies;// 当前房间敌人
    QVector<Chest*> m_currentChests; // 当前房间宝箱
    QVector<bool> visited;
    QTimer *checkChange;
    int visited_count;
};

#endif // LEVEL_H
