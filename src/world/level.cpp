#include "level.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QPointer>
#include <QRandomGenerator>
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
#include "../entities/boss.h"
#include "../entities/enemy.h"
#include "../entities/clockenemy.h"
#include "../entities/clockboom.h"
#include "../entities/sockenemy.h"
#include "../entities/player.h"
#include "../entities/projectile.h"
#include "../items/chest.h"
#include "../ui/gameview.h"
#include "../ui/hud.h"
#include "levelconfig.h"
#include "room.h"

Level::Level(Player *player, QGraphicsScene *scene, QObject *parent)
    : QObject(parent),
      m_levelNumber(1),
      m_currentRoomIndex(0),
      m_player(player),
      m_scene(scene),
      checkChange(nullptr),
      visited_count(0),
      m_levelTextTimer(nullptr),
      m_dialogBox(nullptr),
      m_dialogText(nullptr),
      m_continueHint(nullptr),
      m_currentDialogIndex(0),
      m_isStoryFinished(false)
{
}

Level::~Level()
{
    // 移除事件过滤器（关键：防止事件发送到已删除的对象）
    if (m_scene)
    {
        m_scene->removeEventFilter(this);
    }

    // 断开所有信号连接，防止析构后回调
    disconnect(this, nullptr, nullptr, nullptr);

    // 清理关卡文字显示定时器
    if (m_levelTextTimer)
    {
        m_levelTextTimer->stop();
        disconnect(m_levelTextTimer, nullptr, nullptr, nullptr);
        m_levelTextTimer->deleteLater();
        m_levelTextTimer = nullptr;
    }

    // 清理对话框UI（如果还存在）
    if (m_dialogBox)
    {
        if (m_scene)
            m_scene->removeItem(m_dialogBox);
        delete m_dialogBox;
        m_dialogBox = nullptr;
    }
    if (m_dialogText)
    {
        if (m_scene)
            m_scene->removeItem(m_dialogText);
        delete m_dialogText;
        m_dialogText = nullptr;
    }
    if (m_continueHint)
    {
        if (m_scene)
            m_scene->removeItem(m_continueHint);
        delete m_continueHint;
        m_continueHint = nullptr;
    }

    if (checkChange)
    {
        checkChange->stop();
        disconnect(checkChange, nullptr, nullptr, nullptr);
        delete checkChange;
        checkChange = nullptr;
    }

    // 清理所有敌人的信号连接
    for (QPointer<Enemy> ePtr : m_currentEnemies)
    {
        if (Enemy *e = ePtr.data())
        {
            disconnect(e, nullptr, this, nullptr);
        }
    }

    // 清理所有门对象
    for (auto it = m_roomDoors.begin(); it != m_roomDoors.end(); ++it)
    {
        qDeleteAll(it.value());
    }
    m_roomDoors.clear();

    qDeleteAll(m_rooms);
    m_rooms.clear();
}

void Level::init(int levelNumber)
{
    if (checkChange)
    {
        checkChange->stop();
        delete checkChange;
        checkChange = nullptr;
    }

    // 清理所有门对象
    for (auto it = m_roomDoors.begin(); it != m_roomDoors.end(); ++it)
    {
        qDeleteAll(it.value());
    }
    m_roomDoors.clear();
    m_currentDoors.clear();

    qDeleteAll(m_rooms);
    m_rooms.clear();
    m_currentEnemies.clear();
    m_currentChests.clear();

    m_levelNumber = levelNumber;

    LevelConfig config;
    if (!config.loadFromFile(levelNumber))
    {
        qWarning() << "加载关卡配置失败，使用默认配置";
        return;
    }

    qDebug() << "加载关卡:" << config.getLevelName();
    qDebug() << "关卡描述条数:" << config.getDescription().size();

    // 如果有剧情描述，显示剧情；否则直接初始化关卡
    if (!config.getDescription().isEmpty())
    {
        // 断开之前的所有 storyFinished 连接，避免重复触发
        disconnect(this, &Level::storyFinished, nullptr, nullptr);
        connect(this, &Level::storyFinished, this, [this, config]()
                { initializeLevelAfterStory(config); });
        showLevelStartText(config);
        const QStringList list = config.getDescription();
        // showCredits(list);
        showStoryDialog(config.getDescription());
    }
    else
    {
        showLevelStartText(config);
        const QStringList list = config.getDescription();
        // showCredits(list);
        initializeLevelAfterStory(config);
    }
}

