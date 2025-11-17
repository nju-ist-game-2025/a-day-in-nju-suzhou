#include "level.h"
#include "room.h"
#include "../entities/player.h"
#include "../entities/enemy.h"
#include "../entities/boss.h"
#include "../items/chest.h"
#include "../core/resourcefactory.h"
#include <QRandomGenerator>
#include <QMessageBox>
#include <QDebug>
#include <QGraphicsScene>

Level::Level(Player *player, QGraphicsScene *scene, QObject *parent) :
    QObject(parent),
    m_levelNumber(1),
    m_currentRoomIndex(0),
    m_player(player),
    m_scene(scene),
    checkChange(nullptr),
    visited_count(0)
{
}

Level::~Level()
{
    if (checkChange) {
        checkChange->stop();
        delete checkChange;
    }

    // 清理所有房间
    qDeleteAll(m_rooms);
    m_rooms.clear();
}

void Level::init(int levelNumber)
{
    // 停止现有定时器
    if (checkChange) {
        checkChange->stop();
        delete checkChange;
        checkChange = nullptr;
    }

    // 清理现有数据
    m_scene->clear();
    qDeleteAll(m_rooms);
    m_rooms.clear();
    m_currentEnemies.clear();
    m_currentChests.clear();

    m_levelNumber = levelNumber;

    // 加载关卡背景
    try {
        QPixmap background = ResourceFactory::loadBackgroundImage(
            "assets/background_game.png", 800, 600);
        m_scene->setBackgroundBrush(QBrush(background));
    } catch (const QString& error) {
        QMessageBox::critical(nullptr, "资源错误", error);
        // 使用默认背景
        m_scene->setBackgroundBrush(QBrush(Qt::darkGray));
    }

    // 生成房间布局
    generateRooms();

    // 初始化第一个房间
    m_currentRoomIndex = 0;
    initCurrentRoom(m_rooms[0]);

    // 启动房间切换检测
    checkChange = new QTimer(this);
    connect(checkChange, &QTimer::timeout, this, &Level::enterNextRoom);
    checkChange->start(100); // 改为100ms，12ms太快
}

void Level::generateRooms()
{
    // 根据关卡号生成不同数量的房间
    int roomCount = 5 + (m_levelNumber - 1) * 3;
    if (roomCount > 14) roomCount = 14;

    visited.resize(roomCount);
    for(int i = 0; i < roomCount; i++) visited[i] = false;
    visited[0] = true;
    visited_count = 1;

    // 创建起始房间
    m_rooms.append(new Room(m_player, true, true, true, true));

    // 创建其他房间
    for (int i = 1; i < roomCount; ++i) {
        if(i % 4 == 1) m_rooms.append(new Room(m_player, false, false, false, true));
        else if(i % 4 == 2) m_rooms.append(new Room(m_player, false, true, false, false));
        else if(i % 4 == 3) m_rooms.append(new Room(m_player, false, false, true, false));
        else m_rooms.append(new Room(m_player, true, false, false, false));
    }
}

void Level::initCurrentRoom(Room* room)
{
    if (m_currentRoomIndex >= m_rooms.size()) return;

    // 清理当前房间实体
    clearCurrentRoomEntities();

    // 生成当前房间的敌人
    spawnEnemiesInRoom(m_currentRoomIndex);

    // 生成当前房间的宝箱
    if (m_currentRoomIndex % 2 == 0 || m_currentRoomIndex == m_rooms.size() - 1) {
        spawnChestsInRoom(m_currentRoomIndex);
    }

    // 如果是最后一个房间，生成BOSS
    if (visited_count == m_rooms.size() - 1) {
        try {
            QPixmap bossPix = ResourceFactory::createEnemyImage(80, "assets/enemy.png");
            Boss* boss = new Boss(bossPix, 2.0);
            boss->setPos(400, 300);
            m_scene->addItem(boss);
            m_currentEnemies.append(boss);
            room->currentEnemies.append(boss);
        } catch (const QString& error) {
            qWarning() << "无法创建BOSS:" << error;
        }
    }

    emit roomEntered(m_currentRoomIndex);
    qDebug() << "进入房间:" << m_currentRoomIndex;
}


void Level::spawnEnemiesInRoom(int roomIndex)
{
    try {
        QPixmap enemyPix = ResourceFactory::createEnemyImage(40, "assets/enemy.png");
        int enemyCount = 2 + m_levelNumber + (roomIndex / 2);

        for (int i = 0; i < enemyCount; ++i) {
            int x = QRandomGenerator::global()->bounded(100, 700);
            int y = QRandomGenerator::global()->bounded(100, 500);

            if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100) {
                x += 150;
                y += 150;
            }

            Enemy* enemy = new Enemy(enemyPix, 1.0);
            enemy->setPos(x, y);
            m_scene->addItem(enemy);
            m_currentEnemies.append(enemy);
            m_rooms[roomIndex]->currentEnemies.append(enemy);
        }
    } catch (const QString& error) {
        qWarning() << "生成敌人失败:" << error;
    }
}

