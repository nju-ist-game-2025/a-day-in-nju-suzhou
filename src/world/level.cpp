#include "level.h"
#include "room.h"
#include "../entities/player.h"
#include "../entities/enemy.h"
#include "../entities/boss.h"
#include "../items/chest.h"
#include "../core/resourcefactory.h"
#include <QRandomGenerator>
#include <QMessageBox>

Level::Level(QGraphicsScene *parent) :
        QGraphicsScene(parent),
        m_levelNumber(1),
        m_currentRoomIndex(0),
        m_player(nullptr)
{
    // 设置关卡场景大小（与背景图尺寸一致）
    setSceneRect(0, 0, 800, 600);
}

Level::~Level()
{
    // 清理所有房间
    qDeleteAll(m_rooms);
    m_rooms.clear();
}

void Level::init(int levelNumber)
{
    // 清理现有数据
    clear();
    qDeleteAll(m_rooms);
    m_rooms.clear();
    m_currentEnemies.clear();
    m_currentChests.clear();

    m_levelNumber = levelNumber;

    // 加载关卡背景
    try {
        QPixmap background = ResourceFactory::loadBackgroundImage(
                "assets/background_game.png", 800, 600);
        setBackgroundBrush(QBrush(background));
    } catch (const QString& error) {
        QMessageBox::critical(nullptr, "资源错误", error);
        return;
    }

    // 生成房间布局
    generateRooms();

    // 初始化第一个房间
    m_currentRoomIndex = 0;
    initCurrentRoom();
}

void Level::generateRooms()
{
    // 根据关卡号生成不同数量的房间（1-3关：5-8个房间，每关递增）
    int roomCount = 5 + (m_levelNumber - 1) * 3;
    if (roomCount > 14) roomCount = 14; // 最大房间数限制

    for (int i = 0; i < roomCount; ++i) {
        m_rooms.append(new Room());
    }
}

void Level::initCurrentRoom()
{
    if (m_currentRoomIndex >= m_rooms.size()) return;

    // 清理当前房间实体
    clearCurrentRoomEntities();

    // 生成当前房间的敌人
    spawnEnemiesInRoom(m_currentRoomIndex);

    // 生成当前房间的宝箱（每2个房间一个，最后一个房间必出）
    if (m_currentRoomIndex % 2 == 0 || m_currentRoomIndex == m_rooms.size() - 1) {
        spawnChestsInRoom(m_currentRoomIndex);
    }

    // 如果是最后一个房间，生成BOSS
    if (m_currentRoomIndex == m_rooms.size() - 1) {
        QPixmap bossPix = ResourceFactory::createEnemyImage(80, "assets/enemy.png");
        Boss* boss = new Boss(bossPix, 2.0);
        boss->setPos(400, 300); // 中心位置
        addItem(boss);
        m_currentEnemies.append(boss);
    }

    emit roomEntered(m_currentRoomIndex);
}

void Level::spawnEnemiesInRoom(int roomIndex)
{
    // 敌人数量随关卡和房间递增
    int enemyCount = 2 + m_levelNumber + (roomIndex / 2);
    QPixmap enemyPix = ResourceFactory::createEnemyImage(40, "assets/enemy.png");

    for (int i = 0; i < enemyCount; ++i) {
        // 随机位置（避开中心区域）
        int x = QRandomGenerator::global()->bounded(100, 700);
        int y = QRandomGenerator::global()->bounded(100, 500);

        // 避免出现在中心玩家初始位置
        if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100) {
            x += 150;
            y += 150;
        }

        Enemy* enemy = new Enemy(enemyPix, 1.0);
        enemy->setPos(x, y);
        addItem(enemy);
        m_currentEnemies.append(enemy);
    }
}

void Level::spawnChestsInRoom(int roomIndex)
{
    QPixmap chestPix = ResourceFactory::createChestImage(50, "assets/chest.png");
    bool isLocked = (roomIndex % 4 == 0) && (roomIndex != 0); // 每4个房间出现一个锁着的宝箱

    // 随机位置
    int x = QRandomGenerator::global()->bounded(150, 650);
    int y = QRandomGenerator::global()->bounded(150, 450);

    Chest* chest;
    if (isLocked) {
        chest = new lockedChest(m_player, chestPix, 1.0);
    } else {
        chest = new Chest(m_player, false, chestPix, 1.0);
    }

    chest->setPos(x, y);
    addItem(chest);
    m_currentChests.append(chest);
}

void Level::clearCurrentRoomEntities()
{
    // 移除并删除当前房间的敌人
    for (Enemy* enemy : m_currentEnemies) {
        removeItem(enemy);
        delete enemy;
    }
    m_currentEnemies.clear();

    // 移除并删除当前房间的宝箱
    for (Chest* chest : m_currentChests) {
        removeItem(chest);
        delete chest;
    }
    m_currentChests.clear();
}

bool Level::isAllRoomsCompleted() const
{
    return m_currentRoomIndex >= m_rooms.size() - 1;
}

bool Level::enterNextRoom()
{
    if (m_currentRoomIndex + 1 >= m_rooms.size()) {
        // 所有房间完成，触发关卡完成
        emit levelCompleted(m_levelNumber);
        return false;
    }

    m_currentRoomIndex++;
    initCurrentRoom();
    return true;
}