void Level::initializeLevelAfterStory(const LevelConfig &config)
{
    qDebug() << "加载关卡:" << config.getLevelName();

    for (int i = 0; i < config.getRoomCount(); ++i)
    {
        const RoomConfig &roomCfg = config.getRoom(i);
        bool hasUp = (roomCfg.doorUp >= 0);
        bool hasDown = (roomCfg.doorDown >= 0);
        bool hasLeft = (roomCfg.doorLeft >= 0);
        bool hasRight = (roomCfg.doorRight >= 0);

        Room *room = new Room(m_player, hasUp, hasDown, hasLeft, hasRight);

        // 根据配置自动判断战斗房间（有敌人或有Boss的房间）
        if (roomCfg.enemyCount > 0 || roomCfg.hasBoss)
        {
            room->setBattleRoom(true);
            qDebug() << "房间" << i << "标记为战斗房间（敌人数:" << roomCfg.enemyCount << "，Boss:" << roomCfg.hasBoss << "）";
        }

        m_rooms.append(room);
    }

    visited.resize(config.getRoomCount());
    visited.fill(false);
    visited[config.getStartRoomIndex()] = true;
    visited_count = 1;

    m_currentRoomIndex = config.getStartRoomIndex();
    const RoomConfig &startRoomCfg = config.getRoom(m_currentRoomIndex);

    try
    {
        QString bgPath = ConfigManager::instance().getAssetPath(startRoomCfg.backgroundImage);
        QPixmap bg = ResourceFactory::loadImage(bgPath);
        m_scene->setBackgroundBrush(QBrush(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
    }
    catch (const QString &e)
    {
        qWarning() << "加载房间背景失败:" << e;
    }

    initCurrentRoom(m_rooms[m_currentRoomIndex]);

    checkChange = new QTimer(this);
    connect(checkChange, &QTimer::timeout, this, &Level::enterNextRoom);
    checkChange->start(100);

    buildMinimapData();
}

// 处理鼠标点击继续对话
void Level::onDialogClicked()
{
    if (!m_isStoryFinished)
    {
        nextDialog();
    }
}

void Level::showStoryDialog(const QStringList &dialogs)
{
    m_currentDialogs = dialogs;
    m_currentDialogIndex = 0;
    QString imagePath;
    if (m_levelNumber == 1)
        imagePath = "assets/galgame/l1.png";
    else if (m_levelNumber == 2)
        imagePath = "assets/galgame/l2.png";
    else
        imagePath = "assets/galgame/l3.png";

    // 检查文件是否存在
    QFile file(imagePath);
    if (!file.exists())
    {
        qWarning() << "图片文件不存在:" << imagePath;
        return;
    }

    // 加载图片
    QPixmap bgPixmap(imagePath);
    if (bgPixmap.isNull())
    {
        qWarning() << "加载图片失败:" << imagePath;
        return;
    }

    // 创建图片项
    m_dialogBox = new QGraphicsPixmapItem(bgPixmap.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    m_dialogBox->setPos(0, 0);
    m_dialogBox->setZValue(10000);
    m_scene->addItem(m_dialogBox);

    // 创建对话框文本
    m_dialogText = new QGraphicsTextItem();
    if (m_levelNumber == 2)
        m_dialogText->setDefaultTextColor(Qt::black);
    else
        m_dialogText->setDefaultTextColor(Qt::white);
    m_dialogText->setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    m_dialogText->setTextWidth(700);
    m_dialogText->setPos(50, 420);
    m_dialogText->setZValue(10001);
    m_scene->addItem(m_dialogText);

    // 创建继续提示
    m_continueHint = new QGraphicsTextItem("点击或按Enter键继续...");
    m_continueHint->setDefaultTextColor(QColor(255, 255, 255, 180));
    m_continueHint->setFont(QFont("Microsoft YaHei", 12, QFont::Light));
    m_continueHint->setPos(600, 550);
    m_continueHint->setZValue(10001);
    m_scene->addItem(m_continueHint);

    // 让对话框可点击
    m_dialogBox->setFlag(QGraphicsItem::ItemIsFocusable);
    m_dialogBox->setAcceptHoverEvents(true);

    // 先移除旧的事件过滤器（如果存在），再安装新的
    m_scene->removeEventFilter(this);
    m_scene->installEventFilter(this);

    // 显示第一句对话
    nextDialog();
}

void Level::nextDialog()
{
    if (m_currentDialogIndex >= m_currentDialogs.size())
    {
        finishStory();
        return;
    }

    QString currentText = m_currentDialogs[m_currentDialogIndex];
    m_dialogText->setPlainText(currentText);
    m_currentDialogIndex++;

    qDebug() << "显示对话:" << m_currentDialogIndex << "/" << m_currentDialogs.size();
}

void Level::finishStory()
{
    qDebug() << "剧情播放完毕";

    // 移除事件过滤器
    if (m_scene)
    {
        m_scene->removeEventFilter(this);
    }

    // 清理剧情UI
    if (m_dialogBox)
    {
        m_scene->removeItem(m_dialogBox);
        delete m_dialogBox;
        m_dialogBox = nullptr;
    }
    if (m_dialogText)
    {
        m_scene->removeItem(m_dialogText);
        delete m_dialogText;
        m_dialogText = nullptr;
    }
    if (m_continueHint)
    {
        m_scene->removeItem(m_continueHint);
        delete m_continueHint;
        m_continueHint = nullptr;
    }

    m_isStoryFinished = true;
    emit storyFinished();
}

// 文本滚动——暂时搁置
void Level::showCredits(const QStringList &desc)
{
    const QStringList credits = desc;
    // 创建文本项
    QGraphicsTextItem *creditsText = new QGraphicsTextItem();
    creditsText->setDefaultTextColor(QColor(102, 0, 153));
    creditsText->setFont(QFont("SimSun", 20, QFont::Bold));

    // 设置文本内容
    QString fullText;
    for (const QString &line : credits)
    {
        fullText += line + "\n" + "\n" + "\n";
    }
    creditsText->setPlainText(fullText);

    // 设置初始位置（屏幕底部）
    QRectF sceneRect = m_scene->sceneRect();
    creditsText->setPos(sceneRect.width() / 2 - creditsText->boundingRect().width() / 2,
                        sceneRect.height() + 100);
    creditsText->setZValue(11000);
    m_scene->addItem(creditsText);

    // 创建动画
    QPropertyAnimation *animation = new QPropertyAnimation(creditsText, "pos");
    animation->setDuration(15000); // 15秒
    animation->setStartValue(creditsText->pos());
    animation->setEndValue(QPointF(sceneRect.width() / 2 - creditsText->boundingRect().width() / 2,
                                   -creditsText->boundingRect().height() - 100));
    animation->setEasingCurve(QEasingCurve::Linear);

    // 动画结束时清理，使用QPointer保护this指针
    QPointer<Level> levelPtr(this);
    QPointer<QGraphicsScene> scenePtr(m_scene);
    connect(animation, &QPropertyAnimation::finished, [levelPtr, scenePtr, creditsText, animation]()
            {
                if (scenePtr && creditsText->scene() == scenePtr) {
                    scenePtr->removeItem(creditsText);
                }
                delete creditsText;
                animation->deleteLater(); });

    animation->start();
}

void Level::showLevelStartText(LevelConfig &config)
{
    QGraphicsTextItem *levelTextItem = new QGraphicsTextItem(QString(config.getLevelName()));
    levelTextItem->setDefaultTextColor(Qt::red);
    levelTextItem->setFont(QFont("Arial", 28, QFont::Bold));

    int sceneWidth = 800;
    int sceneHeight = 600;
    levelTextItem->setPos(sceneWidth / 2 - 150, sceneHeight / 2 - 30);
    levelTextItem->setZValue(10000);
    m_scene->addItem(levelTextItem);
    m_scene->update();
    qDebug() << "文字项已添加到场景，Z值:" << levelTextItem->zValue();

    // 停止之前的定时器（如果存在）
    if (m_levelTextTimer)
    {
        m_levelTextTimer->stop();
        m_levelTextTimer->deleteLater();
    }

    // 2秒后自动移除，使用QPointer保护this指针
    m_levelTextTimer = new QTimer(this);
    m_levelTextTimer->setSingleShot(true);
    QPointer<Level> levelPtr(this);
    QPointer<QGraphicsScene> scenePtr(m_scene);
    connect(m_levelTextTimer, &QTimer::timeout, [levelTextItem, levelPtr, scenePtr]()
            {
        if (scenePtr && levelTextItem->scene() == scenePtr) {
            scenePtr->removeItem(levelTextItem);
        }
        delete levelTextItem;
        qDebug() << "测试文字已移除"; });
    m_levelTextTimer->start(2000);
}

void Level::initCurrentRoom(Room *room)
{
    if (m_currentRoomIndex >= m_rooms.size())
        return;

    for (Room *rr : m_rooms)
    {
        if (rr)
            rr->stopChangeTimer();
    }

    // Clear only the scene, not the room data
    clearSceneEntities();

    LevelConfig config;
    if (config.loadFromFile(m_levelNumber))
    {
        const RoomConfig &roomCfg = config.getRoom(m_currentRoomIndex);
        try
        {
            QString bgPath = ConfigManager::instance().getAssetPath(roomCfg.backgroundImage);
            QPixmap bg = ResourceFactory::loadImage(bgPath);
            if (m_scene)
                m_scene->setBackgroundBrush(QBrush(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
            qDebug() << "加载房间" << m_currentRoomIndex << "背景:" << roomCfg.backgroundImage;
        }
        catch (const QString &e)
        {
            qWarning() << "加载地图背景失败:" << e;
        }
        spawnDoors(roomCfg);
    }

    // Spawn enemies and chests - they will be added to both room lists and global lists
    // inside the spawn functions, so we don't need to add them again
    qDebug() << "初始化房间" << m_currentRoomIndex << "，开始生成实体";
    spawnEnemiesInRoom(m_currentRoomIndex);
    spawnChestsInRoom(m_currentRoomIndex);
    qDebug() << "初始化房间" << m_currentRoomIndex << "完成，房间敌人数:" << room->currentEnemies.size() << "，全局敌人数:" << m_currentEnemies.size();

    // 如果是战斗房间且尚未开始战斗，且有敌人，则标记战斗开始
    if (room && room->isBattleRoom() && !room->isBattleStarted() && !room->currentEnemies.isEmpty())
    {
        qDebug() << "战斗房间" << m_currentRoomIndex << "触发战斗";
        room->startBattle();
    }

    if (m_player)
    {
        m_player->setZValue(100);
    }

    emit roomEntered(m_currentRoomIndex);

    if (room)
        room->startChangeTimer();
}

void Level::spawnEnemiesInRoom(int roomIndex)
{
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber))
    {
        qWarning() << "加载关卡配置失败";
        return;
    }

    const RoomConfig &roomCfg = config.getRoom(roomIndex);

    try
    {
        // 根据关卡号确定敌人类型
        QString enemyType;
        if (m_levelNumber == 1)
        {
            enemyType = "clock_normal";
        }
        else if (m_levelNumber == 2)
        {
            enemyType = "sock_normal";
        }

        QPixmap enemyPix = ResourceFactory::createEnemyImage(40, m_levelNumber, enemyType);
        QPixmap bossPix = ResourceFactory::createBossImage(80);
        int enemyCount = roomCfg.enemyCount;
        bool hasBoss = roomCfg.hasBoss;

        Room *cur = m_rooms[roomIndex];
        // 如果没有敌人且不是战斗房间，或者战斗已经结束，则打开门
        if (enemyCount == 0 && (!cur->isBattleRoom() || cur->isBattleStarted()))
        {
            openDoors(cur);
        }

        for (int i = 0; i < enemyCount; ++i)
        {
            int x = QRandomGenerator::global()->bounded(100, 700);
            int y = QRandomGenerator::global()->bounded(100, 500);

            if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100)
            {
                x += 150;
                y += 150;
            }

            // 根据关卡号和敌人类型创建具体的敌人实例
            Enemy *enemy = createEnemyByType(m_levelNumber, enemyType, enemyPix, 1.0);
            enemy->setPos(x, y);
            enemy->setPlayer(m_player);
            m_scene->addItem(enemy);

            // 使用 QPointer 存储
            QPointer<Enemy> eptr(enemy);
            m_currentEnemies.append(eptr);
            m_rooms[roomIndex]->currentEnemies.append(eptr);

            connect(enemy, &Enemy::dying, this, &Level::onEnemyDying);
            qDebug() << "创建敌人" << enemyType << "位置:" << x << "," << y;
        }

        // 第一关的战斗房间生成clock_boom（房间k生成k-1个）
        if (m_levelNumber == 1 && cur->isBattleRoom() && roomIndex > 0)
        {
            int boomCount = roomIndex - 1; // room1不生成，room2生成1个，room3生成2个...

            QPixmap boomNormalPic = ResourceFactory::createEnemyImage(40, 1, "clock_boom");
            // 红色闪烁效果会在ClockBoom构造函数中自动生成（类似Entity的flash效果）

            for (int i = 0; i < boomCount; ++i)
            {
                int x = QRandomGenerator::global()->bounded(100, 700);
                int y = QRandomGenerator::global()->bounded(100, 500);

                if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100)
                {
                    x += 150;
                    y += 150;
                }

                ClockBoom *boom = new ClockBoom(boomNormalPic, boomNormalPic, 1.0);
                boom->setPos(x, y);
                boom->setPlayer(m_player);
                m_scene->addItem(boom);

                QPointer<Enemy> eptr(boom);
                m_currentEnemies.append(eptr);
                m_rooms[roomIndex]->currentEnemies.append(eptr);

                connect(boom, &Enemy::dying, this, &Level::onEnemyDying);
                qDebug() << "创建ClockBoom，房间" << roomIndex << "，编号" << i << "，位置:" << x << "," << y;
            }
        }

        if (hasBoss)
        {
            int x = QRandomGenerator::global()->bounded(100, 700);
            int y = QRandomGenerator::global()->bounded(100, 500);

            if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100)
            {
                x += 150;
                y += 150;
            }

            Boss *boss = new Boss(bossPix, 1.0);
            boss->setPos(x, y);
            boss->setPlayer(m_player);
            m_scene->addItem(boss);

            // 使用 QPointer 存储
            QPointer<Boss> eptr(boss);
            m_currentEnemies.append(eptr);
            // Room::currentEnemies 也应为 QVector<QPointer<Enemy>>
            m_rooms[roomIndex]->currentEnemies.append(eptr);

            connect(boss, &Boss::dying, this, &Level::onEnemyDying);
            qDebug() << "创建boss" << "位置:" << x << "," << y << "已连接dying信号";
        }
    }
    catch (const QString &error)
    {
        qWarning() << "生成敌人失败:" << error;
    }
}

