#include "roommanager.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QtMath>
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
#include "../entities/boss.h"
#include "../entities/enemy.h"
#include "../entities/level_1/clockboom.h"
#include "../entities/player.h"
#include "../items/chest.h"
#include "../items/droppeditemfactory.h"
#include "factory/bossfactory.h"
#include "factory/enemyfactory.h"

RoomManager::RoomManager(Player* player, QGraphicsScene* scene, QObject* parent)
    : QObject(parent), m_player(player), m_scene(scene) {
}

RoomManager::~RoomManager() {
    cleanup();
}

void RoomManager::cleanup() {
    // 清理定时器
    if (m_checkChangeTimer) {
        m_checkChangeTimer->stop();
        delete m_checkChangeTimer;
        m_checkChangeTimer = nullptr;
    }

    // 断开所有敌人信号
    for (QPointer<Enemy> ePtr : m_currentEnemies) {
        if (Enemy* e = ePtr.data()) {
            disconnect(e, nullptr, this, nullptr);
        }
    }
    m_currentEnemies.clear();
    m_currentChests.clear();

    // 清理门对象
    for (auto it = m_roomDoors.begin(); it != m_roomDoors.end(); ++it) {
        qDeleteAll(it.value());
    }
    m_roomDoors.clear();
    m_currentDoors.clear();

    // 清理房间对象
    qDeleteAll(m_rooms);
    m_rooms.clear();
    m_visited.clear();
    m_visitedCount = 0;
}

bool RoomManager::init(int levelNumber) {
    cleanup();
    m_levelNumber = levelNumber;
    m_hasEncounteredBossDoor = false;
    m_bossDoorsAlreadyOpened = false;

    LevelConfig config;
    if (!config.loadFromFile(levelNumber)) {
        qWarning() << "RoomManager: 加载关卡配置失败";
        return false;
    }

    return createRooms(config);
}

bool RoomManager::createRooms(const LevelConfig& config) {
    int roomCount = config.getRoomCount();
    m_rooms.resize(roomCount);
    m_visited.resize(roomCount);
    m_visited.fill(false);

    for (int i = 0; i < roomCount; ++i) {
        const RoomConfig& roomCfg = config.getRoom(i);
        Room* room = new Room();
        // Room类使用setBattleRoom，并通过检查hasBoss来判断是否战斗房间
        room->setBattleRoom(roomCfg.hasBoss || roomCfg.enemyCount > 0);
        m_rooms[i] = room;
    }

    // 标记起始房间已访问
    int startRoom = config.getStartRoomIndex();
    if (startRoom >= 0 && startRoom < roomCount) {
        m_currentRoomIndex = startRoom;
        m_visited[startRoom] = true;
        m_visitedCount = 1;
    }

    return true;
}

Room* RoomManager::currentRoom() const {
    if (m_currentRoomIndex >= 0 && m_currentRoomIndex < m_rooms.size()) {
        return m_rooms[m_currentRoomIndex];
    }
    return nullptr;
}

Room* RoomManager::getRoom(int index) const {
    if (index >= 0 && index < m_rooms.size()) {
        return m_rooms[index];
    }
    return nullptr;
}

bool RoomManager::loadRoom(int roomIndex) {
    if (roomIndex < 0 || roomIndex >= m_rooms.size()) {
        qWarning() << "RoomManager: 无效的房间索引:" << roomIndex;
        return false;
    }

    if (!m_visited[roomIndex]) {
        qWarning() << "RoomManager: 房间" << roomIndex << "未被访问过";
        return false;
    }

    // 停止所有房间的定时器
    for (Room* r : m_rooms) {
        if (r)
            r->stopChangeTimer();
    }

    clearSceneEntities();
    m_currentRoomIndex = roomIndex;
    Room* targetRoom = m_rooms[roomIndex];

    // 更新背景和门
    LevelConfig config;
    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig& roomCfg = config.getRoom(roomIndex);
        updateBackground(roomCfg.backgroundImage);
        spawnDoors(roomCfg);
    }

    // 恢复敌人
    for (QPointer<Enemy> enemyPtr : targetRoom->currentEnemies) {
        if (Enemy* enemy = enemyPtr.data()) {
            m_scene->addItem(enemy);
            m_currentEnemies.append(enemyPtr);
        }
    }

    // 恢复宝箱
    for (QPointer<Chest> chestPtr : targetRoom->currentChests) {
        if (Chest* chest = chestPtr.data()) {
            m_scene->addItem(chest);
            m_currentChests.append(chestPtr);
        }
    }

    // 恢复掉落物品
    targetRoom->restoreDroppedItemsToScene(m_scene);

    if (m_player) {
        m_player->setZValue(100);
    }

    emit roomEntered(roomIndex);

    if (targetRoom) {
        targetRoom->startChangeTimer();
    }

    return true;
}

