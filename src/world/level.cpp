#include "level.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QRandomGenerator>
#include <algorithm>
#include <QPointer>
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
#include "../core/audiomanager.h"
#include "../entities/boss.h"
#include "../entities/enemy.h"
#include "../entities/player.h"
#include "../items/chest.h"
#include "levelconfig.h"
#include "room.h"

Level::Level(Player *player, QGraphicsScene *scene, QObject *parent)
        : QObject(parent),
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

    qDeleteAll(m_rooms);
    m_rooms.clear();
}

void Level::init(int levelNumber) {
    if (checkChange) {
        checkChange->stop();
        delete checkChange;
        checkChange = nullptr;
    }

    qDeleteAll(m_rooms);
    m_rooms.clear();
    m_currentEnemies.clear();
    m_currentChests.clear();

    m_levelNumber = levelNumber;

    LevelConfig config;
    if (!config.loadFromFile(levelNumber)) {
        qWarning() << "加载关卡配置失败，使用默认配置";
        return;
    }

    qDebug() << "加载关卡:" << config.getLevelName();

    for (int i = 0; i < config.getRoomCount(); ++i) {
        const RoomConfig &roomCfg = config.getRoom(i);
        bool hasUp = (roomCfg.doorUp >= 0);
        bool hasDown = (roomCfg.doorDown >= 0);
        bool hasLeft = (roomCfg.doorLeft >= 0);
        bool hasRight = (roomCfg.doorRight >= 0);

        Room *room = new Room(m_player, hasUp, hasDown, hasLeft, hasRight);
        m_rooms.append(room);
    }

    visited.resize(config.getRoomCount());
    visited.fill(false);
    visited[config.getStartRoomIndex()] = true;
    visited_count = 1;

    m_currentRoomIndex = config.getStartRoomIndex();
    const RoomConfig &startRoomCfg = config.getRoom(m_currentRoomIndex);

    try {
        QString bgPath = ConfigManager::instance().getAssetPath(startRoomCfg.backgroundImage);
        QPixmap bg = ResourceFactory::loadImage(bgPath);
        m_scene->setBackgroundBrush(QBrush(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
    } catch (const QString &e) {
        qWarning() << "加载房间背景失败:" << e;
    }

    initCurrentRoom(m_rooms[m_currentRoomIndex]);

    checkChange = new QTimer(this);
    connect(checkChange, &QTimer::timeout, this, &Level::enterNextRoom);
    checkChange->start(100);
}

void Level::initCurrentRoom(Room *room) {
    if (m_currentRoomIndex >= m_rooms.size())
        return;

    for (Room *rr: m_rooms) {
        if (rr)
            rr->stopChangeTimer();
    }

    clearCurrentRoomEntities();

    LevelConfig config;
    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig &roomCfg = config.getRoom(m_currentRoomIndex);
        try {
            QString bgPath = ConfigManager::instance().getAssetPath(roomCfg.backgroundImage);
            QPixmap bg = ResourceFactory::loadImage(bgPath);
            if (m_scene)
                m_scene->setBackgroundBrush(QBrush(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
            qDebug() << "加载房间" << m_currentRoomIndex << "背景:" << roomCfg.backgroundImage;
        } catch (const QString &e) {
            qWarning() << "加载地图背景失败:" << e;
        }
    }

    spawnEnemiesInRoom(m_currentRoomIndex);
    spawnChestsInRoom(m_currentRoomIndex);

    emit roomEntered(m_currentRoomIndex);
    qDebug() << "进入房间:" << m_currentRoomIndex;

    if (room)
        room->startChangeTimer();
}

void Level::spawnEnemiesInRoom(int roomIndex) {
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        qWarning() << "加载关卡配置失败";
        return;
    }

    const RoomConfig &roomCfg = config.getRoom(roomIndex);

    try {
        QPixmap enemyPix = ResourceFactory::createEnemyImage(40);
        int enemyCount = roomCfg.enemyCount;

        for (int i = 0; i < enemyCount; ++i) {
            int x = QRandomGenerator::global()->bounded(100, 700);
            int y = QRandomGenerator::global()->bounded(100, 500);

            if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100) {
                x += 150;
                y += 150;
            }

            Enemy *enemy = new Enemy(enemyPix, 1.0);
            enemy->setPos(x, y);
            enemy->setPlayer(m_player);
            m_scene->addItem(enemy);

            // 使用 QPointer 存储
            QPointer<Enemy> eptr(enemy);
            m_currentEnemies.append(eptr);
            // Room::currentEnemies 也应为 QVector<QPointer<Enemy>>
            m_rooms[roomIndex]->currentEnemies.append(eptr);

            connect(enemy, &Enemy::dying, this, &Level::onEnemyDying);
            qDebug() << "创建敌人" << i << "位置:" << x << "," << y << "已连接dying信号";
        }
    } catch (const QString &error) {
        qWarning() << "生成敌人失败:" << error;
    }
}

void Level::spawnChestsInRoom(int roomIndex) {
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        qWarning() << "加载关卡配置失败";
        return;
    }

    const RoomConfig &roomCfg = config.getRoom(roomIndex);

    if (!roomCfg.hasChest) {
        return;
    }

    try {
        QPixmap chestPix = ResourceFactory::createChestImage(50);

        int x = QRandomGenerator::global()->bounded(150, 650);
        int y = QRandomGenerator::global()->bounded(150, 450);

        Chest *chest;
        if (roomCfg.isChestLocked) {
            chest = new lockedChest(m_player, chestPix, 1.0);
        } else {
            chest = new Chest(m_player, false, chestPix, 1.0);
        }
        chest->setPos(x, y);
        m_scene->addItem(chest);

        QPointer<Chest> cptr(chest);
        m_currentChests.append(cptr);
        m_rooms[roomIndex]->currentChests.append(cptr);
    } catch (const QString &error) {
        qWarning() << "生成宝箱失败:" << error;
    }
}