void Level::spawnChestsInRoom(int roomIndex)
{
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber))
    {
        qWarning() << "加载关卡配置失败";
        return;
    }

    const RoomConfig &roomCfg = config.getRoom(roomIndex);

    if (!roomCfg.hasChest)
    {
        return;
    }

    try
    {
        QPixmap chestPix = ResourceFactory::createChestImage(50);

        int x = QRandomGenerator::global()->bounded(150, 650);
        int y = QRandomGenerator::global()->bounded(150, 450);

        Chest *chest;
        if (roomCfg.isChestLocked)
        {
            chest = new lockedChest(m_player, chestPix, 1.0);
        }
        else
        {
            chest = new Chest(m_player, false, chestPix, 1.0);
        }
        chest->setPos(x, y);
        m_scene->addItem(chest);

        QPointer<Chest> cptr(chest);
        m_currentChests.append(cptr);
        m_rooms[roomIndex]->currentChests.append(cptr);
    }
    catch (const QString &error)
    {
        qWarning() << "生成宝箱失败:" << error;
    }
}

Enemy *Level::createEnemyByType(int levelNumber, const QString &enemyType, const QPixmap &pic, double scale)
{
    // 根据关卡号和敌人类型创建具体的敌人实例
    if (levelNumber == 1)
    {
        if (enemyType == "clock_normal")
        {
            return new ClockEnemy(pic, scale);
        }
    }
    else if (levelNumber == 2)
    {
        if (enemyType == "sock_normal")
        {
            return new SockNormal(pic, scale);
        }
        else if (enemyType == "sock_angrily")
        {
            return new SockAngrily(pic, scale);
        }
    }

    // 默认返回基础敌人
    qDebug() << "未知敌人类型，使用默认Enemy:" << enemyType;
    return new Enemy(pic, scale);
}

