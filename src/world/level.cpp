#include "level.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QRandomGenerator>
#include <algorithm>
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
#include "../entities/boss.h"
#include "../entities/enemy.h"
#include "../entities/player.h"
#include "../items/chest.h"
#include "levelconfig.h"
#include "room.h"

Level::Level(Player* player, QGraphicsScene* scene, QObject* parent) : QObject(parent),
                                                                       m_levelNumber(1),
                                                                       m_currentRoomIndex(0),
                                                                       m_player(player),
                                                                       m_scene(scene),
                                                                       checkChange(nullptr),
                                                                       visited_count(0) {
}

Level::~Level() {
    if (checkChange) {
        checkChange->stop();
        delete checkChange;
    }

    // 清理所有房间
    qDeleteAll(m_rooms);
    m_rooms.clear();
}

void Level::init(int levelNumber) {
    // 停止现有定时器
    if (checkChange) {
        checkChange->stop();
        delete checkChange;
        checkChange = nullptr;
    }

    // 清理现有数据
    qDeleteAll(m_rooms);
    m_rooms.clear();
    m_currentEnemies.clear();
    m_currentChests.clear();

    m_levelNumber = levelNumber;

    // 加载关卡配置
    LevelConfig config;
    if (!config.loadFromFile(levelNumber)) {
        qWarning() << "加载关卡配置失败，使用默认配置";
        return;
    }

    qDebug() << "加载关卡:" << config.getLevelName();

    // 根据JSON配置生成房间
    for (int i = 0; i < config.getRoomCount(); ++i) {
        const RoomConfig& roomCfg = config.getRoom(i);

        // 创建房间，根据门配置设置哪些门存在
        bool hasUp = (roomCfg.doorUp >= 0);
        bool hasDown = (roomCfg.doorDown >= 0);
        bool hasLeft = (roomCfg.doorLeft >= 0);
        bool hasRight = (roomCfg.doorRight >= 0);

        Room* room = new Room(m_player, hasUp, hasDown, hasLeft, hasRight);
        m_rooms.append(room);
    }

    visited.resize(config.getRoomCount());
    visited.fill(false);
    visited[config.getStartRoomIndex()] = true;
    visited_count = 1;

    // 设置起始房间
    m_currentRoomIndex = config.getStartRoomIndex();
    const RoomConfig& startRoomCfg = config.getRoom(m_currentRoomIndex);

    // 加载起始房间背景
    try {
        QString bgPath = ConfigManager::instance().getAssetPath(startRoomCfg.backgroundImage);
        QPixmap bg = ResourceFactory::loadImage(bgPath);
        m_scene->setBackgroundBrush(QBrush(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
    } catch (const QString& e) {
        qWarning() << "加载房间背景失败:" << e;
    }

    // 初始化第一个房间
    initCurrentRoom(m_rooms[m_currentRoomIndex]);

    // 启动房间切换检测
    checkChange = new QTimer(this);
    connect(checkChange, &QTimer::timeout, this, &Level::enterNextRoom);
    checkChange->start(100);
}
void Level::initCurrentRoom(Room* room) {
    if (m_currentRoomIndex >= m_rooms.size())
        return;

    // 停止所有房间的切换检测定时器，然后启动当前房间的定时器
    for (Room* rr : m_rooms) {
        if (rr)
            rr->stopChangeTimer();
    }
    // 清理当前房间实体
    clearCurrentRoomEntities();

    // 加载当前房间背景
    LevelConfig config;
    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig& roomCfg = config.getRoom(m_currentRoomIndex);
        try {
            QString bgPath = ConfigManager::instance().getAssetPath(roomCfg.backgroundImage);
            QPixmap bg = ResourceFactory::loadImage(bgPath);
            if (m_scene)
                m_scene->setBackgroundBrush(QBrush(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
            qDebug() << "加载房间" << m_currentRoomIndex << "背景:" << roomCfg.backgroundImage;
        } catch (const QString& e) {
            qWarning() << "加载地图背景失败:" << e;
        }
    }

    // 生成当前房间的敌人
    spawnEnemiesInRoom(m_currentRoomIndex);

    // 生成当前房间的宝箱
    spawnChestsInRoom(m_currentRoomIndex);

    emit roomEntered(m_currentRoomIndex);
    qDebug() << "进入房间:" << m_currentRoomIndex;

    if (room)
        room->startChangeTimer();
}

void Level::spawnEnemiesInRoom(int roomIndex) {
    // 重新加载配置以获取房间信息
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        qWarning() << "加载关卡配置失败";
        return;
    }

    const RoomConfig& roomCfg = config.getRoom(roomIndex);

    try {
        QPixmap enemyPix = ResourceFactory::createEnemyImage(40);

        // 从配置读取敌人数量
        int enemyCount = roomCfg.enemyCount;

        for (int i = 0; i < enemyCount; ++i) {
            int x = QRandomGenerator::global()->bounded(100, 700);
            int y = QRandomGenerator::global()->bounded(100, 500);

            if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100) {
                x += 150;
                y += 150;
            }

            Enemy* enemy = new Enemy(enemyPix, 1.0);
            enemy->setPos(x, y);
            enemy->setPlayer(m_player);
            m_scene->addItem(enemy);
            m_currentEnemies.append(enemy);
            m_rooms[roomIndex]->currentEnemies.append(enemy);
            // 监听敌人死亡信号（在销毁之前发出）
            connect(enemy, &Enemy::dying, this, &Level::onEnemyDying);
            qDebug() << "创建敌人" << i << "位置:" << x << "," << y << "已连接dying信号";
        }
    } catch (const QString& error) {
        qWarning() << "生成敌人失败:" << error;
    }
}

void Level::spawnChestsInRoom(int roomIndex) {
    // 重新加载配置以获取房间信息
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        qWarning() << "加载关卡配置失败";
        return;
    }

    const RoomConfig& roomCfg = config.getRoom(roomIndex);

    // 检查是否有宝箱
    if (!roomCfg.hasChest) {
        return;
    }

    try {
        QPixmap chestPix = ResourceFactory::createChestImage(50);

        int x = QRandomGenerator::global()->bounded(150, 650);
        int y = QRandomGenerator::global()->bounded(150, 450);

        Chest* chest;
        if (roomCfg.isChestLocked) {
            chest = new lockedChest(m_player, chestPix, 1.0);
        } else {
            chest = new Chest(m_player, false, chestPix, 1.0);
        }
        chest->setPos(x, y);
        m_scene->addItem(chest);
        m_currentChests.append(chest);
        m_rooms[roomIndex]->currentChests.append(chest);
    } catch (const QString& error) {
        qWarning() << "生成宝箱失败:" << error;
    }
}