void Level::clearCurrentRoomEntities() {
    // 敌人
    for (QPointer<Enemy> enemyPtr : m_currentEnemies) {
        Enemy *enemy = enemyPtr.data();
        // 从所有房间的列表中移除该 QPointer（使用 Qt 接口）
        for (Room *r: m_rooms) {
            if (!r) continue;
            r->currentEnemies.removeAll(enemyPtr);
        }
        if (m_scene && enemy)
            m_scene->removeItem(enemy);
        if (enemy)
            delete enemy;
    }
    m_currentEnemies.clear();

    // 宝箱
    for (QPointer<Chest> chestPtr : m_currentChests) {
        Chest *chest = chestPtr.data();
        for (Room *r: m_rooms) {
            if (!r) continue;
            r->currentChests.removeAll(chestPtr);
        }
        if (m_scene && chest)
            m_scene->removeItem(chest);
        if (chest)
            delete chest;
    }
    m_currentChests.clear();
}

bool Level::enterNextRoom() {
    Room *currentRoom = m_rooms[m_currentRoomIndex];
    int x = currentRoom->getChangeX();
    int y = currentRoom->getChangeY();

    if (!x && !y)
        return false;

    qDebug() << "enterNextRoom: 检测到切换请求 x=" << x << ", y=" << y;

    AudioManager::instance().playSound("enter_room");
    qDebug() << "进入新房间音效已触发";

    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        qWarning() << "加载关卡配置失败";
        return false;
    }

    const RoomConfig &currentRoomCfg = config.getRoom(m_currentRoomIndex);
    int nextRoomIndex = -1;

    if (y == -1) {
        nextRoomIndex = currentRoomCfg.doorUp;
    } else if (y == 1) {
        nextRoomIndex = currentRoomCfg.doorDown;
    } else if (x == -1) {
        nextRoomIndex = currentRoomCfg.doorLeft;
    } else if (x == 1) {
        nextRoomIndex = currentRoomCfg.doorRight;
    }

    if (nextRoomIndex < 0 || nextRoomIndex >= m_rooms.size()) {
        qDebug() << "无效的目标房间:" << nextRoomIndex;
        currentRoom->resetChangeDir();
        return false;
    }

    qDebug() << "切换房间: 从" << m_currentRoomIndex << "到" << nextRoomIndex;

    m_currentRoomIndex = nextRoomIndex;

    if (y == -1) {
        m_player->setPos(m_player->pos().x(), 560);
    } else if (y == 1) {
        m_player->setPos(m_player->pos().x(), 40);
    } else if (x == -1) {
        m_player->setPos(760, m_player->pos().y());
    } else if (x == 1) {
        m_player->setPos(40, m_player->pos().y());
    }

    if (!visited[m_currentRoomIndex]) {
        visited[m_currentRoomIndex] = true;
        visited_count++;
        initCurrentRoom(m_rooms[m_currentRoomIndex]);
    } else {
        loadRoom(m_currentRoomIndex);
    }

    const RoomConfig &nextRoomCfg = config.getRoom(nextRoomIndex);
    Room *nextRoom = m_rooms[nextRoomIndex];

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

    for (Room *rr: m_rooms) {
        if (rr)
            rr->stopChangeTimer();
    }

    m_currentRoomIndex = roomIndex;
    Room *targetRoom = m_rooms[roomIndex];

    LevelConfig config;
    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig &roomCfg = config.getRoom(roomIndex);
        try {
            QString bgPath = ConfigManager::instance().getAssetPath(roomCfg.backgroundImage);
            QPixmap bg = ResourceFactory::loadImage(bgPath);
            m_scene->setBackgroundBrush(QBrush(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
            qDebug() << "加载房间" << roomIndex << "背景:" << roomCfg.backgroundImage;
        } catch (const QString &e) {
            qWarning() << "加载地图背景失败:" << e;
        }
    }

    // 重新添加房间实体到场景（使用 QPointer）
    for (QPointer<Enemy> enemyPtr : targetRoom->currentEnemies) {
        Enemy *enemy = enemyPtr.data();
        if (enemy) {
            m_scene->addItem(enemy);
            m_currentEnemies.append(enemyPtr);
        }
    }

    for (QPointer<Chest> chestPtr : targetRoom->currentChests) {
        Chest *chest = chestPtr.data();
        if (chest) {
            m_scene->addItem(chest);
            m_currentChests.append(chestPtr);
        }
    }

    if (m_player) {
        m_player->setZValue(100);
    }

    emit roomEntered(roomIndex);

    if (targetRoom)
        targetRoom->startChangeTimer();
}