void Level::spawnDoors(const RoomConfig &roomCfg)
{
    try
    {
        // 检查是否已有该房间的门对象（复用逻辑）
        if (m_roomDoors.contains(m_currentRoomIndex))
        {
            // 复用已存在的门对象
            m_currentDoors = m_roomDoors[m_currentRoomIndex];

            // 将门重新添加到场景并更新状态
            Room *currentRoom = m_rooms[m_currentRoomIndex];
            for (Door *door : m_currentDoors)
            {
                if (door)
                {
                    m_scene->addItem(door);

                    // 根据当前房间状态更新门的显示
                    if (door->state() != Door::Open)
                    {
                        // 只更新未打开的门
                        if (currentRoom)
                        {
                            bool shouldBeOpen = false;
                            if (door->direction() == Door::Up && currentRoom->isDoorOpenUp())
                                shouldBeOpen = true;
                            else if (door->direction() == Door::Down && currentRoom->isDoorOpenDown())
                                shouldBeOpen = true;
                            else if (door->direction() == Door::Left && currentRoom->isDoorOpenLeft())
                                shouldBeOpen = true;
                            else if (door->direction() == Door::Right && currentRoom->isDoorOpenRight())
                                shouldBeOpen = true;

                            if (shouldBeOpen)
                            {
                                door->setOpenState();
                            }
                        }
                    }
                }
            }
            return;
        }

        // 首次创建该房间的门对象
        m_currentDoors.clear();

        // 场景尺寸：800x600
        // 上下门：120x80，左右门：80x120

        if (roomCfg.doorUp >= 0)
        {
            Door *door = new Door(Door::Up);
            // 上门：水平居中在顶部 (800-120)/2 = 340
            door->setPos(340, 0);
            m_scene->addItem(door);
            m_currentDoors.append(door);

            // 检查这个门是否应该已经是打开状态
            Room *currentRoom = m_rooms[m_currentRoomIndex];
            if (currentRoom && currentRoom->isDoorOpenUp())
            {
                door->setOpenState();
            }
        }
        if (roomCfg.doorDown >= 0)
        {
            Door *door = new Door(Door::Down);
            // 下门：水平居中在底部 (600-80) = 520
            door->setPos(340, 520);
            m_scene->addItem(door);
            m_currentDoors.append(door);

            Room *currentRoom = m_rooms[m_currentRoomIndex];
            if (currentRoom && currentRoom->isDoorOpenDown())
            {
                door->setOpenState();
            }
        }
        if (roomCfg.doorLeft >= 0)
        {
            Door *door = new Door(Door::Left);
            // 左门：垂直居中在左侧 (600-120)/2 = 240
            door->setPos(0, 240);
            m_scene->addItem(door);
            m_currentDoors.append(door);

            Room *currentRoom = m_rooms[m_currentRoomIndex];
            if (currentRoom && currentRoom->isDoorOpenLeft())
            {
                door->setOpenState();
            }
        }
        if (roomCfg.doorRight >= 0)
        {
            Door *door = new Door(Door::Right);
            // 右门：垂直居中在右侧 800-80 = 720
            door->setPos(720, 240);
            m_scene->addItem(door);
            m_currentDoors.append(door);

            Room *currentRoom = m_rooms[m_currentRoomIndex];
            if (currentRoom && currentRoom->isDoorOpenRight())
            {
                door->setOpenState();
            }
        }

        // 保存到房间门映射（首次创建时）
        m_roomDoors[m_currentRoomIndex] = m_currentDoors;
    }
    catch (const QString &error)
    {
        qWarning() << "生成门失败:" << error;
    }
}