int RoomManager::enterNextRoom(Door::Direction direction) {
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        return -1;
    }

    const RoomConfig& roomCfg = config.getRoom(m_currentRoomIndex);
    int nextRoomIndex = -1;

    switch (direction) {
        case Door::Up:
            nextRoomIndex = roomCfg.doorUp;
            break;
        case Door::Down:
            nextRoomIndex = roomCfg.doorDown;
            break;
        case Door::Left:
            nextRoomIndex = roomCfg.doorLeft;
            break;
        case Door::Right:
            nextRoomIndex = roomCfg.doorRight;
            break;
    }

    if (nextRoomIndex < 0 || nextRoomIndex >= m_rooms.size()) {
        return -1;
    }

    // 保存当前房间状态
    Room* currentRoom = m_rooms[m_currentRoomIndex];
    if (currentRoom) {
        currentRoom->saveDroppedItemsFromScene(m_scene);
    }

    // 清理场景
    clearSceneEntities();

    // 切换房间
    m_currentRoomIndex = nextRoomIndex;

    // 标记访问
    if (!m_visited[nextRoomIndex]) {
        m_visited[nextRoomIndex] = true;
        m_visitedCount++;
    }

    return nextRoomIndex;
}

bool RoomManager::isRoomVisited(int roomIndex) const {
    if (roomIndex >= 0 && roomIndex < m_visited.size()) {
        return m_visited[roomIndex];
    }
    return false;
}

void RoomManager::markRoomVisited(int roomIndex) {
    if (roomIndex >= 0 && roomIndex < m_visited.size()) {
        if (!m_visited[roomIndex]) {
            m_visited[roomIndex] = true;
            m_visitedCount++;
        }
    }
}

bool RoomManager::areAllNonBossRoomsCompleted() const {
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        return false;
    }

    for (int i = 0; i < m_rooms.size(); ++i) {
        const RoomConfig& roomCfg = config.getRoom(i);
        if (roomCfg.hasBoss) {
            continue;  // 跳过Boss房间
        }

        if (!m_visited[i]) {
            return false;  // 还有未访问的房间
        }

        Room* room = m_rooms[i];
        if (room && room->isBattleRoom() && !room->currentEnemies.isEmpty()) {
            return false;  // 战斗房间还有敌人
        }
    }

    return true;
}

void RoomManager::spawnDoors(const RoomConfig& roomCfg) {
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        qWarning() << "RoomManager: 无法加载关卡配置";
        return;
    }

    // 检查是否已有该房间的门
    if (m_roomDoors.contains(m_currentRoomIndex)) {
        m_currentDoors = m_roomDoors[m_currentRoomIndex];
        Room* room = m_rooms[m_currentRoomIndex];

        for (Door* door : m_currentDoors) {
            if (door) {
                m_scene->addItem(door);

                // 更新门状态
                if (door->state() != Door::Open && room) {
                    bool shouldBeOpen = false;
                    if (door->direction() == Door::Up && room->isDoorOpenUp())
                        shouldBeOpen = true;
                    else if (door->direction() == Door::Down && room->isDoorOpenDown())
                        shouldBeOpen = true;
                    else if (door->direction() == Door::Left && room->isDoorOpenLeft())
                        shouldBeOpen = true;
                    else if (door->direction() == Door::Right && room->isDoorOpenRight())
                        shouldBeOpen = true;

                    if (shouldBeOpen) {
                        door->setOpenState();
                    }
                }
            }
        }
        return;
    }

    // 首次创建门
    m_currentDoors.clear();
    Room* currentRoom = m_rooms[m_currentRoomIndex];

    auto createDoor = [&](int targetRoom, Door::Direction dir, int x, int y) {
        if (targetRoom < 0)
            return;

        bool isBossDoor = config.getRoom(targetRoom).hasBoss;
        Door* door = new Door(dir, isBossDoor);
        door->setPos(x, y);
        m_scene->addItem(door);
        m_currentDoors.append(door);

        // 检查是否应该已经打开
        bool shouldBeOpen = false;
        if (currentRoom) {
            switch (dir) {
                case Door::Up:
                    shouldBeOpen = currentRoom->isDoorOpenUp();
                    break;
                case Door::Down:
                    shouldBeOpen = currentRoom->isDoorOpenDown();
                    break;
                case Door::Left:
                    shouldBeOpen = currentRoom->isDoorOpenLeft();
                    break;
                case Door::Right:
                    shouldBeOpen = currentRoom->isDoorOpenRight();
                    break;
            }
        }
        if (shouldBeOpen) {
            door->setOpenState();
        }
    };

    createDoor(roomCfg.doorUp, Door::Up, 340, 0);
    createDoor(roomCfg.doorDown, Door::Down, 340, 520);
    createDoor(roomCfg.doorLeft, Door::Left, 0, 240);
    createDoor(roomCfg.doorRight, Door::Right, 720, 240);

    m_roomDoors[m_currentRoomIndex] = m_currentDoors;
}