void Level::spawnChestsInRoom(int roomIndex)
{
    try {
        QPixmap chestPix = ResourceFactory::createChestImage(50, "assets/chest.png");
        bool isLocked = (roomIndex % 4 == 0) && (roomIndex != 0);

        int x = QRandomGenerator::global()->bounded(150, 650);
        int y = QRandomGenerator::global()->bounded(150, 450);

        Chest* chest;
        if (isLocked) {
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

void Level::clearCurrentRoomEntities()
{
    for (Enemy* enemy : m_currentEnemies) {
        m_scene->removeItem(enemy);
        delete enemy;
    }
    m_currentEnemies.clear();

    for (Chest* chest : m_currentChests) {
        m_scene->removeItem(chest);
        delete chest;
    }
    m_currentChests.clear();
}

bool Level::enterNextRoom()
{
    if (m_currentRoomIndex + 1 >= m_rooms.size()) {
        //emit levelCompleted(m_levelNumber);
        //return false;
    }

    qDebug() << "尝试进入下一个房间，当前房间:" << m_currentRoomIndex;

    //地图模式一
    Room* r = m_rooms[m_currentRoomIndex];
    int x = r->getChangeX(), y = r->getChangeY();
    if(!x && !y) return false;
    int sz = m_rooms.size(), mode = m_currentRoomIndex % 4;
    if(x == -1) {
        if(!m_currentRoomIndex) m_currentRoomIndex = 1;
        else {
            if(mode == 1) m_currentRoomIndex += (m_currentRoomIndex + 4 < sz ? 4 : 0);
            else if(mode == 3) m_currentRoomIndex -= 4;
        }
        m_player->setPos(760, m_player->pos().y());
    }
    if(x == 1) {
        if(!m_currentRoomIndex) m_currentRoomIndex = 3;
        else {
            if(mode == 3) m_currentRoomIndex += (m_currentRoomIndex + 4 < sz ? 4 : 0);
            else if(mode == 1) m_currentRoomIndex -= 4;
        }
        m_player->setPos(40, m_player->pos().y());
    }
    if(y == -1) {
        if(!m_currentRoomIndex) m_currentRoomIndex = 2;
        else {
            if(mode == 2) m_currentRoomIndex += (m_currentRoomIndex + 4 < sz ? 4 : 0);
            else if(mode == 4) m_currentRoomIndex -= 4;
        }
        m_player->setPos(m_player->pos().x(), 560);
    }
    if(y == 1) {
        if(!m_currentRoomIndex) m_currentRoomIndex = 4;
        else {
            if(mode == 4) m_currentRoomIndex += (m_currentRoomIndex + 4 < sz ? 4 : 0);
            else if(mode == 2) m_currentRoomIndex -= 4;
        }
        m_player->setPos(m_player->pos().x(), 40);
    }
    if(!visited[m_currentRoomIndex]) {
        visited[m_currentRoomIndex] = true;
        visited_count++;
        Room *nxt = m_rooms[m_currentRoomIndex];
        initCurrentRoom(nxt);
    } else {
        loadRoom(m_currentRoomIndex);
    }
    r->resetChangeDir();

    qDebug() << "切换后房间:" << m_currentRoomIndex;
    qDebug() << "change_x:" << x << "change_y:" << y;
    return true;
}


void Level::loadRoom(int roomIndex)
{
    if (roomIndex < 0 || roomIndex >= m_rooms.size()) {
        qWarning() << "无效的房间索引:" << roomIndex;
        return;
    }
    if (!visited[roomIndex]) {
        qWarning() << "房间" << roomIndex << "未被访问过，无法加载";
        return;
    }

    m_currentRoomIndex = roomIndex;
    Room* targetRoom = m_rooms[roomIndex];

    // 重新加载背景
    /*try {
        QPixmap background = ResourceFactory::loadBackgroundImage(
            "assets/background_game.png", 800, 600);
        setBackgroundBrush(QBrush(background));
    } catch (const QString& error) {
        qWarning() << "加载背景失败:" << error;
        setBackgroundBrush(QBrush(Qt::darkGray));
    }*/

    for (Enemy* enemy : targetRoom->currentEnemies) {
            m_scene->addItem(enemy);
            m_currentEnemies.append(enemy);
            qDebug() << "恢复敌人，位置:" << enemy->pos();
    }

    for (Chest* chest : targetRoom->currentChests) {
            m_scene->addItem(chest);
            m_currentChests.append(chest);
            qDebug() << "恢复宝箱，位置:" << chest->pos();
    }


    if (m_player) {
        // 根据进入方向设置玩家位置
        Room* previousRoom = (m_currentRoomIndex > 0) ? m_rooms[m_currentRoomIndex - 1] : nullptr;
        if (previousRoom) {
            int prevChangeX = previousRoom->getChangeX();
            int prevChangeY = previousRoom->getChangeY();

            if (prevChangeX == -1) m_player->setPos(760, m_player->pos().y());
            else if (prevChangeX == 1) m_player->setPos(40, m_player->pos().y());
            else if (prevChangeY == -1) m_player->setPos(m_player->pos().x(), 560);
            else if (prevChangeY == 1) m_player->setPos(m_player->pos().x(), 40);
        }
        m_player->setZValue(100);
    }

    emit roomEntered(roomIndex);
}

Room* Level::currentRoom() const
{
    if (m_currentRoomIndex < m_rooms.size()) {
        return m_rooms[m_currentRoomIndex];
    }
    return nullptr;
}
