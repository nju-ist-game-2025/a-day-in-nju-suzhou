#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H

#include <QMap>
#include <QObject>
#include <QPair>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include "door.h"
#include "levelconfig.h"
#include "room.h"

class Player;
class Enemy;
class Boss;
class Chest;
class QGraphicsScene;
class QGraphicsPixmapItem;
class DroppedItem;

/**
 * @brief 房间管理器类 - 封装房间切换、门管理、敌人生成等逻辑
 *
 * 职责：
 *   - 房间初始化和切换
 *   - 门的创建、状态管理和开关控制
 *   - 敌人和宝箱的生成
 *   - 房间访问状态跟踪
 *   - 物品掉落管理
 */
class RoomManager : public QObject {
    Q_OBJECT

   public:
    explicit RoomManager(Player* player, QGraphicsScene* scene, QObject* parent = nullptr);
    ~RoomManager() override;

    /**
     * @brief 初始化房间管理器
     * @param levelNumber 关卡号
     * @return 是否初始化成功
     */
    bool init(int levelNumber);
    // 清理所有资源
    void cleanup();
    // 用于配置加载
    void setLevelNumber(int levelNumber) { m_levelNumber = levelNumber; }
    int levelNumber() const { return m_levelNumber; }

    // ==================== 房间操作 ====================

    // 获取当前房间
    Room* currentRoom() const;
    int currentRoomIndex() const { return m_currentRoomIndex; }
    int roomCount() const { return m_rooms.size(); }

    // 获取指定索引的房间
    Room* getRoom(int index) const;

    /**
     * @brief 加载指定房间
     * @param roomIndex 房间索引
     * @return 是否加载成功
     */
    bool loadRoom(int roomIndex);

    /**
     * @brief 进入下一个房间
     * @param direction 方向
     * @return 新房间索引，-1表示无法移动
     */
    int enterNextRoom(Door::Direction direction);
    bool isRoomVisited(int roomIndex) const;
    void markRoomVisited(int roomIndex);
    bool areAllNonBossRoomsCompleted() const;

    // 获取已访问房间数量
    int visitedRoomCount() const { return m_visitedCount; }

    // 获取已访问房间数量引用
    int& visitedCountRef() { return m_visitedCount; }
    // 获取访问状态数组
    const QVector<bool>& visitedRooms() const { return m_visited; }
    QVector<bool>& visitedRoomsRef() { return m_visited; }
    QVector<Room*>& roomsRef() { return m_rooms; }
    void setCurrentRoomIndex(int index) { m_currentRoomIndex = index; }

    // ==================== 门管理 ====================

    void spawnDoors(const RoomConfig& roomCfg);
    void openDoors();
    void openBossDoors();

    // 检查是否可以打开Boss门
    bool canOpenBossDoor() const;

    // 获取当前房间的门列表
    const QVector<Door*>& currentDoors() const { return m_currentDoors; }
    // 获取当前房间的门列表引用
    QVector<Door*>& currentDoorsRef() { return m_currentDoors; }
    // 获取房间门映射引用
    QMap<int, QVector<Door*>>& roomDoorsRef() { return m_roomDoors; }

    // ==================== 敌人/宝箱管理 ====================

    void spawnEnemiesInRoom();
    void spawnChestsInRoom();
    void spawnEnemiesForBoss(const QVector<QPair<QString, int>>& enemies);

    // ==================== 物品掉落管理 ====================

    /**
     * @brief 在指定位置掉落随机物品
     * @param position 掉落位置
     */
    void dropRandomItem(QPointF position);

    /**
     * @brief 从指定位置掉落多个物品（散开效果）
     * @param position 掉落位置
     * @param count 物品数量
     * @param scatter 是否散开
     */
    void dropItemsFromPosition(QPointF position, int count, bool scatter = true);
    void restoreDroppedItemsToScene();
    QVector<QPointer<Enemy>>& currentEnemies() { return m_currentEnemies; }
    QVector<QPointer<Chest>>& currentChests() { return m_currentChests; }
    void removeEnemy(Enemy* enemy);
    void clearSceneEntities();

    // ==================== 背景管理 ====================

    void setBackgroundItem(QGraphicsPixmapItem* item) { m_backgroundItem = item; }
    void updateBackground(const QString& backgroundPath);

    // ==================== 状态标记 ====================

    void setHasEncounteredBossDoor(bool value) { m_hasEncounteredBossDoor = value; }
    bool hasEncounteredBossDoor() const { return m_hasEncounteredBossDoor; }

    void setBossDoorsAlreadyOpened(bool value) { m_bossDoorsAlreadyOpened = value; }
    bool bossDoorsAlreadyOpened() const { return m_bossDoorsAlreadyOpened; }

   signals:
    void roomEntered(int roomIndex);
    void enemiesCleared(int roomIndex, bool up, bool down, bool left, bool right);
    void bossDoorsOpened();
    void enemyDied(Enemy* enemy);
    void enemyCreated(Enemy* enemy);  // 敌人创建信号，Level连接dying信号
    void bossCreated(Boss* boss);     // Boss创建信号
    void enemySummoned(Enemy* enemy);

   private:
    void initCurrentRoom(Room* room);
    bool createRooms(const LevelConfig& config);

    int m_levelNumber = 0;
    int m_currentRoomIndex = 0;
    int m_visitedCount = 0;

    Player* m_player = nullptr;
    QGraphicsScene* m_scene = nullptr;

    QVector<Room*> m_rooms;
    QVector<bool> m_visited;
    QVector<QPointer<Enemy>> m_currentEnemies;
    QVector<QPointer<Chest>> m_currentChests;
    QVector<Door*> m_currentDoors;
    QMap<int, QVector<Door*>> m_roomDoors;

    QGraphicsPixmapItem* m_backgroundItem = nullptr;

    // Boss门相关状态
    bool m_hasEncounteredBossDoor = false;
    bool m_bossDoorsAlreadyOpened = false;

    // 房间切换检测定时器
    QTimer* m_checkChangeTimer = nullptr;
};

#endif  // ROOMMANAGER_H