void Level::clearCurrentRoomEntities() {
    for (Enemy* enemy : m_currentEnemies) {
        // 从所有房间的列表中移除该指针，防止留存悬空指针
        for (Room* r : m_rooms) {
            if (!r)
                continue;
            r->currentEnemies.erase(std::remove(r->currentEnemies.begin(), r->currentEnemies.end(), enemy), r->currentEnemies.end());
        }
        if (m_scene && enemy)
            m_scene->removeItem(enemy);
        delete enemy;
    }
    m_currentEnemies.clear();

    for (Chest* chest : m_currentChests) {
        // 从所有房间的列表中移除该指针
        for (Room* r : m_rooms) {
            if (!r)
                continue;
            r->currentChests.erase(std::remove(r->currentChests.begin(), r->currentChests.end(), chest), r->currentChests.end());
        }
        if (m_scene && chest)
            m_scene->removeItem(chest);
        delete chest;
    }
    m_currentChests.clear();
}

bool Level::enterNextRoom() {
    Room* currentRoom = m_rooms[m_currentRoomIndex];
    int x = currentRoom->getChangeX();
    int y = currentRoom->getChangeY();

    // 如果没有切换请求，直接返回
    if (!x && !y)
        return false;

    qDebug() << "enterNextRoom: 检测到切换请求 x=" << x << ", y=" << y;

    // 加载关卡配置
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        qWarning() << "加载关卡配置失败";
        return false;
    }

    const RoomConfig& currentRoomCfg = config.getRoom(m_currentRoomIndex);
    int nextRoomIndex = -1;

    // 根据移动方向确定目标房间
    if (y == -1) {  // 向上
        nextRoomIndex = currentRoomCfg.doorUp;
    } else if (y == 1) {  // 向下
        nextRoomIndex = currentRoomCfg.doorDown;
    } else if (x == -1) {  // 向左
        nextRoomIndex = currentRoomCfg.doorLeft;
    } else if (x == 1) {  // 向右
        nextRoomIndex = currentRoomCfg.doorRight;
    }

    // 检查目标房间是否有效
    if (nextRoomIndex < 0 || nextRoomIndex >= m_rooms.size()) {
        qDebug() << "无效的目标房间:" << nextRoomIndex;
        currentRoom->resetChangeDir();
        return false;
    }

    qDebug() << "切换房间: 从" << m_currentRoomIndex << "到" << nextRoomIndex;

    // 更新当前房间索引
    m_currentRoomIndex = nextRoomIndex;

    if (y == -1) {  // 玩家请求向上进入上方房间 -> 在目标房间的下门出现
        m_player->setPos(m_player->pos().x(), 560);
    } else if (y == 1) {  // 玩家请求向下进入下方房间 -> 在目标房间的上门出现
        m_player->setPos(m_player->pos().x(), 40);
    } else if (x == -1) {  // 玩家请求向左进入左侧房间 -> 在目标房间的右门出现
        m_player->setPos(760, m_player->pos().y());
    } else if (x == 1) {  // 玩家请求向右进入右侧房间 -> 在目标房间的左门出现
        m_player->setPos(40, m_player->pos().y());
    }

    // 初始化或加载房间
    if (!visited[m_currentRoomIndex]) {
        visited[m_currentRoomIndex] = true;
        visited_count++;
        initCurrentRoom(m_rooms[m_currentRoomIndex]);
    } else {
        loadRoom(m_currentRoomIndex);
    }

    // 打开新房间的对应门（便于返回）
    const RoomConfig& nextRoomCfg = config.getRoom(nextRoomIndex);
    Room* nextRoom = m_rooms[nextRoomIndex];

    if (y == -1 && nextRoomCfg.doorDown >= 0) {
        nextRoom->setDoorOpenDown(true);
    } else if (y == 1 && nextRoomCfg.doorUp >= 0) {
        nextRoom->setDoorOpenUp(true);
    }

    currentRoom->resetChangeDir();
    return true;
}

