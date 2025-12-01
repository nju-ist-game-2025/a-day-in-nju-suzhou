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
class Chest;
class QGraphicsScene;
class QGraphicsPixmapItem;
class DroppedItem;

/**
 * @brief 房间管理器类 - 封装房间切换、门管理、敌人生成等逻辑
 *
 * 将 Level 类中与房间管理相关的功能提取出来，降低 Level 类的复杂度。
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

    /**
     * @brief 清理所有资源
     */
    void cleanup();

    /**
     * @brief 设置关卡号（用于配置加载）
     */
    void setLevelNumber(int levelNumber) { m_levelNumber = levelNumber; }

    /**
     * @brief 获取关卡号
     */
    int levelNumber() const { return m_levelNumber; }

    // ==================== 房间操作 ====================

    /**
     * @brief 获取当前房间
     */
    Room* currentRoom() const;

    /**
     * @brief 获取当前房间索引
     */
    int currentRoomIndex() const { return m_currentRoomIndex; }

    /**
     * @brief 获取房间数量
     */
    int roomCount() const { return m_rooms.size(); }

    /**
     * @brief 获取指定索引的房间
     */
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

    /**
     * @brief 检查房间是否已访问
     */
    bool isRoomVisited(int roomIndex) const;

    /**
     * @brief 标记房间已访问
     */
    void markRoomVisited(int roomIndex);

    /**
     * @brief 检查所有非Boss房间是否都已完成（已访问且敌人清空）
     */
    bool areAllNonBossRoomsCompleted() const;

    /**
     * @brief 获取已访问房间数量
     */
    int visitedRoomCount() const { return m_visitedCount; }

    /**
     * @brief 获取已访问房间数量引用（可修改）
     */
    int& visitedCountRef() { return m_visitedCount; }

    /**
     * @brief 获取访问状态数组
     */
    const QVector<bool>& visitedRooms() const { return m_visited; }

    /**
     * @brief 获取访问状态数组引用（可修改）
     */
    QVector<bool>& visitedRoomsRef() { return m_visited; }

    /**
     * @brief 获取房间数组引用（可修改）
     */
    QVector<Room*>& roomsRef() { return m_rooms; }

    /**
     * @brief 设置当前房间索引
     */
    void setCurrentRoomIndex(int index) { m_currentRoomIndex = index; }

    // ==================== 门管理 ====================

    /**
     * @brief 生成当前房间的门
     */
    void spawnDoors(const RoomConfig& roomCfg);

    /**
     * @brief 打开当前房间的门
     */
    void openDoors();

    /**
     * @brief 打开通往Boss房间的门
     */
    void openBossDoors();

    /**
     * @brief 检查是否可以打开Boss门
     */
    bool canOpenBossDoor() const;

    /**
     * @brief 获取当前房间的门列表
     */
    const QVector<Door*>& currentDoors() const { return m_currentDoors; }

    /**
     * @brief 获取当前房间的门列表引用（可修改）
     */
    QVector<Door*>& currentDoorsRef() { return m_currentDoors; }

    /**
     * @brief 获取房间门映射引用（可修改）
     */
    QMap<int, QVector<Door*>>& roomDoorsRef() { return m_roomDoors; }

    // ==================== 敌人/宝箱管理 ====================

    /**
     * @brief 在当前房间生成敌人
     */
    void spawnEnemiesInRoom();

    /**
     * @brief 在当前房间生成宝箱
     */
    void spawnChestsInRoom();

    /**
     * @brief Boss召唤敌人
     * @param enemies 敌人类型和数量的列表
     */
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

    /**
     * @brief 恢复掉落物品到场景
     */
    void restoreDroppedItemsToScene();

    /**
     * @brief 获取当前敌人列表
     */
    QVector<QPointer<Enemy>>& currentEnemies() { return m_currentEnemies; }

    /**
     * @brief 获取当前宝箱列表
     */
    QVector<QPointer<Chest>>& currentChests() { return m_currentChests; }

    /**
     * @brief 移除敌人
     */
    void removeEnemy(Enemy* enemy);

    /**
     * @brief 清理当前房间的场景实体
     */
    void clearSceneEntities();

    // ==================== 背景管理 ====================

    /**
     * @brief 设置背景图片项
     */
    void setBackgroundItem(QGraphicsPixmapItem* item) { m_backgroundItem = item; }

    /**
     * @brief 更新房间背景
     */
    void updateBackground(const QString& backgroundPath);

    // ==================== 状态标记 ====================

    void setHasEncounteredBossDoor(bool value) { m_hasEncounteredBossDoor = value; }
    bool hasEncounteredBossDoor() const { return m_hasEncounteredBossDoor; }

    void setBossDoorsAlreadyOpened(bool value) { m_bossDoorsAlreadyOpened = value; }
    bool bossDoorsAlreadyOpened() const { return m_bossDoorsAlreadyOpened; }

   signals:
    /**
     * @brief 房间进入信号
     */
    void roomEntered(int roomIndex);

    /**
     * @brief 敌人清空信号
     */
    void enemiesCleared(int roomIndex, bool up, bool down, bool left, bool right);

    /**
     * @brief Boss门打开信号
     */
    void bossDoorsOpened();

    /**
     * @brief 敌人死亡信号（转发给Level处理）
     */
    void enemyDied(Enemy* enemy);

    /**
     * @brief 请求创建敌人
     */
    void requestCreateEnemy(int levelNumber, const QString& type, const QPixmap& pic, double scale, QPointF pos);

    /**
     * @brief 请求创建Boss
     */
    void requestCreateBoss(int levelNumber, const QPixmap& pic, double scale, QPointF pos);

    /**
     * @brief Boss召唤的敌人生成完成信号
     * @param enemy 生成的敌人
     */
    void enemySummoned(Enemy* enemy);

   private:
    /**
     * @brief 初始化当前房间
     */
    void initCurrentRoom(Room* room);

    /**
     * @brief 创建房间对象
     */
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