void RoomManager::openDoors() {
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        return;
    }

    const RoomConfig& roomCfg = config.getRoom(m_currentRoomIndex);
    Room* cur = m_rooms[m_currentRoomIndex];
    bool up = false, down = false, left = false, right = false;
    int doorIndex = 0;

    auto tryOpenDoor = [&](int neighborRoom, bool& flag, void (Room::*setOpen)(bool), Door::Direction dir) {
        if (neighborRoom < 0) {
            return;
        }

        const RoomConfig& neighborCfg = config.getRoom(neighborRoom);

        // Boss门特殊处理
        if (neighborCfg.hasBoss && !canOpenBossDoor()) {
            qDebug() << "RoomManager: 门通往Boss房间" << neighborRoom << "，但条件不满足";
            doorIndex++;
            return;
        }

        if (cur)
            (cur->*setOpen)(true);
        flag = true;

        if (doorIndex < m_currentDoors.size()) {
            m_currentDoors[doorIndex]->open();
        }
        doorIndex++;

        // 设置邻居房间对应门的状态
        if (neighborRoom >= 0 && neighborRoom < m_rooms.size()) {
            Room* neighbor = m_rooms[neighborRoom];
            bool shouldOpenNeighbor = !neighbor->isBattleRoom() || m_visited[neighborRoom];
            if (shouldOpenNeighbor) {
                switch (dir) {
                    case Door::Up:
                        neighbor->setDoorOpenDown(true);
                        break;
                    case Door::Down:
                        neighbor->setDoorOpenUp(true);
                        break;
                    case Door::Left:
                        neighbor->setDoorOpenRight(true);
                        break;
                    case Door::Right:
                        neighbor->setDoorOpenLeft(true);
                        break;
                }
            }
        }
    };

    tryOpenDoor(roomCfg.doorUp, up, &Room::setDoorOpenUp, Door::Up);
    tryOpenDoor(roomCfg.doorDown, down, &Room::setDoorOpenDown, Door::Down);
    tryOpenDoor(roomCfg.doorLeft, left, &Room::setDoorOpenLeft, Door::Left);
    tryOpenDoor(roomCfg.doorRight, right, &Room::setDoorOpenRight, Door::Right);

    emit enemiesCleared(m_currentRoomIndex, up, down, left, right);
}

void RoomManager::openBossDoors() {
    if (m_bossDoorsAlreadyOpened) {
        return;
    }

    m_bossDoorsAlreadyOpened = true;
    qDebug() << "RoomManager: 打开Boss门";

    // 遍历所有门，找到Boss门并打开
    for (Door* door : m_currentDoors) {
        if (door && door->isBossDoor()) {
            door->open();
        }
    }

    // 更新房间状态
    LevelConfig config;
    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig& roomCfg = config.getRoom(m_currentRoomIndex);
        Room* cur = m_rooms[m_currentRoomIndex];

        if (roomCfg.doorUp >= 0 && config.getRoom(roomCfg.doorUp).hasBoss) {
            cur->setDoorOpenUp(true);
        }
        if (roomCfg.doorDown >= 0 && config.getRoom(roomCfg.doorDown).hasBoss) {
            cur->setDoorOpenDown(true);
        }
        if (roomCfg.doorLeft >= 0 && config.getRoom(roomCfg.doorLeft).hasBoss) {
            cur->setDoorOpenLeft(true);
        }
        if (roomCfg.doorRight >= 0 && config.getRoom(roomCfg.doorRight).hasBoss) {
            cur->setDoorOpenRight(true);
        }
    }

    emit bossDoorsOpened();
}