void Level::buildMinimapData()
{
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber))
        return;

    // BFS to determine coordinates
    struct Node
    {
        int id;
        int x, y;
    };

    QVector<HUD::RoomNode> nodes;
    QVector<bool> visited(config.getRoomCount(), false);
    QList<Node> queue;

    int startRoom = config.getStartRoomIndex();
    queue.append({startRoom, 0, 0});
    visited[startRoom] = true;

    while (!queue.isEmpty())
    {
        Node current = queue.takeFirst();
        const RoomConfig &cfg = config.getRoom(current.id);

        HUD::RoomNode hudNode;
        hudNode.id = current.id;
        hudNode.x = current.x;
        hudNode.y = current.y;
        hudNode.visited = false; // Will be updated by HUD
        hudNode.up = cfg.doorUp;
        hudNode.down = cfg.doorDown;
        hudNode.left = cfg.doorLeft;
        hudNode.right = cfg.doorRight;

        nodes.append(hudNode);

        if (cfg.doorUp >= 0 && !visited[cfg.doorUp])
        {
            visited[cfg.doorUp] = true;
            queue.append({cfg.doorUp, current.x, current.y - 1});
        }
        if (cfg.doorDown >= 0 && !visited[cfg.doorDown])
        {
            visited[cfg.doorDown] = true;
            queue.append({cfg.doorDown, current.x, current.y + 1});
        }
        if (cfg.doorLeft >= 0 && !visited[cfg.doorLeft])
        {
            visited[cfg.doorLeft] = true;
            queue.append({cfg.doorLeft, current.x - 1, current.y});
        }
        if (cfg.doorRight >= 0 && !visited[cfg.doorRight])
        {
            visited[cfg.doorRight] = true;
            queue.append({cfg.doorRight, current.x + 1, current.y});
        }
    }

    // Pass to HUD
    if (auto view = dynamic_cast<GameView *>(parent()))
    {
        if (auto hud = view->getHUD())
        {
            hud->setMapLayout(nodes);
        }
    }
}