void Level::loadRoom(int roomIndex) {
    if (roomIndex < 0 || roomIndex >= m_rooms.size()) {
        qWarning() << "无效的房间索引:" << roomIndex;
        return;
    }
    if (!visited[roomIndex]) {
        qWarning() << "房间" << roomIndex << "未被访问过，无法加载";
        return;
    }

    // 停止所有房间的定时器
    for (Room* rr : m_rooms) {
        if (rr)
            rr->stopChangeTimer();
    }

    m_currentRoomIndex = roomIndex;
    Room* targetRoom = m_rooms[roomIndex];

    // 从 JSON 配置加载背景
    LevelConfig config;
    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig& roomCfg = config.getRoom(roomIndex);
        try {
            QString bgPath = ConfigManager::instance().getAssetPath(roomCfg.backgroundImage);
            QPixmap bg = ResourceFactory::loadImage(bgPath);
            m_scene->setBackgroundBrush(QBrush(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
            qDebug() << "加载房间" << roomIndex << "背景:" << roomCfg.backgroundImage;
        } catch (const QString& e) {
            qWarning() << "加载地图背景失败:" << e;
        }
    }

    // 重新添加房间实体到场景
    for (Enemy* enemy : targetRoom->currentEnemies) {
        m_scene->addItem(enemy);
        m_currentEnemies.append(enemy);
    }

    for (Chest* chest : targetRoom->currentChests) {
        m_scene->addItem(chest);
        m_currentChests.append(chest);
    }

    if (m_player) {
        m_player->setZValue(100);
    }

    emit roomEntered(roomIndex);

    // 启动当前房间的切换检测定时器
    if (targetRoom)
        targetRoom->startChangeTimer();
}

Room* Level::currentRoom() const {
    if (m_currentRoomIndex < m_rooms.size()) {
        return m_rooms[m_currentRoomIndex];
    }
    return nullptr;
}

void Level::onEnemyDying(Enemy* enemy) {
    qDebug() << "onEnemyDying 被调用";

    if (!enemy) {
        qWarning() << "onEnemyDying: enemy 为 null";
        return;
    }

    qDebug() << "确认是Enemy对象";

    // 从enemy指针直接移除，不需要dynamic_cast
    m_currentEnemies.erase(std::remove(m_currentEnemies.begin(), m_currentEnemies.end(), enemy), m_currentEnemies.end());

    qDebug() << "已从全局敌人列表移除，剩余:" << m_currentEnemies.size();

    // 从每个房间的列表移除
    for (Room* r : m_rooms) {
        if (!r)
            continue;
        r->currentEnemies.erase(std::remove(r->currentEnemies.begin(), r->currentEnemies.end(), enemy), r->currentEnemies.end());
    }

    // 如果当前房间敌人已清空，打开所有有效的门
    Room* cur = m_rooms[m_currentRoomIndex];
    qDebug() << "当前房间" << m_currentRoomIndex << "敌人数量:" << (cur ? cur->currentEnemies.size() : -1);

    if (cur && cur->currentEnemies.isEmpty()) {
        // 加载配置以获取门信息
        LevelConfig config;
        if (config.loadFromFile(m_levelNumber)) {
            const RoomConfig& roomCfg = config.getRoom(m_currentRoomIndex);

            // 打开所有存在的门
            if (roomCfg.doorUp >= 0) {
                cur->setDoorOpenUp(true);
                qDebug() << "打开上门，通往房间" << roomCfg.doorUp;
            }
            if (roomCfg.doorDown >= 0) {
                cur->setDoorOpenDown(true);
                qDebug() << "打开下门，通往房间" << roomCfg.doorDown;
            }
            if (roomCfg.doorLeft >= 0) {
                cur->setDoorOpenLeft(true);
                qDebug() << "打开左门，通往房间" << roomCfg.doorLeft;
            }
            if (roomCfg.doorRight >= 0) {
                cur->setDoorOpenRight(true);
                qDebug() << "打开右门，通往房间" << roomCfg.doorRight;
            }
        }

        qDebug() << "房间" << m_currentRoomIndex << "敌人全部清空，门已打开";

        // 显示通知提示玩家可以前进
        emit enemiesCleared(m_currentRoomIndex);
    }
}

void Level::onPlayerDied() {
    qDebug() << "Level::onPlayerDied - 通知所有敌人移除玩家引用";

    // 从当前敌人列表移除玩家引用
    for (Enemy* e : m_currentEnemies) {
        if (e)
            e->setPlayer(nullptr);
    }

    // 也从每个房间的列表中移除玩家引用（防止尚未进入房间的敌人继续追击）
    for (Room* r : m_rooms) {
        if (!r)
            continue;
        for (Enemy* e : r->currentEnemies) {
            if (e)
                e->setPlayer(nullptr);
        }
    }
}