bool RoomManager::canOpenBossDoor() const {
    return areAllNonBossRoomsCompleted();
}

void RoomManager::spawnEnemiesInRoom() {
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        qWarning() << "RoomManager: 加载关卡配置失败";
        return;
    }

    const RoomConfig& roomCfg = config.getRoom(m_currentRoomIndex);
    Room* cur = m_rooms[m_currentRoomIndex];

    // 如果房间已清除，跳过
    if (cur && cur->isCleared()) {
        qDebug() << "房间" << m_currentRoomIndex << "已被清除，跳过敌人生成";
        openDoors();
        return;
    }

    try {
        bool hasBoss = roomCfg.hasBoss;

        // 计算敌人数量
        int totalEnemyCount = 0;
        for (const EnemySpawnConfig& enemyCfg : roomCfg.enemies) {
            totalEnemyCount += enemyCfg.count;
        }

        if (totalEnemyCount == 0 && (!cur->isBattleRoom() || cur->isBattleStarted())) {
            openDoors();
        }

        // 根据配置生成各种类型的敌人
        for (const EnemySpawnConfig& enemyCfg : roomCfg.enemies) {
            QString enemyType = enemyCfg.type;
            int count = enemyCfg.count;

            // 特殊处理ClockBoom类型
            if (enemyType == "clock_boom") {
                int enemySize = ConfigManager::instance().getEntitySize("enemies", "clock_boom");
                if (enemySize <= 0)
                    enemySize = 40;
                QPixmap boomNormalPic = ResourceFactory::createEnemyImage(enemySize, m_levelNumber, "clock_boom");

                for (int i = 0; i < count; ++i) {
                    int x = QRandomGenerator::global()->bounded(100, 700);
                    int y = QRandomGenerator::global()->bounded(100, 500);

                    if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100) {
                        x += 150;
                        y += 150;
                    }

                    ClockBoom* boom = new ClockBoom(boomNormalPic, boomNormalPic, 1.0);
                    boom->setPos(x, y);
                    boom->setPlayer(m_player);
                    boom->preloadCollisionMask();

                    m_scene->addItem(boom);

                    QPointer<Enemy> eptr(boom);
                    m_currentEnemies.append(eptr);
                    m_rooms[m_currentRoomIndex]->currentEnemies.append(eptr);

                    emit enemyCreated(boom);
                    qDebug() << "创建ClockBoom，房间" << m_currentRoomIndex << "，编号" << i << "，位置:" << x << "," << y;
                }
            } else {
                // 普通敌人生成
                int enemySize = ConfigManager::instance().getEntitySize("enemies", enemyType);
                if (enemySize <= 0)
                    enemySize = 40;

                QPixmap enemyPix;
                double enemyScale = 1.0;

                // 对于ScalingEnemy类型（Level 3的缩放敌人），使用高分辨率图片
                if (m_levelNumber == 3 && (enemyType == "optimization" || enemyType == "digital_system" || enemyType == "yanglin" || enemyType == "probability_theory")) {
                    enemyPix = ResourceFactory::createEnemyImageHighRes(m_levelNumber, enemyType, 200);
                    enemyScale = static_cast<double>(enemySize) / static_cast<double>(enemyPix.width());
                    qDebug() << "使用高分辨率图片创建ScalingEnemy:" << enemyType
                             << "图片尺寸:" << enemyPix.width() << "x" << enemyPix.height()
                             << "基础scale:" << enemyScale;
                } else {
                    enemyPix = ResourceFactory::createEnemyImage(enemySize, m_levelNumber, enemyType);
                }

                for (int i = 0; i < count; ++i) {
                    int x, y;

                    // probability_theory固定刷新在地图正中间
                    if (enemyType == "probability_theory") {
                        x = 400;
                        y = 300;
                    } else {
                        x = QRandomGenerator::global()->bounded(100, 700);
                        y = QRandomGenerator::global()->bounded(100, 500);

                        if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100) {
                            x += 150;
                            y += 150;
                        }
                    }

                    Enemy* enemy = EnemyFactory::instance().createEnemy(m_levelNumber, enemyType, enemyPix, enemyScale);
                    enemy->setPos(x, y);
                    enemy->setPlayer(m_player);

                    // 为绕圈移动的敌人设置不同的初始角度
                    if (enemyType == "yanglin" && count > 1) {
                        double initialAngle = (2.0 * M_PI / count) * i;
                        enemy->setCircleAngle(initialAngle);
                        qDebug() << "设置yanglin" << i << "初始角度:" << (initialAngle * 180.0 / M_PI) << "度";
                    }

                    enemy->preloadCollisionMask();
                    m_scene->addItem(enemy);

                    QPointer<Enemy> eptr(enemy);
                    m_currentEnemies.append(eptr);
                    m_rooms[m_currentRoomIndex]->currentEnemies.append(eptr);

                    emit enemyCreated(enemy);
                    qDebug() << "创建敌人" << enemyType << "位置:" << x << "," << y;
                }
            }
        }

        // Boss生成
        if (hasBoss) {
            QString bossType;
            if (m_levelNumber == 1) {
                bossType = "nightmare";
            } else if (m_levelNumber == 2) {
                bossType = "washmachine";
            } else {
                bossType = "teacher";
            }

            int bossSize = ConfigManager::instance().getEntitySize("bosses", bossType);
            QPixmap bossPix = ResourceFactory::createBossImage(bossSize, m_levelNumber, bossType);

            int x = QRandomGenerator::global()->bounded(100, 700);
            int y = QRandomGenerator::global()->bounded(100, 500);

            if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100) {
                x += 150;
                y += 150;
            }

            Boss* boss = BossFactory::instance().createBoss(m_levelNumber, bossPix, 1.0, m_scene);
            if (boss) {
                boss->setPos(x, y);
                boss->setPlayer(m_player);
                boss->preloadCollisionMask();

                m_scene->addItem(boss);

                QPointer<Enemy> eptr(static_cast<Enemy*>(boss));
                m_currentEnemies.append(eptr);
                m_rooms[m_currentRoomIndex]->currentEnemies.append(eptr);

                emit bossCreated(boss);
                qDebug() << "创建boss类型:" << bossType << "位置:" << x << "," << y;
            }
        }
    } catch (const QString& error) {
        qWarning() << "RoomManager: 生成敌人失败:" << error;
    }
}