void Level::clearSceneEntities()
{
    // Remove enemies from scene only, keep them in room data
    for (QPointer<Enemy> enemyPtr : m_currentEnemies)
    {
        Enemy *enemy = enemyPtr.data();
        if (m_scene && enemy)
        {
            m_scene->removeItem(enemy);
        }
    }
    m_currentEnemies.clear();

    // Remove chests from scene only, keep them in room data
    for (QPointer<Chest> chestPtr : m_currentChests)
    {
        Chest *chest = chestPtr.data();
        if (m_scene && chest)
        {
            m_scene->removeItem(chest);
        }
    }
    m_currentChests.clear();

    // Clear old door items (deprecated)
    for (QGraphicsItem *item : m_doorItems)
    {
        if (m_scene && item)
        {
            m_scene->removeItem(item);
        }
        delete item;
    }
    m_doorItems.clear();

    // Clear new Door objects (don't delete, just remove from scene)
    // 门对象保存在 m_roomDoors 中，会被复用
    for (Door *door : m_currentDoors)
    {
        if (m_scene && door)
        {
            m_scene->removeItem(door);
        }
    }
    m_currentDoors.clear();

    // 清理场景中残留的子弹
    if (m_scene)
    {
        QList<QGraphicsItem *> allItems = m_scene->items();
        QVector<Projectile *> projectilesToDelete;

        // 先收集所有子弹
        for (QGraphicsItem *item : allItems)
        {
            if (auto proj = dynamic_cast<Projectile *>(item))
            {
                projectilesToDelete.append(proj);
            }
        }

        // 再统一删除，避免在遍历时修改容器
        for (Projectile *proj : projectilesToDelete)
        {
            if (proj)
            {
                proj->destroy();
            }
        }
    }
}

void Level::clearCurrentRoomEntities()
{
    if (m_scene)
    {
        m_scene->setBackgroundBrush(QBrush()); // 设置为空画刷
    }
    // Delete all entities completely (used when resetting level)
    for (QPointer<Enemy> enemyPtr : m_currentEnemies)
    {
        Enemy *enemy = enemyPtr.data();
        if (enemy)
        {
            disconnect(enemy, nullptr, this, nullptr);
        }
        for (Room *r : m_rooms)
        {
            if (!r)
                continue;
            r->currentEnemies.removeAll(enemyPtr);
        }
        if (m_scene && enemy)
            m_scene->removeItem(enemy);
        if (enemy)
            delete enemy;
    }
    m_currentEnemies.clear();

    for (QPointer<Chest> chestPtr : m_currentChests)
    {
        Chest *chest = chestPtr.data();
        for (Room *r : m_rooms)
        {
            if (!r)
                continue;
            r->currentChests.removeAll(chestPtr);
        }
        if (m_scene && chest)
            m_scene->removeItem(chest);
        if (chest)
            delete chest;
    }
    m_currentChests.clear();

    for (QGraphicsItem *item : m_doorItems)
    {
        if (m_scene && item)
        {
            m_scene->removeItem(item);
        }
        delete item;
    }
    m_doorItems.clear();

    if (m_scene)
    {
        QList<QGraphicsItem *> allItems = m_scene->items();
        QVector<Projectile *> projectilesToDelete;
        for (QGraphicsItem *item : allItems)
        {
            if (auto proj = dynamic_cast<Projectile *>(item))
            {
                projectilesToDelete.append(proj);
            }
        }
        for (Projectile *proj : projectilesToDelete)
        {
            if (proj)
            {
                proj->destroy();
            }
        }
    }
}

bool Level::enterNextRoom()
{
    Room *currentRoom = m_rooms[m_currentRoomIndex];
    int x = currentRoom->getChangeX();
    int y = currentRoom->getChangeY();

    if (!x && !y)
        return false;

    qDebug() << "enterNextRoom: 检测到切换请求 x=" << x << ", y=" << y;

    AudioManager::instance().playSound("enter_room");
    qDebug() << "进入新房间音效已触发";

    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber))
    {
        qWarning() << "加载关卡配置失败";
        return false;
    }

    const RoomConfig &currentRoomCfg = config.getRoom(m_currentRoomIndex);
    int nextRoomIndex = -1;

    if (y == -1)
    {
        nextRoomIndex = currentRoomCfg.doorUp;
    }
    else if (y == 1)
    {
        nextRoomIndex = currentRoomCfg.doorDown;
    }
    else if (x == -1)
    {
        nextRoomIndex = currentRoomCfg.doorLeft;
    }
    else if (x == 1)
    {
        nextRoomIndex = currentRoomCfg.doorRight;
    }

    if (nextRoomIndex < 0 || nextRoomIndex >= m_rooms.size())
    {
        qDebug() << "无效的目标房间:" << nextRoomIndex;
        currentRoom->resetChangeDir();
        return false;
    }

    qDebug() << "切换房间: 从" << m_currentRoomIndex << "到" << nextRoomIndex;

    m_currentRoomIndex = nextRoomIndex;

    if (y == -1)
    {
        m_player->setPos(m_player->pos().x(), 560);
    }
    else if (y == 1)
    {
        m_player->setPos(m_player->pos().x(), 40);
    }
    else if (x == -1)
    {
        m_player->setPos(760, m_player->pos().y());
    }
    else if (x == 1)
    {
        m_player->setPos(40, m_player->pos().y());
    }

    const RoomConfig &nextRoomCfg = config.getRoom(nextRoomIndex);
    Room *nextRoom = m_rooms[nextRoomIndex];

    // 判断是否是首次进入战斗房间（在visited标记之前判断）
    bool isFirstEnterBattleRoom = !visited[m_currentRoomIndex] && nextRoom->isBattleRoom();

    if (!visited[m_currentRoomIndex])
    {
        visited[m_currentRoomIndex] = true;
        visited_count++;
        initCurrentRoom(m_rooms[m_currentRoomIndex]);
    }
    else
    {
        loadRoom(m_currentRoomIndex);
    }

    // 单向门逻辑：如果是首次进入战斗房间，不打开返回的门
    if (!isFirstEnterBattleRoom)
    {
        // 正常情况：双向开门
        if (y == -1 && nextRoomCfg.doorDown >= 0)
        {
            nextRoom->setDoorOpenDown(true);
        }
        else if (y == 1 && nextRoomCfg.doorUp >= 0)
        {
            nextRoom->setDoorOpenUp(true);
        }
        else if (x == -1 && nextRoomCfg.doorRight >= 0)
        {
            nextRoom->setDoorOpenRight(true);
        }
        else if (x == 1 && nextRoomCfg.doorLeft >= 0)
        {
            nextRoom->setDoorOpenLeft(true);
        }
    }
    else
    {
        qDebug() << "首次进入战斗房间" << nextRoomIndex << "，返回门保持关闭";
    }

    currentRoom->resetChangeDir();
    return true;
}