Room *Level::currentRoom() const {
    if (m_currentRoomIndex < m_rooms.size()) {
        return m_rooms[m_currentRoomIndex];
    }
    return nullptr;
}

void Level::onEnemyDying(Enemy *enemy) {
    qDebug() << "onEnemyDying 被调用";

    if (!enemy) {
        qWarning() << "onEnemyDying: enemy 为 null";
        return;
    }

    // 使用 QPointer 包装目标并用 removeAll 安全移除
    QPointer<Enemy> target(enemy);
    m_currentEnemies.removeAll(target);

    qDebug() << "已从全局敌人列表移除，剩余:" << m_currentEnemies.size();
    bonusEffects();

    for (Room *r: m_rooms) {
        if (!r) continue;
        r->currentEnemies.removeAll(target);
    }

    Room *cur = m_rooms[m_currentRoomIndex];
    qDebug() << "当前房间" << m_currentRoomIndex << "敌人数量:" << (cur ? cur->currentEnemies.size() : -1);

    if (cur && cur->currentEnemies.isEmpty()) {
        LevelConfig config;
        if (config.loadFromFile(m_levelNumber)) {
            const RoomConfig &roomCfg = config.getRoom(m_currentRoomIndex);

            AudioManager::instance().playSound("door_open");
            qDebug() << "敌人清空，播放门打开音效";

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
        emit enemiesCleared(m_currentRoomIndex);
    }
}

void Level::onPlayerDied() {
    qDebug() << "Level::onPlayerDied - 通知所有敌人移除玩家引用";

    // 先收集当前全局敌人的原始指针，并断开其 dying 信号，避免在调用 setPlayer 时触发 onEnemyDying 导致容器被修改
    QVector<Enemy *> rawEnemies;
    for (QPointer<Enemy> ePtr : m_currentEnemies) {
        Enemy *e = ePtr.data();
        if (e) {
            QObject::disconnect(e, &Enemy::dying, this, &Level::onEnemyDying);
            rawEnemies.append(e);
        }
    }
    for (Enemy *e : rawEnemies) {
        if (e)
            e->setPlayer(nullptr);
    }

    // 对每个房间同样处理：先收集原始指针并断开信号，再调用 setPlayer(nullptr)
    for (Room *r : m_rooms) {
        if (!r) continue;
        QVector<Enemy *> roomRaw;
        for (QPointer<Enemy> ePtr : r->currentEnemies) {
            Enemy *e = ePtr.data();
            if (e) {
                QObject::disconnect(e, &Enemy::dying, this, &Level::onEnemyDying);
                roomRaw.append(e);
            }
        }
        for (Enemy *e : roomRaw) {
            if (e)
                e->setPlayer(nullptr);
        }
    }
}

void Level::bonusEffects() {
    if(!m_player) return;
    QVector<StatusEffect*> effectPool;

    effectPool.append(new SpeedEffect(5, 1.5));
    effectPool.append(new DamageEffect(5, 1.5));
    effectPool.append(new shootSpeedEffect(5, 1.5));
    //effectPool.append(new soulHeartEffect(m_player, 1));
    effectPool.append(new decDamage(5, 0.5));
    effectPool.append(new InvincibleEffect(5));

    int selectedIndex = QRandomGenerator::global()->bounded(effectPool.size() + 1);
    if (selectedIndex < effectPool.size() && effectPool[selectedIndex]) {
        effectPool[selectedIndex]->applyTo(m_player);
        // 移除选中的效果，避免重复删除
        effectPool.remove(selectedIndex);
    } else if(selectedIndex == effectPool.size()) {
        soulHeartEffect *soul = new soulHeartEffect(m_player, 1);
        soul->applyTo(m_player);
    }

    // 清理所有未使用的效果
    for (StatusEffect* effect : effectPool) {
        effect->deleteLater();
    }
}