void RoomManager::spawnChestsInRoom() {
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        qWarning() << "RoomManager: 加载关卡配置失败";
        return;
    }

    const RoomConfig& roomCfg = config.getRoom(m_currentRoomIndex);
    if (!roomCfg.hasChest) {
        return;
    }

    try {
        int x = QRandomGenerator::global()->bounded(150, 650);
        int y = QRandomGenerator::global()->bounded(150, 450);

        Chest* chest = nullptr;

        if (roomCfg.isChestLocked) {
            // 高级宝箱 - 需要钥匙，使用 chest_up.png
            QPixmap chestPix = ResourceFactory::createLockedChestImage(50);
            chest = new LockedChest(m_player, chestPix, 1.0);
            qDebug() << "RoomManager: 生成高级宝箱（需要钥匙）";
        } else {
            // 普通宝箱 - 无需钥匙，使用 chest.png
            QPixmap chestPix = ResourceFactory::createNormalChestImage(50);
            chest = new NormalChest(m_player, chestPix, 1.0);
            qDebug() << "RoomManager: 生成普通宝箱";
        }

        chest->setPos(x, y);
        m_scene->addItem(chest);

        QPointer<Chest> chestPtr(chest);
        m_currentChests.append(chestPtr);
        m_rooms[m_currentRoomIndex]->currentChests.append(chestPtr);

        qDebug() << "RoomManager: 创建宝箱，位置:" << x << "," << y;
    } catch (const QString& error) {
        qWarning() << "RoomManager: 生成宝箱失败:" << error;
    }
}