void Level::loadRoom(int roomIndex)
{
    if (roomIndex < 0 || roomIndex >= m_rooms.size())
    {
        qWarning() << "无效的房间索引:" << roomIndex;
        return;
    }
    if (!visited[roomIndex])
    {
        qWarning() << "房间" << roomIndex << "未被访问过，无法加载";
        return;
    }

    for (Room *rr : m_rooms)
    {
        if (rr)
            rr->stopChangeTimer();
    }

    // Clear scene first
    clearSceneEntities();

    m_currentRoomIndex = roomIndex;
    Room *targetRoom = m_rooms[roomIndex];

    LevelConfig config;
    if (config.loadFromFile(m_levelNumber))
    {
        const RoomConfig &roomCfg = config.getRoom(roomIndex);
        try
        {
            QString bgPath = ConfigManager::instance().getAssetPath(roomCfg.backgroundImage);
            QPixmap bg = ResourceFactory::loadImage(bgPath);
            m_scene->setBackgroundBrush(QBrush(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
            qDebug() << "重新加载房间" << roomIndex << "背景:" << roomCfg.backgroundImage;
        }
        catch (const QString &e)
        {
            qWarning() << "加载地图背景失败:" << e;
        }
        spawnDoors(roomCfg);
    }

    qDebug() << "重新加载房间" << roomIndex << "，房间保存的敌人数:" << targetRoom->currentEnemies.size();

    // 重新添加房间实体到场景（使用 QPointer）
    for (QPointer<Enemy> enemyPtr : targetRoom->currentEnemies)
    {
        Enemy *enemy = enemyPtr.data();
        if (enemy)
        {
            m_scene->addItem(enemy);
            m_currentEnemies.append(enemyPtr);
            qDebug() << "  恢复敌人到场景，位置:" << enemy->pos();
        }
    }

    for (QPointer<Chest> chestPtr : targetRoom->currentChests)
    {
        Chest *chest = chestPtr.data();
        if (chest)
        {
            m_scene->addItem(chest);
            m_currentChests.append(chestPtr);
        }
    }

    qDebug() << "重新加载房间" << roomIndex << "完成，当前场景敌人数:" << m_currentEnemies.size();

    if (m_player)
    {
        m_player->setZValue(100);
    }

    emit roomEntered(roomIndex);

    if (targetRoom)
        targetRoom->startChangeTimer();
}

Room *Level::currentRoom() const
{
    if (m_currentRoomIndex < m_rooms.size())
    {
        return m_rooms[m_currentRoomIndex];
    }
    return nullptr;
}

void Level::onEnemyDying(Enemy *enemy)
{
    qDebug() << "onEnemyDying 被调用";

    if (!enemy)
    {
        qWarning() << "onEnemyDying: enemy 为 null";
        return;
    }

    // 使用 QPointer 包装目标并用 removeAll 安全移除
    QPointer<Enemy> target(enemy);
    m_currentEnemies.removeAll(target);

    qDebug() << "已从全局敌人列表移除，剩余:" << m_currentEnemies.size();

    // 延迟执行bonusEffects，避免在dying信号处理中访问不稳定状态
    // 使用QPointer防止Level已被删除时访问
    QPointer<Level> levelPtr(this);
    QPointer<Player> playerPtr(m_player);
    QTimer::singleShot(100, this, [levelPtr, playerPtr]()
                       {
        if (levelPtr && playerPtr) {
            levelPtr->bonusEffects();
        } });

    for (Room *r : m_rooms)
    {
        if (!r)
            continue;
        r->currentEnemies.removeAll(target);
    }

    Room *cur = m_rooms[m_currentRoomIndex];
    qDebug() << "当前房间" << m_currentRoomIndex << "敌人数量:" << (cur ? cur->currentEnemies.size() : -1);

    if (cur && cur->currentEnemies.isEmpty())
    {
        openDoors(cur);
    }
}

void Level::openDoors(Room *cur)
{
    LevelConfig config;
    bool up = false, down = false, left = false, right = false;
    if (config.loadFromFile(m_levelNumber))
    {
        const RoomConfig &roomCfg = config.getRoom(m_currentRoomIndex);

        AudioManager::instance().playSound("door_open");
        qDebug() << "敌人清空，播放门打开音效和动画";

        int doorIndex = 0;

        if (roomCfg.doorUp >= 0)
        {
            cur->setDoorOpenUp(true);
            up = true;
            // 播放开门动画
            if (doorIndex < m_currentDoors.size())
            {
                m_currentDoors[doorIndex]->open();
            }
            doorIndex++;
            qDebug() << "打开上门，通往房间" << roomCfg.doorUp;

            // 设置邻房间的对应门为打开状态（除非邻居是未访问的战斗房间）
            int neighborRoom = roomCfg.doorUp;
            if (neighborRoom >= 0 && neighborRoom < m_rooms.size())
            {
                Room *neighbor = m_rooms[neighborRoom];
                bool shouldOpenNeighborDoor = !neighbor->isBattleRoom() || visited[neighborRoom];
                if (shouldOpenNeighborDoor)
                {
                    neighbor->setDoorOpenDown(true);
                }
            }
        }
        if (roomCfg.doorDown >= 0)
        {
            cur->setDoorOpenDown(true);
            down = true;
            if (doorIndex < m_currentDoors.size())
            {
                m_currentDoors[doorIndex]->open();
            }
            doorIndex++;
            qDebug() << "打开下门，通往房间" << roomCfg.doorDown;

            int neighborRoom = roomCfg.doorDown;
            if (neighborRoom >= 0 && neighborRoom < m_rooms.size())
            {
                Room *neighbor = m_rooms[neighborRoom];
                bool shouldOpenNeighborDoor = !neighbor->isBattleRoom() || visited[neighborRoom];
                if (shouldOpenNeighborDoor)
                {
                    neighbor->setDoorOpenUp(true);
                }
            }
        }
        if (roomCfg.doorLeft >= 0)
        {
            cur->setDoorOpenLeft(true);
            left = true;
            if (doorIndex < m_currentDoors.size())
            {
                m_currentDoors[doorIndex]->open();
            }
            doorIndex++;
            qDebug() << "打开左门，通往房间" << roomCfg.doorLeft;

            int neighborRoom = roomCfg.doorLeft;
            if (neighborRoom >= 0 && neighborRoom < m_rooms.size())
            {
                Room *neighbor = m_rooms[neighborRoom];
                bool shouldOpenNeighborDoor = !neighbor->isBattleRoom() || visited[neighborRoom];
                if (shouldOpenNeighborDoor)
                {
                    neighbor->setDoorOpenRight(true);
                }
            }
        }
        if (roomCfg.doorRight >= 0)
        {
            cur->setDoorOpenRight(true);
            right = true;
            if (doorIndex < m_currentDoors.size())
            {
                m_currentDoors[doorIndex]->open();
            }
            doorIndex++;
            qDebug() << "打开右门，通往房间" << roomCfg.doorRight;

            int neighborRoom = roomCfg.doorRight;
            if (neighborRoom >= 0 && neighborRoom < m_rooms.size())
            {
                Room *neighbor = m_rooms[neighborRoom];
                bool shouldOpenNeighborDoor = !neighbor->isBattleRoom() || visited[neighborRoom];
                if (shouldOpenNeighborDoor)
                {
                    neighbor->setDoorOpenLeft(true);
                }
            }
        }
    }

    qDebug() << "房间" << m_currentRoomIndex << "敌人全部清空，门已打开";
    if (m_currentRoomIndex == m_rooms.size() - 1)
        emit levelCompleted(m_levelNumber);
    emit enemiesCleared(m_currentRoomIndex, up, down, left, right);
}

void Level::onPlayerDied()
{
    qDebug() << "Level::onPlayerDied - 通知所有敌人移除玩家引用";

    // 先收集当前全局敌人的原始指针，并断开其 dying 信号，避免在调用 setPlayer 时触发 onEnemyDying 导致容器被修改
    QVector<Enemy *> rawEnemies;
    for (QPointer<Enemy> ePtr : m_currentEnemies)
    {
        Enemy *e = ePtr.data();
        if (e)
        {
            QObject::disconnect(e, &Enemy::dying, this, &Level::onEnemyDying);
            rawEnemies.append(e);
        }
    }
    for (Enemy *e : rawEnemies)
    {
        if (e)
            e->setPlayer(nullptr);
    }

    // 对每个房间同样处理：先收集原始指针并断开信号，再调用 setPlayer(nullptr)
    for (Room *r : m_rooms)
    {
        if (!r)
            continue;
        QVector<Enemy *> roomRaw;
        for (QPointer<Enemy> ePtr : r->currentEnemies)
        {
            Enemy *e = ePtr.data();
            if (e)
            {
                QObject::disconnect(e, &Enemy::dying, this, &Level::onEnemyDying);
                roomRaw.append(e);
            }
        }
        for (Enemy *e : roomRaw)
        {
            if (e)
                e->setPlayer(nullptr);
        }
    }
}

void Level::bonusEffects()
{
    if (!m_player)
        return;

    // 随机选择一种效果
    // 0: SpeedEffect
    // 1: DamageEffect
    // 2: shootSpeedEffect
    // 3: decDamage
    // 4: InvincibleEffect
    // 5: soulHeartEffect

    int type = QRandomGenerator::global()->bounded(12);
    if (type >= 6)
        return;
    StatusEffect *effect = nullptr;

    switch (type)
    {
    case 0:
        effect = new SpeedEffect(5, 1.5);
        break;
    case 1:
        effect = new DamageEffect(5, 1.5);
        break;
    case 2:
        effect = new shootSpeedEffect(5, 1.5);
        break;
    case 3:
        effect = new decDamage(5, 0.5);
        break;
    case 4:
        effect = new InvincibleEffect(5);
        break;
    case 5:
        effect = new soulHeartEffect(m_player, 1);
        break;
    }

    if (effect)
    {
        effect->applyTo(m_player);
        // effect会在expire()中调用deleteLater()自我销毁
    }
}