void RoomManager::removeEnemy(Enemy* enemy) {
    if (!enemy)
        return;

    QPointer<Enemy> target(enemy);
    m_currentEnemies.removeAll(target);

    for (Room* r : m_rooms) {
        if (r) {
            r->currentEnemies.removeAll(target);
        }
    }
}

void RoomManager::clearSceneEntities() {
    // 从场景移除敌人
    for (QPointer<Enemy> enemyPtr : m_currentEnemies) {
        if (Enemy* enemy = enemyPtr.data()) {
            if (m_scene) {
                m_scene->removeItem(enemy);
            }
        }
    }
    m_currentEnemies.clear();

    // 从场景移除宝箱
    for (QPointer<Chest> chestPtr : m_currentChests) {
        if (Chest* chest = chestPtr.data()) {
            if (m_scene) {
                m_scene->removeItem(chest);
            }
        }
    }
    m_currentChests.clear();

    // 从场景移除门
    for (Door* door : m_currentDoors) {
        if (door && m_scene) {
            m_scene->removeItem(door);
        }
    }
    // 注意：不清空m_currentDoors，只是从场景移除
}

void RoomManager::updateBackground(const QString& backgroundPath) {
    if (!m_backgroundItem)
        return;

    try {
        QString path = ConfigManager::instance().getAssetPath(backgroundPath);
        QPixmap bg = ResourceFactory::loadImage(path);
        m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        m_backgroundItem->setPos(0, 0);
    } catch (const QString& e) {
        qWarning() << "RoomManager: 加载背景失败:" << e;
    }
}

void RoomManager::initCurrentRoom(Room* room) {
    if (!room)
        return;

    room->startChangeTimer();
}

// ==================== Boss召唤敌人 ====================

void RoomManager::spawnEnemiesForBoss(const QVector<QPair<QString, int>>& enemies) {
    if (!m_scene)
        return;

    qDebug() << "RoomManager: Boss召唤敌人...";

    // 收集所有需要召唤的敌人，用于1秒后的操作
    QVector<QPointer<Enemy>> spawnedEnemies;

    for (const auto& enemyPair : enemies) {
        QString enemyType = enemyPair.first;
        int count = enemyPair.second;

        // 为每种敌人类型获取独立的尺寸配置
        int enemySize = ConfigManager::instance().getEntitySize("enemies", enemyType);
        if (enemySize <= 0)
            enemySize = 40;  // 默认值

        qDebug() << "RoomManager: 召唤" << count << "个" << enemyType << "尺寸:" << enemySize;

        if (enemyType == "clock_boom") {
            // 召唤clock_boom - 使用正确的图片路径
            QPixmap boomPic = ResourceFactory::createEnemyImage(enemySize, m_levelNumber, "clock_boom");

            // 如果加载失败，尝试直接加载
            if (boomPic.isNull()) {
                boomPic = QPixmap("assets/enemy/level_1/clock_boom.png");
                if (!boomPic.isNull()) {
                    boomPic = boomPic.scaled(enemySize, enemySize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
            }

            for (int i = 0; i < count; ++i) {
                // 立即随机生成在场景中
                int x = QRandomGenerator::global()->bounded(100, 700);
                int y = QRandomGenerator::global()->bounded(100, 500);

                ClockBoom* boom = new ClockBoom(boomPic, boomPic, 1.0);
                boom->setPos(x, y);
                boom->setPlayer(m_player);
                boom->setIsSummoned(true);  // 标记为召唤的敌人，不触发bonus

                // 预加载碰撞掩码
                boom->preloadCollisionMask();

                // 暂停敌人的AI和移动（等待1秒）
                boom->pauseTimers();

                m_scene->addItem(boom);

                QPointer<Enemy> eptr(boom);
                m_currentEnemies.append(eptr);
                spawnedEnemies.append(eptr);
                if (m_currentRoomIndex >= 0 && m_currentRoomIndex < m_rooms.size()) {
                    m_rooms[m_currentRoomIndex]->currentEnemies.append(eptr);
                }

                // 发射信号让Level连接敌人死亡回调
                emit enemySummoned(boom);
            }
        } else if (enemyType == "clock_normal") {
            // 召唤clock_normal
            QPixmap normalPic = ResourceFactory::createEnemyImage(enemySize, m_levelNumber, "clock_normal");

            for (int i = 0; i < count; ++i) {
                // 立即随机生成在场景中
                int x = QRandomGenerator::global()->bounded(100, 700);
                int y = QRandomGenerator::global()->bounded(100, 500);

                Enemy* enemy = EnemyFactory::instance().createEnemy(m_levelNumber, "clock_normal", normalPic, 1.0);
                enemy->setPos(x, y);
                enemy->setPlayer(m_player);
                enemy->setIsSummoned(true);  // 标记为召唤的敌人，不触发bonus

                // 预加载碰撞掩码
                enemy->preloadCollisionMask();

                // 暂停敌人的AI和移动（等待1秒）
                enemy->pauseTimers();

                m_scene->addItem(enemy);

                QPointer<Enemy> eptr(enemy);
                m_currentEnemies.append(eptr);
                spawnedEnemies.append(eptr);
                if (m_currentRoomIndex >= 0 && m_currentRoomIndex < m_rooms.size()) {
                    m_rooms[m_currentRoomIndex]->currentEnemies.append(eptr);
                }

                // 发射信号让Level连接敌人死亡回调
                emit enemySummoned(enemy);
            }
        } else if (enemyType == "pillow") {
            // 召唤pillow
            QPixmap pillowPic = ResourceFactory::createEnemyImage(enemySize, m_levelNumber, "pillow");

            for (int i = 0; i < count; ++i) {
                // 立即随机生成在场景中
                int x = QRandomGenerator::global()->bounded(100, 700);
                int y = QRandomGenerator::global()->bounded(100, 500);

                Enemy* enemy = EnemyFactory::instance().createEnemy(m_levelNumber, "pillow", pillowPic, 1.0);
                enemy->setPos(x, y);
                enemy->setPlayer(m_player);
                enemy->setIsSummoned(true);  // 标记为召唤的敌人，不触发bonus

                // 预加载碰撞掩码
                enemy->preloadCollisionMask();

                // 暂停敌人的AI和移动（等待1秒）
                enemy->pauseTimers();

                m_scene->addItem(enemy);

                QPointer<Enemy> eptr(enemy);
                m_currentEnemies.append(eptr);
                spawnedEnemies.append(eptr);
                if (m_currentRoomIndex >= 0 && m_currentRoomIndex < m_rooms.size()) {
                    m_rooms[m_currentRoomIndex]->currentEnemies.append(eptr);
                }

                // 发射信号让Level连接敌人死亡回调
                emit enemySummoned(enemy);
            }
        }
    }

    // 1秒后：恢复所有敌人的移动，并让ClockBoom进入引爆动画
    QTimer::singleShot(1000, this, [spawnedEnemies]() {
        for (QPointer<Enemy> ePtr : spawnedEnemies) {
            if (Enemy* e = ePtr.data()) {
                // 恢复敌人的AI和移动
                e->resumeTimers();

                // 如果是ClockBoom，立即触发倒计时（进入引爆动画）
                ClockBoom* boom = dynamic_cast<ClockBoom*>(e);
                if (boom) {
                    boom->triggerCountdown();
                }
            }
        }
        qDebug() << "RoomManager: 召唤完成，所有敌人开始移动，ClockBoom进入引爆动画";
    });

    qDebug() << "RoomManager: Boss召唤敌人完成，当前场上敌人数:" << m_currentEnemies.size();
}

// ==================== 物品掉落管理 ====================

void RoomManager::dropRandomItem(QPointF position) {
    if (!m_scene || !m_player)
        return;

    DroppedItemFactory::dropRandomItem(ItemDropPool::ENEMY_DROP, position, m_player, m_scene);
    qDebug() << "RoomManager: 在位置" << position << "掉落随机物品";
}

void RoomManager::dropItemsFromPosition(QPointF position, int count, bool scatter) {
    if (!m_scene || !m_player || count <= 0)
        return;

    Q_UNUSED(scatter)  // 工厂类自动处理散开效果
    DroppedItemFactory::dropItemsScattered(ItemDropPool::ENEMY_DROP, position, count, m_player, m_scene);
    qDebug() << "RoomManager: 从位置" << position << "掉落" << count << "个物品";
}

void RoomManager::restoreDroppedItemsToScene() {
    Room* room = currentRoom();
    if (room && m_scene) {
        room->restoreDroppedItemsToScene(m_scene);
        qDebug() << "RoomManager: 恢复掉落物品到场景";
    }
}
