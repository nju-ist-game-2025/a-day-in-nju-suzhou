#include "level.h"
#include <QDebug>
#include <QFile>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QPointer>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QVariantAnimation>
#include <QtMath>
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
// entities
#include "../entities/boss.h"
#include "../entities/enemy.h"
#include "../entities/player.h"
#include "../entities/projectile.h"
// level_1
#include "../entities/level_1/clockboom.h"
#include "../entities/level_1/clockenemy.h"
#include "../entities/level_1/nightmareboss.h"
#include "../entities/level_1/pillowenemy.h"
// level_2
#include "../entities/level_2/pantsenemy.h"
#include "../entities/level_2/sockenemy.h"
#include "../entities/level_2/sockshooter.h"
#include "../entities/level_2/walker.h"
#include "../entities/level_2/washmachineboss.h"
// level_3
#include "../entities/level_3/digitalsystemenemy.h"
#include "../entities/level_3/optimizationenemy.h"
#include "../entities/level_3/probabilityenemy.h"
#include "../entities/level_3/teacherboss.h"
#include "../entities/level_3/xukeenemy.h"
#include "../entities/level_3/yanglinenemy.h"
#include "../entities/level_3/zhuhaoenemy.h"
// items 和 ui
#include "../items/chest.h"
#include "../items/droppeditem.h"
#include "../items/droppeditemfactory.h"
#include "../ui/dialogsystem.h"
#include "../ui/gameview.h"
#include "../ui/hud.h"
#include "factory/bossfactory.h"
#include "factory/enemyfactory.h"
#include "levelconfig.h"
#include "room.h"

Level::Level(Player* player, QGraphicsScene* scene, QObject* parent)
    : QObject(parent),
      m_levelNumber(1),
      m_player(player),
      m_scene(scene),
      checkChange(nullptr) {
    // 创建房间管理器（完全管理房间数据）
    m_roomManager = new RoomManager(m_player, m_scene, this);

    // 连接RoomManager的敌人创建信号（连接死亡回调）
    connect(m_roomManager, &RoomManager::enemyCreated, this, [this](Enemy* enemy) {
        if (enemy) {
            connect(enemy, &Enemy::dying, this, &Level::onEnemyDying);
        }
    });

    // 连接RoomManager的Boss创建信号（设置Boss特有信号和死亡回调）
    connect(m_roomManager, &RoomManager::bossCreated, this, [this](Boss* boss) {
        if (boss) {
            // 连接死亡信号
            connect(boss, &Boss::dying, this, &Level::onEnemyDying);

            // 设置Boss特有的信号连接（对话、吸纳、召唤敌人等）
            setupBossConnections(boss);

            qDebug() << "Boss dying信号和特有信号已连接";
        }
    });

    // 连接RoomManager的敌人召唤信号（用于Boss召唤敌人时连接死亡回调）
    connect(m_roomManager, &RoomManager::enemySummoned, this, [this](Enemy* enemy) {
        if (enemy) {
            connect(enemy, &Enemy::dying, this, &Level::onEnemyDying);
        }
    });

    // 创建对话系统
    m_dialogSystem = new DialogSystem(m_scene, this);
    connect(m_dialogSystem, &DialogSystem::dialogStarted, this, &Level::dialogStarted);
    connect(m_dialogSystem, &DialogSystem::dialogFinished, this, &Level::dialogFinished);
    connect(m_dialogSystem, &DialogSystem::storyFinished, this, &Level::onDialogSystemStoryFinished);
    connect(m_dialogSystem, &DialogSystem::bossDialogFinished, this, &Level::onDialogSystemBossDialogFinished);
    connect(m_dialogSystem, &DialogSystem::eliteDialogFinished, this, &Level::onDialogSystemEliteDialogFinished);
    connect(m_dialogSystem, &DialogSystem::requestPauseEnemies, this, &Level::pauseAllEnemyTimers);
    connect(m_dialogSystem, &DialogSystem::requestResumeEnemies, this, &Level::resumeAllEnemyTimers);

    // 创建奖励系统并连接信号
    m_rewardSystem = new RewardSystem(m_player, m_scene, this);
    connect(m_rewardSystem, &RewardSystem::requestShowDialog, this, &Level::onRewardShowDialogRequested);
    connect(m_rewardSystem, &RewardSystem::rewardSequenceCompleted, this, &Level::onRewardSequenceCompleted);
    connect(m_rewardSystem, &RewardSystem::requestShowGKeyHint, this, &Level::showGKeyHint);
    connect(m_rewardSystem, &RewardSystem::ticketPickedUp, this, &Level::ticketPickedUp);
}

// ========== 房间管理访问器实现（委托给RoomManager）==========
int Level::currentRoomIndex() const {
    return m_roomManager->currentRoomIndex();
}
void Level::setCurrentRoomIndex(int index) {
    m_roomManager->setCurrentRoomIndex(index);
}
QVector<Room*>& Level::rooms() {
    return m_roomManager->roomsRef();
}
const QVector<Room*>& Level::rooms() const {
    return m_roomManager->roomsRef();
}
QVector<QPointer<Enemy>>& Level::currentEnemies() {
    return m_roomManager->currentEnemies();
}
QVector<QPointer<Chest>>& Level::currentChests() {
    return m_roomManager->currentChests();
}
QVector<Door*>& Level::currentDoors() {
    return m_roomManager->currentDoorsRef();
}
QMap<int, QVector<Door*>>& Level::roomDoors() {
    return m_roomManager->roomDoorsRef();
}
QVector<bool>& Level::visitedRooms() {
    return m_roomManager->visitedRoomsRef();
}
const QVector<bool>& Level::visitedRooms() const {
    return m_roomManager->visitedRooms();
}
int& Level::visitedCount() {
    return m_roomManager->visitedCountRef();
}
bool Level::hasEncounteredBossDoor() const {
    return m_roomManager->hasEncounteredBossDoor();
}
void Level::setHasEncounteredBossDoor(bool value) {
    m_roomManager->setHasEncounteredBossDoor(value);
}
bool Level::bossDoorsAlreadyOpened() const {
    return m_roomManager->bossDoorsAlreadyOpened();
}
void Level::setBossDoorsAlreadyOpened(bool value) {
    m_roomManager->setBossDoorsAlreadyOpened(value);
}

Level::~Level() {
    // 断开所有信号连接，防止析构后回调
    disconnect(this, nullptr, nullptr, nullptr);

    // 清理背景图片项
    if (m_backgroundItem) {
        if (m_scene)
            m_scene->removeItem(m_backgroundItem);
        delete m_backgroundItem;
        m_backgroundItem = nullptr;
    }
    if (m_backgroundOverlay) {
        if (m_scene)
            m_scene->removeItem(m_backgroundOverlay);
        delete m_backgroundOverlay;
        m_backgroundOverlay = nullptr;
    }

    if (checkChange) {
        checkChange->stop();
        disconnect(checkChange, nullptr, nullptr, nullptr);
        delete checkChange;
        checkChange = nullptr;
    }

    // 清理吸收动画定时器
    if (m_absorbAnimationTimer) {
        m_absorbAnimationTimer->stop();
        disconnect(m_absorbAnimationTimer, nullptr, nullptr, nullptr);
        delete m_absorbAnimationTimer;
        m_absorbAnimationTimer = nullptr;
    }

    // 清理所有敌人的信号连接（通过RoomManager访问）
    for (QPointer<Enemy> ePtr : currentEnemies()) {
        if (Enemy* e = ePtr.data()) {
            disconnect(e, nullptr, this, nullptr);
        }
    }

    // DialogSystem、RoomManager、RewardSystem 会作为QObject子对象自动被删除
}

void Level::init(int levelNumber) {
    if (checkChange) {
        checkChange->stop();
        delete checkChange;
        checkChange = nullptr;
    }

    // 清理房间管理器（会清理门、房间、敌人、宝箱等）
    m_roomManager->cleanup();

    m_levelNumber = levelNumber;
    m_roomManager->setLevelNumber(levelNumber);
    setHasEncounteredBossDoor(false);
    setBossDoorsAlreadyOpened(false);

    // 重置Boss相关状态，防止跨关卡状态污染导致崩溃
    m_currentWashMachineBoss = nullptr;
    m_currentTeacherBoss = nullptr;
    m_bossDefeated = false;
    // 重置奖励系统状态
    if (m_rewardSystem) {
        m_rewardSystem->cleanup();
    }
    hideGKeyHint();  // 隐藏G键提示

    // 重置精英房间状态
    m_isEliteRoom = false;
    m_elitePhase2Triggered = false;
    m_eliteYanglinDeathCount = 0;
    m_isEliteDialog = false;
    m_zhuhaoEnemy = nullptr;

    // 设置对话系统参数
    m_dialogSystem->setLevelNumber(levelNumber);
    m_dialogSystem->setTeacherBoss(nullptr);
    m_dialogSystem->setBossDefeated(false);

    LevelConfig config;
    if (!config.loadFromFile(levelNumber)) {
        qWarning() << "加载关卡配置失败，使用默认配置";
        return;
    }

    qDebug() << "加载关卡:" << config.getLevelName();
    qDebug() << "关卡描述条数:" << config.getDescription().size();

    // 开发者模式
    if (m_skipToBoss) {
        qDebug() << "开发者模式: 直接进入Boss对话";
        // 直接初始化关卡（跳过开头对话的显示，但内部状态正确初始化）
        initializeLevelAfterStory(config);
        // 延迟发出storyFinished信号，确保GameView的连接已建立
        QTimer::singleShot(0, this, [this]() { emit storyFinished(); });
        return;
    }

    // 正常流程：如果有剧情描述，显示剧情；否则直接初始化关卡
    if (!config.getDescription().isEmpty()) {
        // 断开之前的所有 storyFinished 连接，避免重复触发
        disconnect(this, &Level::storyFinished, nullptr, nullptr);
        connect(this, &Level::storyFinished, this, [this, config]() { initializeLevelAfterStory(config); });
        showLevelStartText(config);
        showStoryDialog(config.getDescription());
    } else {
        showLevelStartText(config);
        initializeLevelAfterStory(config);
    }
}

void Level::initializeLevelAfterStory(const LevelConfig& config) {
    qDebug() << "加载关卡:" << config.getLevelName();

    bool isDevMode = m_skipToBoss;  // 保存开发者模式状态

    for (int i = 0; i < config.getRoomCount(); ++i) {
        const RoomConfig& roomCfg = config.getRoom(i);
        bool hasUp = (roomCfg.doorUp >= 0);
        bool hasDown = (roomCfg.doorDown >= 0);
        bool hasLeft = (roomCfg.doorLeft >= 0);
        bool hasRight = (roomCfg.doorRight >= 0);

        Room* room = new Room(m_player, hasUp, hasDown, hasLeft, hasRight);

        // 根据配置自动判断战斗房间（有敌人或有Boss的房间）
        if (roomCfg.enemyCount > 0 || roomCfg.hasBoss) {
            room->setBattleRoom(true);
            qDebug() << "房间" << i << "标记为战斗房间（敌人数:" << roomCfg.enemyCount << "，Boss:" << roomCfg.hasBoss
                     << "）";
        }

        rooms().append(room);
    }

    visitedRooms().resize(config.getRoomCount());
    visitedRooms().fill(false);

    int startRoomIndex = config.getStartRoomIndex();
    int bossRoomIndex = -1;

    // 开发者模式：模拟遍历所有房间，直接进入boss房
    if (isDevMode) {
        // 找到Boss房索引
        for (int i = 0; i < config.getRoomCount(); ++i) {
            if (config.getRoom(i).hasBoss) {
                bossRoomIndex = i;
                break;
            }
        }

        if (bossRoomIndex >= 0) {
            // 标记所有非boss房间为已访问且已清除
            for (int i = 0; i < config.getRoomCount(); ++i) {
                if (!config.getRoom(i).hasBoss) {
                    visitedRooms()[i] = true;
                    if (rooms()[i]) {
                        rooms()[i]->setCleared(true);
                    }
                }
            }
            visitedCount() = config.getRoomCount() - 1;

            // 设置boss门状态
            setHasEncounteredBossDoor(true);
            setBossDoorsAlreadyOpened(true);
            // 跳过boss时对话也标记为完成
            if (m_dialogSystem) {
                m_dialogSystem->finishStory();
            }

            // 直接进入boss房
            setCurrentRoomIndex(bossRoomIndex);
            qDebug() << "开发者模式: 模拟完成，进入Boss房" << bossRoomIndex;
        } else {
            // 没找到boss房，使用正常流程
            visitedRooms()[startRoomIndex] = true;
            visitedCount() = 1;
            setCurrentRoomIndex(startRoomIndex);
        }

        m_skipToBoss = false;  // 重置标志
    } else {
        // 正常流程：从起始房间开始
        visitedRooms()[startRoomIndex] = true;
        visitedCount() = 1;
        setCurrentRoomIndex(startRoomIndex);
    }

    const RoomConfig& currentRoomCfg = config.getRoom(currentRoomIndex());

    // 创建背景图片项
    if (!m_backgroundItem) {
        m_backgroundItem = new QGraphicsPixmapItem();
        m_backgroundItem->setZValue(-1000);  // 设置最低优先级
        m_scene->addItem(m_backgroundItem);
    }

    try {
        QString bgPath = ConfigManager::instance().getAssetPath(currentRoomCfg.backgroundImage);
        QPixmap bg = ResourceFactory::loadImage(bgPath);
        m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        m_backgroundItem->setPos(0, 0);
        // 保存当前与原始背景路径（相对assets/）
        m_currentBackgroundPath = currentRoomCfg.backgroundImage;
        m_originalBackgroundPath = m_currentBackgroundPath;
    } catch (const QString& e) {
        qWarning() << "加载房间背景失败:" << e;
    }

    // 开发者模式：显示boss对话，对话结束后初始化boss房
    bool isShowingDevModeBossDialog = false;
    if (isDevMode && bossRoomIndex >= 0 && !currentRoomCfg.bossDialog.isEmpty()) {
        visitedRooms()[bossRoomIndex] = true;
        visitedCount()++;
        isShowingDevModeBossDialog = true;
        showStoryDialog(currentRoomCfg.bossDialog, true, currentRoomCfg.bossDialogBackground);
    } else {
        // 正常初始化当前房间
        initCurrentRoom(rooms()[currentRoomIndex()]);
    }

    checkChange = new QTimer(this);
    connect(checkChange, &QTimer::timeout, this, &Level::enterNextRoom);
    // 开发者模式下显示boss对话时，不启动checkChange计时器，等对话结束后再启动
    if (!isShowingDevModeBossDialog) {
        checkChange->start(100);
    }

    buildMinimapData();
}

// ========== 对话系统回调槽函数 ==========

// 委托给DialogSystem
void Level::showStoryDialog(const QStringList& dialogs, bool isBossDialog, const QString& customBackground) {
    // 设置精英房间对话标志
    if (m_isEliteDialog) {
        m_dialogSystem->setEliteDialog(true);
    }
    // 暂停所有房间的定时器（防止对话期间玩家被攻击）
    if (isBossDialog) {
        for (Room* room : rooms()) {
            if (room) {
                room->stopChangeTimer();
            }
        }
    }
    m_dialogSystem->showStoryDialog(dialogs, isBossDialog, customBackground);
}

void Level::fadeDialogBackgroundTo(const QString& imagePath, int duration) {
    m_dialogSystem->fadeDialogBackgroundTo(imagePath, duration);
}

// 关卡开始剧情结束回调
void Level::onDialogSystemStoryFinished() {
    emit storyFinished();
}

// Boss对话结束回调
void Level::onDialogSystemBossDialogFinished() {
    // 检查是否是WashMachineBoss的中途对话（变异阶段）
    if (m_currentWashMachineBoss && m_currentWashMachineBoss->getPhase() >= 2) {
        qDebug() << "WashMachineBoss对话结束，通知Boss继续战斗";

        // 释放所有被吸纳的实体（随机分布在场景中）
        for (int i = 0; i < m_absorbingItems.size(); ++i) {
            QGraphicsItem* item = m_absorbingItems[i];
            if (!item)
                continue;

            if (item == m_player) {
                // 恢复玩家
                m_player->setVisible(true);
                m_player->setCanMove(true);
                m_player->setCanShoot(true);
                m_player->setPermanentInvincible(false);
                m_player->setScale(1.0);
                m_player->setPos(400, 450);
                qDebug() << "[吸纳] 玩家已恢复";
            } else if (item != m_currentWashMachineBoss) {
                Enemy* enemy = dynamic_cast<Enemy*>(item);
                if (enemy) {
                    int x = QRandomGenerator::global()->bounded(100, 700);
                    int y = QRandomGenerator::global()->bounded(100, 500);
                    enemy->setPos(x, y);
                    enemy->setScale(1.0);
                    enemy->setVisible(true);
                    enemy->resumeTimers();
                    qDebug() << "释放敌人到位置:" << x << "," << y;
                }
            }
        }

        m_absorbingItems.clear();
        m_absorbStartPositions.clear();
        m_absorbAngles.clear();

        m_currentWashMachineBoss->onDialogFinished();
    } else if (m_currentTeacherBoss && m_currentTeacherBoss->getPhase() >= 2) {
        qDebug() << "TeacherBoss阶段转换对话结束，继续战斗";
        m_currentTeacherBoss->onDialogFinished();
    } else if (m_currentTeacherBoss && m_currentTeacherBoss->getPhase() == 1) {
        qDebug() << "TeacherBoss初始对话结束，通知Boss开始第一阶段战斗";
        m_currentTeacherBoss->onDialogFinished();
    } else {
        // WashMachineBoss或其他Boss的初始对话结束
        if (currentRoomIndex() >= 0 && currentRoomIndex() < rooms().size()) {
            qDebug() << "Boss对话结束，初始化boss房间" << currentRoomIndex();
            initCurrentRoom(rooms()[currentRoomIndex()]);
        }
        if (checkChange && !checkChange->isActive()) {
            checkChange->start(100);
        }
        if (m_currentTeacherBoss) {
            qDebug() << "TeacherBoss实例创建完成，通知其进入战斗";
            m_currentTeacherBoss->onDialogFinished();
        } else if (m_currentWashMachineBoss) {
            m_currentWashMachineBoss->onDialogFinished();
        }
    }
}

// 精英房间对话结束回调
void Level::onDialogSystemEliteDialogFinished() {
    m_isEliteDialog = false;

    if (m_elitePhase2Triggered) {
        qDebug() << "精英房间第二阶段对话结束，生成祝昊";
        spawnZhuhaoEnemy();
        resumeAllEnemyTimers();
    } else {
        qDebug() << "精英房间初始对话结束，初始化房间";
        if (currentRoomIndex() >= 0 && currentRoomIndex() < rooms().size()) {
            initCurrentRoom(rooms()[currentRoomIndex()]);
        }
    }
}

void Level::showPhaseTransitionText(const QString& text, const QColor& color) {
    m_dialogSystem->showPhaseTransitionText(text, color);
}

void Level::showCredits(const QStringList& desc) {
    m_dialogSystem->showCredits(desc);
}

void Level::showLevelStartText(LevelConfig& config) {
    m_dialogSystem->showLevelStartText(config.getLevelName());
}

void Level::initCurrentRoom(Room* room) {
    if (currentRoomIndex() >= rooms().size())
        return;

    for (Room* rr : rooms()) {
        if (rr)
            rr->stopChangeTimer();
    }

    clearSceneEntities();

    LevelConfig config;
    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig& roomCfg = config.getRoom(currentRoomIndex());
        try {
            QString bgPath = ConfigManager::instance().getAssetPath(roomCfg.backgroundImage);
            QPixmap bg = ResourceFactory::loadImage(bgPath);
            if (m_backgroundItem) {
                m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                m_backgroundItem->setPos(0, 0);
            }
            qDebug() << "加载房间" << currentRoomIndex() << "背景:" << roomCfg.backgroundImage;
        } catch (const QString& e) {
            qWarning() << "加载地图背景失败:" << e;
        }
        spawnDoors(roomCfg);
    }

    qDebug() << "初始化房间" << currentRoomIndex() << "，开始生成实体";
    spawnEnemiesInRoom(currentRoomIndex());
    spawnChestsInRoom(currentRoomIndex());
    qDebug() << "初始化房间" << currentRoomIndex() << "完成，房间敌人数:" << room->currentEnemies.size() << "，全局敌人数:"
             << currentEnemies().size();

    // 如果是战斗房间且尚未开始战斗，且有敌人，则标记战斗开始
    if (room && room->isBattleRoom() && !room->isBattleStarted() && !room->currentEnemies.isEmpty()) {
        qDebug() << "战斗房间" << currentRoomIndex() << "触发战斗";
        room->startBattle();
    }

    if (m_player) {
        m_player->setZValue(100);
    }

    emit roomEntered(currentRoomIndex());

    if (room)
        room->startChangeTimer();
}

void Level::spawnEnemiesInRoom(int roomIndex) {
    // 委托给 RoomManager 处理
    // 先设置当前房间索引，确保 RoomManager 生成正确房间的敌人
    if (m_roomManager->currentRoomIndex() != roomIndex) {
        m_roomManager->setCurrentRoomIndex(roomIndex);
    }
    m_roomManager->spawnEnemiesInRoom();
}

// 在当前房间生成宝箱（委托给RoomManager）
void Level::spawnChestsInRoom(int roomIndex) {
    Q_UNUSED(roomIndex)  // RoomManager 使用当前房间索引
    m_roomManager->spawnChestsInRoom();
}

// 渐变背景到目标图片路径（绝对或相对 assets/），duration 毫秒
void Level::fadeBackgroundTo(const QString& imagePath, int duration) {
    if (!m_scene)
        return;

    QString fullPath = imagePath;
    // 如果路径看起来像相对assets路径（未包含 assets/ 前缀），尝试从 ConfigManager 获取
    if (!imagePath.startsWith("assets/")) {
        fullPath = ConfigManager::instance().getAssetPath(imagePath);
    }

    QPixmap newBg;
    try {
        newBg = ResourceFactory::loadImage(fullPath);
    } catch (const QString& e) {
        qWarning() << "fadeBackgroundTo: 加载图片失败:" << e;
        return;
    }

    if (newBg.isNull()) {
        qWarning() << "fadeBackgroundTo: 无法加载图片:" << fullPath;
        return;
    }

    // 创建覆盖项（位于背景之上，但低于场景中其它元素）
    if (m_backgroundOverlay) {
        // 如果已有覆盖，移除并删除
        if (m_scene)
            m_scene->removeItem(m_backgroundOverlay);
        delete m_backgroundOverlay;
        m_backgroundOverlay = nullptr;
    }

    m_backgroundOverlay = new QGraphicsPixmapItem(
        newBg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    m_backgroundOverlay->setPos(0, 0);
    // 覆盖在背景之上，但低于界面元素
    m_backgroundOverlay->setZValue(-999);
    m_backgroundOverlay->setOpacity(0.0);
    m_scene->addItem(m_backgroundOverlay);

    // 动画从0到1
    QVariantAnimation* anim = new QVariantAnimation(this);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setDuration(duration);
    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        if (m_backgroundOverlay)
            m_backgroundOverlay->setOpacity(value.toDouble());
    });
    connect(anim, &QVariantAnimation::finished, this, [this, fullPath]() {
        // 动画结束后，把主背景替换为目标图，并移除覆盖项
        if (m_backgroundItem) {
            try {
                QPixmap bg = ResourceFactory::loadImage(fullPath);
                if (!bg.isNull())
                    m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                // 更新当前背景路径
                m_currentBackgroundPath = fullPath;
            } catch (const QString& e) {
                qWarning() << "fadeBackgroundTo finished: 加载图片失败:" << e;
            }
        }
        if (m_backgroundOverlay && m_scene) {
            m_scene->removeItem(m_backgroundOverlay);
            delete m_backgroundOverlay;
            m_backgroundOverlay = nullptr;
        }
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Level::setupBossConnections(Boss* boss) {
    if (!boss)
        return;

    // 根据具体类型设置Boss特有的信号连接
    if (NightmareBoss* nightmareBoss = dynamic_cast<NightmareBoss*>(boss)) {
        qDebug() << "[Level] 设置Nightmare Boss信号连接（第一关）";
        // 连接Nightmare Boss的特殊信号
        connect(nightmareBoss, &NightmareBoss::requestSpawnEnemies,
                this, &Level::spawnEnemiesForBoss);
        // 当一阶段亡语触发时，开始背景渐变到噩梦关专用背景
        connect(nightmareBoss, &NightmareBoss::phase1DeathTriggered,
                this, [this]() { this->fadeBackgroundTo(QString("assets/background/nightmare2_map.png"), 3000); });
    } else if (WashMachineBoss* washMachineBoss = dynamic_cast<WashMachineBoss*>(boss)) {
        qDebug() << "[Level] 设置WashMachine Boss信号连接（第二关）";
        m_currentWashMachineBoss = washMachineBoss;
        washMachineBoss->setupLevelConnections(this);
    } else if (TeacherBoss* teacherBoss = dynamic_cast<TeacherBoss*>(boss)) {
        qDebug() << "[Level] 设置Teacher Boss信号连接（第三关）";
        m_currentTeacherBoss = teacherBoss;
        teacherBoss->setScene(m_scene);  // 设置场景引用，用于生成粉笔、监考员等实体
        teacherBoss->setupLevelConnections(this);
    } else {
        qDebug() << "[Level] 使用默认Boss，无特殊信号连接";
    }
}

Boss* Level::createBossByLevel(int levelNumber, const QPixmap& pic, double scale) {
    // 使用 BossFactory 创建 Boss 实例
    Boss* boss = BossFactory::instance().createBoss(levelNumber, pic, scale, m_scene);

    // 设置Boss信号连接
    setupBossConnections(boss);

    return boss;
}

void Level::spawnDoors(const RoomConfig& roomCfg) {
    // 委托给RoomManager
    m_roomManager->spawnDoors(roomCfg);
}

void Level::buildMinimapData() {
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber))
        return;

    // BFS
    struct Node {
        int id;
        int x, y;
    };

    QVector<HUD::RoomNode> nodes;
    QVector<bool> bfsVisited(config.getRoomCount(), false);
    QList<Node> queue;

    int startRoom = config.getStartRoomIndex();
    queue.append({startRoom, 0, 0});
    bfsVisited[startRoom] = true;

    while (!queue.isEmpty()) {
        Node current = queue.takeFirst();
        const RoomConfig& cfg = config.getRoom(current.id);

        HUD::RoomNode hudNode;
        hudNode.id = current.id;
        hudNode.x = current.x;
        hudNode.y = current.y;
        hudNode.visited = false;        // Will be updated by HUD
        hudNode.hasBoss = cfg.hasBoss;  // 标记boss房间
        hudNode.up = cfg.doorUp;
        hudNode.down = cfg.doorDown;
        hudNode.left = cfg.doorLeft;
        hudNode.right = cfg.doorRight;

        nodes.append(hudNode);

        if (cfg.doorUp >= 0 && !bfsVisited[cfg.doorUp]) {
            bfsVisited[cfg.doorUp] = true;
            queue.append({cfg.doorUp, current.x, current.y - 1});
        }
        if (cfg.doorDown >= 0 && !bfsVisited[cfg.doorDown]) {
            bfsVisited[cfg.doorDown] = true;
            queue.append({cfg.doorDown, current.x, current.y + 1});
        }
        if (cfg.doorLeft >= 0 && !bfsVisited[cfg.doorLeft]) {
            bfsVisited[cfg.doorLeft] = true;
            queue.append({cfg.doorLeft, current.x - 1, current.y});
        }
        if (cfg.doorRight >= 0 && !bfsVisited[cfg.doorRight]) {
            bfsVisited[cfg.doorRight] = true;
            queue.append({cfg.doorRight, current.x + 1, current.y});
        }
    }

    if (auto view = dynamic_cast<GameView*>(parent())) {
        if (auto hud = view->getHUD()) {
            hud->setMapLayout(nodes);
            // 同步玩家已访问的房间状态（使用RoomManager的visited数组）
            hud->syncVisitedRooms(visitedRooms());
        }
    }
}

void Level::clearSceneEntities() {
    for (QPointer<Enemy> enemyPtr : currentEnemies()) {
        Enemy* enemy = enemyPtr.data();
        if (m_scene && enemy) {
            m_scene->removeItem(enemy);
        }
    }
    currentEnemies().clear();

    for (QPointer<Chest> chestPtr : currentChests()) {
        Chest* chest = chestPtr.data();
        if (m_scene && chest) {
            m_scene->removeItem(chest);
        }
    }
    currentChests().clear();

    // 掉落物品的保存应该在 enterNextRoom 中完成（在修改 currentRoomIndex() 之前）
    // clearSceneEntities 只负责清理场景中的子弹等临时物品，不处理掉落物品的保存

    // 门对象保存在 roomDoors() 中，会被复用
    for (Door* door : currentDoors()) {
        if (m_scene && door) {
            m_scene->removeItem(door);
        }
    }
    currentDoors().clear();

    // 清理场景中残留的子弹和毒液轨迹
    if (m_scene) {
        QList<QGraphicsItem*> allItems = m_scene->items();
        QVector<Projectile*> projectilesToDelete;
        QVector<PoisonTrail*> poisonTrailsToDelete;

        // 先收集所有子弹和毒液轨迹
        for (QGraphicsItem* item : allItems) {
            if (auto proj = dynamic_cast<Projectile*>(item)) {
                projectilesToDelete.append(proj);
            } else if (auto trail = dynamic_cast<PoisonTrail*>(item)) {
                poisonTrailsToDelete.append(trail);
            }
        }

        // 再统一删除，避免在遍历时修改容器
        for (Projectile* proj : projectilesToDelete) {
            if (proj) {
                proj->destroy();
            }
        }

        // 删除毒液轨迹
        for (PoisonTrail* trail : poisonTrailsToDelete) {
            if (trail) {
                m_scene->removeItem(trail);
                delete trail;
            }
        }
    }
}

void Level::clearCurrentRoomEntities() {
    // 遮罩效果现在由NightmareBoss自己管理，其析构函数会自动清理
    for (QPointer<Enemy> enemyPtr : currentEnemies()) {
        Enemy* enemy = enemyPtr.data();
        if (enemy) {
            // 断开所有信号连接
            disconnect(enemy, nullptr, this, nullptr);
            disconnect(enemy, nullptr, nullptr, nullptr);
        }
        for (Room* r : rooms()) {
            if (!r)
                continue;
            r->currentEnemies.removeAll(enemyPtr);
        }
        if (m_scene && enemy)
            m_scene->removeItem(enemy);
        if (enemy)
            delete enemy;
    }
    currentEnemies().clear();

    for (QPointer<Chest> chestPtr : currentChests()) {
        Chest* chest = chestPtr.data();
        for (Room* r : rooms()) {
            if (!r)
                continue;
            r->currentChests.removeAll(chestPtr);
        }
        if (m_scene && chest)
            m_scene->removeItem(chest);
        if (chest)
            delete chest;
    }
    currentChests().clear();

    // m_doorItems已移除，门对象现在保存在RoomManager中

    if (m_scene) {
        QList<QGraphicsItem*> allItems = m_scene->items();
        QVector<Projectile*> projectilesToDelete;
        for (QGraphicsItem* item : allItems) {
            if (auto proj = dynamic_cast<Projectile*>(item)) {
                projectilesToDelete.append(proj);
            }
        }
        for (Projectile* proj : projectilesToDelete) {
            if (proj) {
                proj->destroy();
            }
        }
    }
}

bool Level::enterNextRoom() {
    // 安全检查：确保索引和房间对象有效
    if (currentRoomIndex() < 0 || currentRoomIndex() >= rooms().size()) {
        qWarning() << "enterNextRoom: 无效的当前房间索引:" << currentRoomIndex();
        return false;
    }

    Room* currentRoom = rooms()[currentRoomIndex()];
    if (!currentRoom) {
        qWarning() << "enterNextRoom: 当前房间对象为空，索引:" << currentRoomIndex();
        return false;
    }

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

    const RoomConfig& currentRoomCfg = config.getRoom(currentRoomIndex());

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

    if (nextRoomIndex < 0 || nextRoomIndex >= rooms().size()) {
        qDebug() << "无效的目标房间:" << nextRoomIndex;
        currentRoom->resetChangeDir();
        return false;
    }

    qDebug() << "切换房间: 从" << currentRoomIndex() << "到" << nextRoomIndex;

    // 在切换房间之前，保存当前房间的掉落物品
    currentRoom->saveDroppedItemsFromScene(m_scene);

    setCurrentRoomIndex(nextRoomIndex);

    if (y == -1) {
        m_player->setPos(m_player->pos().x(), 560);
    } else if (y == 1) {
        m_player->setPos(m_player->pos().x(), 40);
    } else if (x == -1) {
        m_player->setPos(760, m_player->pos().y());
    } else if (x == 1) {
        m_player->setPos(40, m_player->pos().y());
    }

    const RoomConfig& nextRoomCfg = config.getRoom(nextRoomIndex);
    Room* nextRoom = rooms()[nextRoomIndex];

    // 判断是否是首次进入战斗房间（在visited标记之前判断）
    bool isFirstEnterBattleRoom = !visitedRooms()[currentRoomIndex()] && nextRoom->isBattleRoom();

    // 检测是否首次进入boss房间（使用之前已声明的currentRoomCfg）
    const RoomConfig& newRoomCfg = config.getRoom(currentRoomIndex());
    bool isFirstEnterBossRoom = !visitedRooms()[currentRoomIndex()] && newRoomCfg.hasBoss;
    bool isFirstEnterEliteRoom = !visitedRooms()[currentRoomIndex()] && newRoomCfg.isEliteRoom;

    // 检测是否首次进入cloudDream背景的战斗房间（有敌人时）
    bool isFirstEnterCloudDreamRoom = !visitedRooms()[currentRoomIndex()] &&
                                      newRoomCfg.backgroundImage == "cloudDream" &&
                                      !newRoomCfg.enemies.isEmpty();

    if (!visitedRooms()[currentRoomIndex()]) {
        visitedRooms()[currentRoomIndex()] = true;
        visitedCount()++;

        // 检查是否遇到通往boss房间的门
        if (!hasEncounteredBossDoor() && !newRoomCfg.hasBoss) {
            if ((newRoomCfg.doorUp >= 0 && config.getRoom(newRoomCfg.doorUp).hasBoss) ||
                (newRoomCfg.doorDown >= 0 && config.getRoom(newRoomCfg.doorDown).hasBoss) ||
                (newRoomCfg.doorLeft >= 0 && config.getRoom(newRoomCfg.doorLeft).hasBoss) ||
                (newRoomCfg.doorRight >= 0 && config.getRoom(newRoomCfg.doorRight).hasBoss)) {
                setHasEncounteredBossDoor(true);
                qDebug() << "首次遇到boss门，标记状态";
            }
        }

        // 如果是首次进入精英房间且有对话配置，显示对话
        if (isFirstEnterEliteRoom && !newRoomCfg.eliteDialog.isEmpty()) {
            qDebug() << "首次进入精英房间" << currentRoomIndex() << "，显示精英房间对话";
            m_isEliteRoom = true;
            m_elitePhase2Triggered = false;
            m_eliteYanglinDeathCount = 0;
            m_isEliteDialog = true;
            showStoryDialog(newRoomCfg.eliteDialog, false, newRoomCfg.eliteDialogBackground);
            // 等待对话结束后再初始化房间
        }
        // 如果是首次进入boss房间且有对话配置，显示对话
        else if (isFirstEnterBossRoom && !newRoomCfg.bossDialog.isEmpty()) {
            qDebug() << "首次进入boss房间" << currentRoomIndex() << "，显示boss对话";
            // 传递boss对话标志和自定义背景（如果有）
            showStoryDialog(newRoomCfg.bossDialog, true, newRoomCfg.bossDialogBackground);
            // 等待对话结束后再初始化房间，通过finishStory处理
            // initCurrentRoom会在对话结束后由finishStory调用
        } else {
            initCurrentRoom(rooms()[currentRoomIndex()]);
            // 首次进入cloudDream背景的战斗房间时，显示"你已坠入梦境！"提示
            if (isFirstEnterCloudDreamRoom) {
                qDebug() << "首次进入cloudDream背景房间，显示沉睡提示";
                showPhaseTransitionText("你已坠入梦境！", Qt::red);
            }
        }
    } else {
        loadRoom(currentRoomIndex());
    }

    // 在进入非boss房间后，检查是否满足打开boss门的条件
    if (!newRoomCfg.hasBoss && hasEncounteredBossDoor() && !bossDoorsAlreadyOpened()) {
        if (canOpenBossDoor()) {
            qDebug() << "进入房间后检测到满足boss门开启条件，立即打开boss门";
            setBossDoorsAlreadyOpened(true);
            openBossDoors();
        }
    }

    // 单向门逻辑：如果是首次进入战斗房间，不打开返回的门
    if (!isFirstEnterBattleRoom) {
        // 正常情况：双向开门
        if (y == -1 && nextRoomCfg.doorDown >= 0) {
            nextRoom->setDoorOpenDown(true);
        } else if (y == 1 && nextRoomCfg.doorUp >= 0) {
            nextRoom->setDoorOpenUp(true);
        } else if (x == -1 && nextRoomCfg.doorRight >= 0) {
            nextRoom->setDoorOpenRight(true);
        } else if (x == 1 && nextRoomCfg.doorLeft >= 0) {
            nextRoom->setDoorOpenLeft(true);
        }
    } else {
        qDebug() << "首次进入战斗房间" << nextRoomIndex << "，返回门保持关闭";
    }

    currentRoom->resetChangeDir();
    return true;
}

void Level::loadRoom(int roomIndex) {
    if (roomIndex < 0 || roomIndex >= rooms().size()) {
        qWarning() << "无效的房间索引:" << roomIndex;
        return;
    }
    if (!visitedRooms()[roomIndex]) {
        qWarning() << "房间" << roomIndex << "未被访问过，无法加载";
        return;
    }

    for (Room* rr : rooms()) {
        if (rr)
            rr->stopChangeTimer();
    }

    // Clear scene first
    clearSceneEntities();

    setCurrentRoomIndex(roomIndex);
    Room* targetRoom = rooms()[roomIndex];

    LevelConfig config;
    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig& roomCfg = config.getRoom(roomIndex);
        try {
            QString bgPath = ConfigManager::instance().getAssetPath(roomCfg.backgroundImage);
            QPixmap bg = ResourceFactory::loadImage(bgPath);
            if (m_backgroundItem) {
                m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                m_backgroundItem->setPos(0, 0);
            }
            qDebug() << "重新加载房间" << roomIndex << "背景:" << roomCfg.backgroundImage;
        } catch (const QString& e) {
            qWarning() << "加载地图背景失败:" << e;
        }
        spawnDoors(roomCfg);
    }

    qDebug() << "重新加载房间" << roomIndex << "，房间保存的敌人数:" << targetRoom->currentEnemies.size();

    // 重新添加房间实体到场景（使用 QPointer）
    for (QPointer<Enemy> enemyPtr : targetRoom->currentEnemies) {
        Enemy* enemy = enemyPtr.data();
        if (enemy) {
            m_scene->addItem(enemy);
            currentEnemies().append(enemyPtr);
            qDebug() << "  恢复敌人到场景，位置:" << enemy->pos();
        }
    }

    for (QPointer<Chest> chestPtr : targetRoom->currentChests) {
        Chest* chest = chestPtr.data();
        if (chest) {
            m_scene->addItem(chest);
            currentChests().append(chestPtr);
        }
    }

    // 恢复掉落物品到场景
    targetRoom->restoreDroppedItemsToScene(m_scene);

    qDebug() << "重新加载房间" << roomIndex << "完成，当前场景敌人数:" << currentEnemies().size();

    if (m_player) {
        m_player->setZValue(100);
    }

    emit roomEntered(roomIndex);

    if (targetRoom)
        targetRoom->startChangeTimer();

    // 在重新加载非boss房间后，检查是否满足打开boss门的条件
    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig& roomCfg = config.getRoom(roomIndex);
        if (!roomCfg.hasBoss && hasEncounteredBossDoor() && !bossDoorsAlreadyOpened()) {
            if (canOpenBossDoor()) {
                qDebug() << "重新加载房间后检测到满足boss门开启条件，立即打开boss门";
                setBossDoorsAlreadyOpened(true);
                openBossDoors();
            }
        }
    }
}

Room* Level::currentRoom() const {
    return m_roomManager->currentRoom();
}

bool Level::isAllRoomsCompleted() const {
    // 委托给canOpenBossDoor，该方法检查所有非Boss房间是否都已访问且敌人清空
    return canOpenBossDoor();
}

void Level::onEnemyDying(Enemy* enemy) {
    if (!enemy) {
        qWarning() << "onEnemyDying: enemy 为 null";
        return;
    }

    // 检查是否是Boss死亡
    bool isBoss = dynamic_cast<Boss*>(enemy) != nullptr;

    // 检查是否是精英房间中的yanglin被击败
    if (m_isEliteRoom && !m_elitePhase2Triggered) {
        YanglinEnemy* yanglin = dynamic_cast<YanglinEnemy*>(enemy);
        if (yanglin) {
            m_eliteYanglinDeathCount++;
            qDebug() << "精英房间yanglin被击败，当前被击败数:" << m_eliteYanglinDeathCount;

            // 第一个yanglin被击败时触发第二阶段
            if (m_eliteYanglinDeathCount == 1) {
                qDebug() << "触发精英房间第二阶段";
                // 延迟一小段时间再触发第二阶段对话
                QTimer::singleShot(1000, this, &Level::checkEliteRoomPhase2);
            }
        }
    }

    // 使用 QPointer 包装目标并用 removeAll 安全移除
    QPointer<Enemy> target(enemy);
    currentEnemies().removeAll(target);
    // 只有非召唤的敌人才有概率掉落物品
    // 使用工厂类判断是否掉落（5%概率）
    if (!enemy->isSummoned()) {
        QPointF dropPos = enemy->pos();
        QPointer<Level> levelPtr(this);
        QPointer<Player> playerPtr(m_player);
        QGraphicsScene* scenePtr = m_scene;
        QTimer::singleShot(100, this, [levelPtr, playerPtr, dropPos, scenePtr]() {
            if (levelPtr && playerPtr && scenePtr) {
                if (DroppedItemFactory::shouldEnemyDropItem()) {
                    DroppedItemFactory::dropRandomItem(ItemDropPool::ENEMY_DROP, dropPos, playerPtr, scenePtr);
                }
            }
        });
    }

    // 如果死亡的是NightmareBoss，切换到map1背景（3秒渐变）
    if (dynamic_cast<NightmareBoss*>(enemy)) {
        // 梦魇完全死亡，背景变为map1
        fadeBackgroundTo(QString("assets/background/startRoom.png"), 3000);
    }

    // 如果死亡的是WashMachineBoss，恢复到原始背景（3秒渐变）
    if (dynamic_cast<WashMachineBoss*>(enemy)) {
        // 洗衣机Boss完全死亡，背景恢复为map2
        fadeBackgroundTo(QString("assets/background/map2.png"), 3000);
    }

    for (Room* r : rooms()) {
        if (!r)
            continue;
        r->currentEnemies.removeAll(target);
    }

    Room* cur = rooms()[currentRoomIndex()];
    // 如果是Boss死亡，无论房间是否清空，都标记Boss已被击败
    if (isBoss && !m_bossDefeated) {
        qDebug() << "[Level] Boss被击败，标记m_bossDefeated = true，当前房间剩余敌人:"
                 << (cur ? cur->currentEnemies.size() : 0);
        m_bossDefeated = true;
    }

    if (cur && cur->currentEnemies.isEmpty()) {
        // 检查是否是Boss房间
        LevelConfig config;
        if (config.loadFromFile(m_levelNumber)) {
            const RoomConfig& roomCfg = config.getRoom(currentRoomIndex());

            // 如果是Boss房间
            if (roomCfg.hasBoss) {
                // Boss刚死亡，记录标志并等待小怪清完
                if (isBoss) {
                    qDebug() << "[Level] Boss被击败，房间已清空，启动奖励流程";
                    m_bossDefeated = true;
                    // 延迟启动奖励流程，等待Boss死亡动画完成
                    QTimer::singleShot(1500, this, &Level::startBossRewardSequence);
                    return;  // 不执行正常的开门逻辑
                }
                // 小怪死亡但Boss之前已被击败，现在房间清空，启动奖励流程
                else if (m_bossDefeated && m_rewardSystem && !m_rewardSystem->isRewardSequenceActive()) {
                    qDebug() << "[Level] Boss房间小怪已清空，Boss之前已被击败，启动奖励流程";
                    QTimer::singleShot(500, this, &Level::startBossRewardSequence);
                    return;  // 不执行正常的开门逻辑
                }
            }
        }

        // 非Boss房间或奖励流程已在进行中，正常开门
        if (!m_rewardSystem || !m_rewardSystem->isRewardSequenceActive()) {
            openDoors(cur);
        }

        // 在战斗房间清空敌人后，检查是否满足打开boss门的条件
        if (config.loadFromFile(m_levelNumber)) {
            const RoomConfig& roomCfg = config.getRoom(currentRoomIndex());
            if (!roomCfg.hasBoss && hasEncounteredBossDoor() && !bossDoorsAlreadyOpened()) {
                if (canOpenBossDoor()) {
                    qDebug() << "战斗房间清空后检测到满足boss门开启条件，立即打开boss门";
                    setBossDoorsAlreadyOpened(true);
                    openBossDoors();
                }
            }
        }
    }
}

void Level::openDoors(Room* cur) {
    LevelConfig config;
    bool up = false, down = false, left = false, right = false;
    bool anyDoorOpened = false;  // 追踪是否有门被打开

    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig& roomCfg = config.getRoom(currentRoomIndex());

        int doorIndex = 0;

        if (roomCfg.doorUp >= 0) {
            int neighborRoom = roomCfg.doorUp;
            const RoomConfig& neighborCfg = config.getRoom(neighborRoom);

            // 最高优先级：如果邻居是boss房间，必须所有非boss房间都已访问才能开门
            if (neighborCfg.hasBoss && !canOpenBossDoor()) {
                qDebug() << "上门通往boss房间" << neighborRoom << "，但还有房间未探索，保持关闭";
                doorIndex++;
            } else {
                cur->setDoorOpenUp(true);
                up = true;
                anyDoorOpened = true;
                // 播放开门动画
                if (doorIndex < currentDoors().size()) {
                    currentDoors()[doorIndex]->open();
                }
                doorIndex++;
                qDebug() << "打开上门，通往房间" << neighborRoom;

                // 设置邻房间的对应门为打开状态（除非邻居是未访问的战斗房间）
                if (neighborRoom >= 0 && neighborRoom < rooms().size()) {
                    Room* neighbor = rooms()[neighborRoom];
                    bool shouldOpenNeighborDoor = !neighbor->isBattleRoom() || visitedRooms()[neighborRoom];
                    if (shouldOpenNeighborDoor) {
                        neighbor->setDoorOpenDown(true);
                    }
                }
            }
        }
        if (roomCfg.doorDown >= 0) {
            int neighborRoom = roomCfg.doorDown;
            const RoomConfig& neighborCfg = config.getRoom(neighborRoom);

            // 最高优先级：如果邻居是boss房间，必须所有非boss房间都已访问才能开门
            if (neighborCfg.hasBoss && !canOpenBossDoor()) {
                qDebug() << "下门通往boss房间" << neighborRoom << "，但还有房间未探索，保持关闭";
                doorIndex++;
            } else {
                cur->setDoorOpenDown(true);
                down = true;
                anyDoorOpened = true;
                if (doorIndex < currentDoors().size()) {
                    currentDoors()[doorIndex]->open();
                }
                doorIndex++;
                qDebug() << "打开下门，通往房间" << neighborRoom;

                if (neighborRoom >= 0 && neighborRoom < rooms().size()) {
                    Room* neighbor = rooms()[neighborRoom];
                    bool shouldOpenNeighborDoor = !neighbor->isBattleRoom() || visitedRooms()[neighborRoom];
                    if (shouldOpenNeighborDoor) {
                        neighbor->setDoorOpenUp(true);
                    }
                }
            }
        }
        if (roomCfg.doorLeft >= 0) {
            int neighborRoom = roomCfg.doorLeft;
            const RoomConfig& neighborCfg = config.getRoom(neighborRoom);

            // 最高优先级：如果邻居是boss房间，必须所有非boss房间都已访问才能开门
            if (neighborCfg.hasBoss && !canOpenBossDoor()) {
                qDebug() << "左门通往boss房间" << neighborRoom << "，但还有房间未探索，保持关闭";
                doorIndex++;
            } else {
                cur->setDoorOpenLeft(true);
                left = true;
                anyDoorOpened = true;
                if (doorIndex < currentDoors().size()) {
                    currentDoors()[doorIndex]->open();
                }
                doorIndex++;
                qDebug() << "打开左门，通往房间" << neighborRoom;

                if (neighborRoom >= 0 && neighborRoom < rooms().size()) {
                    Room* neighbor = rooms()[neighborRoom];
                    bool shouldOpenNeighborDoor = !neighbor->isBattleRoom() || visitedRooms()[neighborRoom];
                    if (shouldOpenNeighborDoor) {
                        neighbor->setDoorOpenRight(true);
                    }
                }
            }
        }
        if (roomCfg.doorRight >= 0) {
            int neighborRoom = roomCfg.doorRight;
            const RoomConfig& neighborCfg = config.getRoom(neighborRoom);

            // 最高优先级：如果邻居是boss房间，必须所有非boss房间都已访问才能开门
            if (neighborCfg.hasBoss && !canOpenBossDoor()) {
                qDebug() << "右门通往boss房间" << neighborRoom << "，但还有房间未探索，保持关闭";
                doorIndex++;
            } else {
                cur->setDoorOpenRight(true);
                right = true;
                anyDoorOpened = true;
                if (doorIndex < currentDoors().size()) {
                    currentDoors()[doorIndex]->open();
                }
                doorIndex++;
                qDebug() << "打开右门，通往房间" << neighborRoom;

                if (neighborRoom >= 0 && neighborRoom < rooms().size()) {
                    Room* neighbor = rooms()[neighborRoom];
                    bool shouldOpenNeighborDoor = !neighbor->isBattleRoom() || visitedRooms()[neighborRoom];
                    if (shouldOpenNeighborDoor) {
                        neighbor->setDoorOpenLeft(true);
                    }
                }
            }
        }
    }

    // 只有在实际打开了门时才播放音效和记录日志
    if (anyDoorOpened) {
        AudioManager::instance().playSound("door_open");
        qDebug() << "房间" << currentRoomIndex() << "敌人全部清空，门已打开";
    } else {
        qDebug() << "房间" << currentRoomIndex() << "敌人已清空，但所有门都通往boss房间且条件不满足";
    }

    // 只有战斗房间才发射敌人清空信号
    if (cur && cur->isBattleRoom()) {
        emit enemiesCleared(currentRoomIndex(), up, down, left, right);
    }
}

bool Level::canOpenBossDoor() const {
    // 委托给RoomManager
    return m_roomManager->canOpenBossDoor();
}

void Level::openBossDoors() {
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber))
        return;

    qDebug() << "=== 开始打开所有通往boss房间的门 ===";

    bool shouldReloadCurrentRoom = false;

    // 遍历所有房间，找到所有通往boss房间的门并打开
    for (int roomIndex = 0; roomIndex < config.getRoomCount(); ++roomIndex) {
        const RoomConfig& roomCfg = config.getRoom(roomIndex);

        // 跳过boss房间本身
        if (roomCfg.hasBoss)
            continue;
        if (roomIndex >= rooms().size() || !rooms()[roomIndex])
            continue;
        Room* room = rooms()[roomIndex];
        qDebug() << "检查房间" << roomIndex << "的门（上:" << roomCfg.doorUp << "，下:" << roomCfg.doorDown
                 << "，左:" << roomCfg.doorLeft << "，右:" << roomCfg.doorRight << "）";

        // 检查当前房间的相邻房间是否有通往boss的门
        bool isAdjacentToBoss = false;
        if ((roomCfg.doorUp >= 0 && config.getRoom(roomCfg.doorUp).hasBoss) ||
            (roomCfg.doorDown >= 0 && config.getRoom(roomCfg.doorDown).hasBoss) ||
            (roomCfg.doorLeft >= 0 && config.getRoom(roomCfg.doorLeft).hasBoss) ||
            (roomCfg.doorRight >= 0 && config.getRoom(roomCfg.doorRight).hasBoss)) {
            isAdjacentToBoss = true;
        }

        // 如果当前房间邻接boss房间，且是正在显示的房间，标记需要重新加载
        if (isAdjacentToBoss && roomIndex == currentRoomIndex()) {
            shouldReloadCurrentRoom = true;
        }

        // 检查该房间的每个门是否通往boss房间
        if (roomCfg.doorUp >= 0) {
            const RoomConfig& neighborCfg = config.getRoom(roomCfg.doorUp);
            if (neighborCfg.hasBoss && !room->isDoorOpenUp()) {
                qDebug() << "开启房间" << roomIndex << "通往boss房间" << roomCfg.doorUp << "的上门";
                room->setDoorOpenUp(true);

                // 找到并播放该门的开门动画
                // 如果是当前房间，使用 currentDoors()
                if (roomIndex == currentRoomIndex() && !currentDoors().isEmpty()) {
                    for (Door* door : currentDoors()) {
                        if (door && door->direction() == Door::Up) {
                            qDebug() << "  -> 在当前房间的 currentDoors() 中找到上门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                } else if (roomDoors().contains(roomIndex)) {
                    const QVector<Door*>& doors = roomDoors()[roomIndex];
                    for (Door* door : doors) {
                        if (door && door->direction() == Door::Up) {
                            qDebug() << "  -> 在 roomDoors() 中找到房间" << roomIndex << "的上门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
            }
        }

        if (roomCfg.doorDown >= 0) {
            const RoomConfig& neighborCfg = config.getRoom(roomCfg.doorDown);
            if (neighborCfg.hasBoss && !room->isDoorOpenDown()) {
                qDebug() << "开启房间" << roomIndex << "通往boss房间" << roomCfg.doorDown << "的下门";
                room->setDoorOpenDown(true);

                if (roomIndex == currentRoomIndex() && !currentDoors().isEmpty()) {
                    for (Door* door : currentDoors()) {
                        if (door && door->direction() == Door::Down) {
                            qDebug() << "  -> 在当前房间的 currentDoors() 中找到下门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                } else if (roomDoors().contains(roomIndex)) {
                    const QVector<Door*>& doors = roomDoors()[roomIndex];
                    for (Door* door : doors) {
                        if (door && door->direction() == Door::Down) {
                            qDebug() << "  -> 在 roomDoors() 中找到房间" << roomIndex << "的下门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
            }
        }

        if (roomCfg.doorLeft >= 0) {
            const RoomConfig& neighborCfg = config.getRoom(roomCfg.doorLeft);
            if (neighborCfg.hasBoss && !room->isDoorOpenLeft()) {
                qDebug() << "开启房间" << roomIndex << "通往boss房间" << roomCfg.doorLeft << "的左门";
                room->setDoorOpenLeft(true);

                if (roomIndex == currentRoomIndex() && !currentDoors().isEmpty()) {
                    for (Door* door : currentDoors()) {
                        if (door && door->direction() == Door::Left) {
                            qDebug() << "  -> 在当前房间的 currentDoors() 中找到左门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                } else if (roomDoors().contains(roomIndex)) {
                    const QVector<Door*>& doors = roomDoors()[roomIndex];
                    for (Door* door : doors) {
                        if (door && door->direction() == Door::Left) {
                            qDebug() << "  -> 在 roomDoors() 中找到房间" << roomIndex << "的左门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
            }
        }

        if (roomCfg.doorRight >= 0) {
            const RoomConfig& neighborCfg = config.getRoom(roomCfg.doorRight);
            if (neighborCfg.hasBoss && !room->isDoorOpenRight()) {
                qDebug() << "开启房间" << roomIndex << "通往boss房间" << roomCfg.doorRight << "的右门";
                room->setDoorOpenRight(true);

                if (roomIndex == currentRoomIndex() && !currentDoors().isEmpty()) {
                    for (Door* door : currentDoors()) {
                        if (door && door->direction() == Door::Right) {
                            qDebug() << "  -> 在当前房间的 currentDoors() 中找到右门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                } else if (roomDoors().contains(roomIndex)) {
                    const QVector<Door*>& doors = roomDoors()[roomIndex];
                    for (Door* door : doors) {
                        if (door && door->direction() == Door::Right) {
                            qDebug() << "  -> 在 roomDoors() 中找到房间" << roomIndex << "的右门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
            }
        }
    }

    // 发射boss门开启信号
    emit bossDoorsOpened();

    // 如果当前房间邻接boss房间，需要重新加载门以显示打开状态
    if (shouldReloadCurrentRoom) {
        qDebug() << "当前房间" << currentRoomIndex() << "邻接boss房间，重新加载门以显示打开状态";

        // 清除当前所有门
        for (Door* door : currentDoors()) {
            if (m_scene && door) {
                m_scene->removeItem(door);
            }
            delete door;
        }
        currentDoors().clear();

        // 从缓存中移除
        roomDoors().remove(currentRoomIndex());

        // 重新生成门（会读取房间状态，boss门已经被设置为打开）
        const RoomConfig& currentRoomCfg = config.getRoom(currentRoomIndex());
        spawnDoors(currentRoomCfg);

        qDebug() << "已重新加载当前房间的所有门";
    }
}

void Level::onPlayerDied() {
    qDebug() << "Level::onPlayerDied - 通知所有敌人移除玩家引用";

    // 先收集当前全局敌人的原始指针，并断开其 dying 信号，避免在调用 setPlayer 时触发 onEnemyDying 导致容器被修改
    QVector<Enemy*> rawEnemies;
    for (QPointer<Enemy> ePtr : currentEnemies()) {
        Enemy* e = ePtr.data();
        if (e) {
            QObject::disconnect(e, &Enemy::dying, this, &Level::onEnemyDying);
            rawEnemies.append(e);
        }
    }
    for (Enemy* e : rawEnemies) {
        if (e)
            e->setPlayer(nullptr);
    }

    // 对每个房间同样处理：先收集原始指针并断开信号，再调用 setPlayer(nullptr)
    for (Room* r : rooms()) {
        if (!r)
            continue;
        QVector<Enemy*> roomRaw;
        for (QPointer<Enemy> ePtr : r->currentEnemies) {
            Enemy* e = ePtr.data();
            if (e) {
                QObject::disconnect(e, &Enemy::dying, this, &Level::onEnemyDying);
                roomRaw.append(e);
            }
        }
        for (Enemy* e : roomRaw) {
            if (e)
                e->setPlayer(nullptr);
        }
    }
}

// 在指定位置掉落随机物品（委托给RoomManager）
void Level::dropRandomItem(QPointF position) {
    m_roomManager->dropRandomItem(position);
}

// 从指定位置掉落多个物品（委托给RoomManager）
void Level::dropItemsFromPosition(QPointF position, int count, bool scatter) {
    m_roomManager->dropItemsFromPosition(position, count, scatter);
}

// Boss召唤敌人方法（委托给RoomManager）
void Level::spawnEnemiesForBoss(const QVector<QPair<QString, int>>& enemies) {
    m_roomManager->spawnEnemiesForBoss(enemies);
}

void Level::setPaused(bool paused) {
    m_isPaused = paused;

    // 暂停/恢复所有当前敌人
    for (QPointer<Enemy>& enemyPtr : currentEnemies()) {
        if (Enemy* enemy = enemyPtr.data()) {
            if (paused) {
                enemy->pauseTimers();
            } else {
                enemy->resumeTimers();
            }
        }
    }

    // 暂停/恢复所有子弹
    if (m_scene) {
        QList<QGraphicsItem*> items = m_scene->items();
        for (QGraphicsItem* item : items) {
            if (Projectile* projectile = dynamic_cast<Projectile*>(item)) {
                projectile->setPaused(paused);
            }
        }
    }

    qDebug() << "Level暂停状态:" << (paused ? "已暂停" : "已恢复");
}

// ==================== 辅助方法 ====================

void Level::pauseAllEnemyTimers() {
    for (const QPointer<Enemy>& enemyPtr : currentEnemies()) {
        if (enemyPtr) {
            enemyPtr->pauseTimers();
        }
    }
}

void Level::resumeAllEnemyTimers() {
    for (const QPointer<Enemy>& enemyPtr : currentEnemies()) {
        if (enemyPtr) {
            enemyPtr->resumeTimers();
        }
    }
}

// ==================== Boss通用槽函数 ====================

void Level::onBossRequestDialog(const QStringList& dialogs, const QString& background) {
    qDebug() << "[Level] Boss请求显示对话";

    // 暂停所有敌人的定时器
    pauseAllEnemyTimers();

    // 显示对话（使用boss对话模式）
    showStoryDialog(dialogs, true, background);
}

void Level::onBossRequestAbsorb() {
    qDebug() << "[Level] Boss请求执行吸纳动画";

    if (m_currentWashMachineBoss) {
        performAbsorbAnimation(m_currentWashMachineBoss.data());
    }
}

void Level::onBossRequestFadeDialogBackground(const QString& backgroundPath, int duration) {
    qDebug() << "[Level] Boss请求渐变对话背景:" << backgroundPath << "持续" << duration << "ms";
    // 委托给 DialogSystem
    if (m_dialogSystem) {
        m_dialogSystem->setPendingFadeDialogBackground(backgroundPath, duration);
    }
}

void Level::onBossRequestDialogBackgroundChange(int dialogIndex, const QString& backgroundName) {
    qDebug() << "[Level] Boss请求在对话索引" << dialogIndex << "时切换背景到:" << backgroundName;
    // 委托给 DialogSystem
    if (m_dialogSystem) {
        m_dialogSystem->setDialogBackgroundChange(dialogIndex, backgroundName);
    }
}

void Level::onBossEnemySpawned(Enemy* enemy) {
    if (enemy) {
        QPointer<Enemy> eptr(enemy);
        currentEnemies().append(eptr);
        if (currentRoomIndex() >= 0 && currentRoomIndex() < rooms().size()) {
            rooms()[currentRoomIndex()]->currentEnemies.append(eptr);
        }
        connect(enemy, &Enemy::dying, this, &Level::onEnemyDying);
        qDebug() << "[Level] Boss召唤的敌人已追踪";
    }
}

void Level::changeBackground(const QString& backgroundPath) {
    if (!m_scene || !m_backgroundItem)
        return;

    QPixmap bg(backgroundPath);
    if (!bg.isNull()) {
        m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        qDebug() << "背景已切换为:" << backgroundPath;
    } else {
        qWarning() << "无法加载背景图片:" << backgroundPath;
    }
}

void Level::performAbsorbAnimation(WashMachineBoss* boss) {
    if (!boss || !m_scene || !m_player)
        return;

    m_isAbsorbAnimationActive = true;

    // 计算Boss中心作为吸纳目标点
    m_absorbCenter = boss->pos() + QPointF(boss->pixmap().width() / 2.0, boss->pixmap().height() / 2.0);

    // 清理场景中所有子弹（避免吸纳期间伤害）
    if (m_scene) {
        QList<QGraphicsItem*> allItems = m_scene->items();
        for (QGraphicsItem* item : allItems) {
            if (Projectile* proj = dynamic_cast<Projectile*>(item)) {
                proj->destroy();
            }
        }
    }

    // 收集所有需要被吸纳的实体（除了Boss本身）
    m_absorbingItems.clear();
    m_absorbStartPositions.clear();
    m_absorbAngles.clear();

    // 添加玩家
    m_absorbingItems.append(m_player);
    m_absorbStartPositions.append(m_player->pos());
    m_absorbAngles.append(0);

    // 禁用玩家移动和攻击，设置持久无敌
    m_player->setCanMove(false);
    m_player->setCanShoot(false);
    m_player->setPermanentInvincible(true);  // 吸纳时玩家持久无敌
    qDebug() << "[吸纳] 玩家已设置为无敌状态";

    // 添加所有敌人（除了Boss）
    for (QPointer<Enemy>& enemyPtr : currentEnemies()) {
        if (Enemy* enemy = enemyPtr.data()) {
            if (enemy != boss) {
                enemy->pauseTimers();
                m_absorbingItems.append(enemy);
                m_absorbStartPositions.append(enemy->pos());
                m_absorbAngles.append(0);
            }
        }
    }

    // Boss也停止攻击（通过 m_isAbsorbing 标志，已在 enterPhase3 设置）

    // 为每个实体设置初始螺旋角度
    for (int i = 0; i < m_absorbingItems.size(); ++i) {
        QPointF itemPos = m_absorbStartPositions[i];
        double dx = itemPos.x() - m_absorbCenter.x();
        double dy = itemPos.y() - m_absorbCenter.y();
        m_absorbAngles[i] = qAtan2(dy, dx);
    }

    m_absorbAnimationStep = 0;

    // 创建吸纳动画定时器
    if (!m_absorbAnimationTimer) {
        m_absorbAnimationTimer = new QTimer(this);
        connect(m_absorbAnimationTimer, &QTimer::timeout, this, &Level::onAbsorbAnimationStep);
    }
    m_absorbAnimationTimer->start(16);  // 约60fps

    qDebug() << "吸纳动画开始，共" << m_absorbingItems.size() << "个实体";
}

void Level::onAbsorbAnimationStep() {
    if (!m_isAbsorbAnimationActive) {
        m_absorbAnimationTimer->stop();
        return;
    }

    m_absorbAnimationStep++;

    const int totalSteps = 120;  // 约2秒完成动画
    double progress = static_cast<double>(m_absorbAnimationStep) / totalSteps;

    if (progress >= 1.0) {
        // 动画完成
        m_absorbAnimationTimer->stop();
        m_isAbsorbAnimationActive = false;

        // 清理场景中所有子弹（避免对话期间伤害）
        if (m_scene) {
            QList<QGraphicsItem*> allItems = m_scene->items();
            for (QGraphicsItem* item : allItems) {
                if (Projectile* proj = dynamic_cast<Projectile*>(item)) {
                    proj->destroy();
                }
            }
        }

        // 隐藏所有被吸纳的实体（不删除，等待对话后释放）
        for (int i = 0; i < m_absorbingItems.size(); ++i) {
            QGraphicsItem* item = m_absorbingItems[i];
            if (!item)
                continue;

            // 隐藏实体
            item->setVisible(false);

            // 如果是敌人，确保其定时器停止
            if (item != m_player) {
                Enemy* enemy = dynamic_cast<Enemy*>(item);
                if (enemy) {
                    enemy->pauseTimers();
                }
            }
        }

        // 保存吸纳的实体列表供对话后释放使用
        // m_absorbingItems 保持不清空

        qDebug() << "吸纳动画完成，通知Boss";

        // 通知Boss吸纳完成
        if (m_currentWashMachineBoss) {
            m_currentWashMachineBoss->onAbsorbComplete();
        }

        return;
    }

    // 使用缓动函数让动画更自然
    double easedProgress = 1 - qPow(1 - progress, 3);  // easeOutCubic

    // 更新每个实体的位置（螺旋移动）
    for (int i = 0; i < m_absorbingItems.size(); ++i) {
        QGraphicsItem* item = m_absorbingItems[i];
        if (!item)
            continue;

        QPointF startPos = m_absorbStartPositions[i];
        double startAngle = m_absorbAngles[i];

        // 计算当前距离中心的距离（逐渐减小）
        double startDist = qSqrt(qPow(startPos.x() - m_absorbCenter.x(), 2) +
                                 qPow(startPos.y() - m_absorbCenter.y(), 2));
        double currentDist = startDist * (1 - easedProgress);

        // 计算当前角度（螺旋增加）
        double currentAngle = startAngle + easedProgress * 6 * M_PI;  // 旋转3圈

        // 计算新位置
        double newX = m_absorbCenter.x() + currentDist * qCos(currentAngle);
        double newY = m_absorbCenter.y() + currentDist * qSin(currentAngle);

        item->setPos(newX, newY);

        // 缩小实体（可选视觉效果）
        if (item != m_player) {
            double scale = 1.0 - easedProgress * 0.8;
            item->setScale(scale);
        }
    }
}

// ==================== Boss奖励机制（委托给RewardSystem） ====================

void Level::startBossRewardSequence() {
    if (!m_rewardSystem || m_rewardSystem->isRewardSequenceActive())
        return;

    qDebug() << "[Level] 委托RewardSystem开始Boss奖励流程";

    // 获取当前房间的奖励配置
    QStringList usagiChestItems;
    LevelConfig config;
    if (config.loadFromFile(m_levelNumber)) {
        const RoomConfig& roomCfg = config.getRoom(currentRoomIndex());
        usagiChestItems = roomCfg.usagiChestItems;
    }

    // 委托给RewardSystem处理
    m_rewardSystem->startBossRewardSequence(m_levelNumber, usagiChestItems);
}

void Level::onRewardShowDialogRequested(const QStringList& dialog) {
    qDebug() << "[Level] RewardSystem请求显示对话";

    // 显示对话（使用透明背景，保持游戏画面可见）
    showStoryDialog(dialog, false, "transparent");

    // 对话结束后通知RewardSystem
    disconnect(this, &Level::storyFinished, nullptr, nullptr);
    connect(this, &Level::storyFinished, this, [this]() {
        if (m_rewardSystem) {
            m_rewardSystem->onDialogFinished();
        } }, Qt::SingleShotConnection);
}

void Level::onRewardSequenceCompleted() {
    qDebug() << "[Level] RewardSystem奖励流程完成";
    // RewardSystem已经处理了G键激活逻辑
    // Level只需要在这里做额外的清理或通知（如果需要的话）
}

// ========== 精英房间相关函数 ==========

void Level::checkEliteRoomPhase2() {
    if (!m_isEliteRoom || m_elitePhase2Triggered)
        return;

    qDebug() << "准备触发精英房间第二阶段";
    m_elitePhase2Triggered = true;

    // 暂停所有敌人的定时器
    pauseAllEnemyTimers();

    // 获取第二阶段对话配置
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber)) {
        qWarning() << "无法加载关卡配置";
        startElitePhase2();
        return;
    }

    const RoomConfig& roomCfg = config.getRoom(currentRoomIndex());

    if (roomCfg.elitePhase2Dialog.isEmpty()) {
        qDebug() << "没有第二阶段对话，直接开始第二阶段";
        startElitePhase2();
        return;
    }

    // 显示第二阶段对话
    m_isEliteDialog = true;
    showStoryDialog(roomCfg.elitePhase2Dialog, false, roomCfg.elitePhase2DialogBackground);
}

void Level::startElitePhase2() {
    qDebug() << "开始精英房间第二阶段";

    // 生成祝昊
    spawnZhuhaoEnemy();

    // 恢复所有敌人的定时器
    resumeAllEnemyTimers();
}

void Level::spawnZhuhaoEnemy() {
    qDebug() << "生成祝昊敌人";

    if (!m_scene || !m_player) {
        qWarning() << "无法生成祝昊：场景或玩家为空";
        return;
    }

    // 加载祝昊图片
    QPixmap zhuhaoPic("assets/enemy/level_3/zhuhao.png");
    if (zhuhaoPic.isNull()) {
        qWarning() << "无法加载祝昊图片";
        return;
    }

    // 创建祝昊敌人 - 使用较小的缩放比例避免卡在地图外
    ZhuhaoEnemy* zhuhao = new ZhuhaoEnemy(zhuhaoPic, 0.08);
    zhuhao->setPlayer(m_player);

    // 添加到场景
    m_scene->addItem(zhuhao);

    // 初始化在随机边缘位置
    zhuhao->initializeAtRandomEdge();

    // 添加到敌人列表
    QPointer<Enemy> zhuhaoPtr(zhuhao);
    currentEnemies().append(zhuhaoPtr);

    // 添加到当前房间的敌人列表
    if (currentRoomIndex() >= 0 && currentRoomIndex() < rooms().size()) {
        Room* currentRoom = rooms()[currentRoomIndex()];
        if (currentRoom) {
            currentRoom->currentEnemies.append(zhuhaoPtr);
        }
    }

    // 连接死亡信号
    connect(zhuhao, &Enemy::dying, this, &Level::onEnemyDying);

    // 保存引用
    m_zhuhaoEnemy = zhuhao;

    qDebug() << "祝昊已生成，位置:" << zhuhao->pos();
}

// ==================== G键进入下一关相关方法 ====================

void Level::showGKeyHint() {
    if (!m_scene)
        return;

    // 如果已存在提示，先移除
    hideGKeyHint();

    // 创建提示文字
    m_gKeyHintText = new QGraphicsTextItem("按 G 键进入下一关");
    m_gKeyHintText->setDefaultTextColor(QColor(0, 0, 0));  // 黑色
    m_gKeyHintText->setFont(QFont("Microsoft YaHei", 20, QFont::Bold));

    // 居中显示在屏幕下方
    QRectF textRect = m_gKeyHintText->boundingRect();
    m_gKeyHintText->setPos((800 - textRect.width()) / 2, 500);
    m_gKeyHintText->setZValue(9000);  // 确保在大部分元素之上
    m_scene->addItem(m_gKeyHintText);

    qDebug() << "[Level] 显示G键提示";
}

void Level::hideGKeyHint() {
    if (m_gKeyHintText) {
        if (m_scene) {
            m_scene->removeItem(m_gKeyHintText);
        }
        delete m_gKeyHintText;
        m_gKeyHintText = nullptr;
        qDebug() << "[Level] 隐藏G键提示";
    }
}

bool Level::isGKeyEnabled() const {
    return m_rewardSystem ? m_rewardSystem->isGKeyEnabled() : false;
}

void Level::triggerNextLevelByGKey() {
    if (!isGKeyEnabled()) {
        qDebug() << "[Level] G键未激活，忽略";
        return;
    }

    qDebug() << "[Level] G键触发进入下一关";

    // 禁用G键
    if (m_rewardSystem) {
        m_rewardSystem->disableGKey();
        m_rewardSystem->setBossRoomCleared(false);
    }

    // 隐藏提示
    hideGKeyHint();

    // 发送关卡完成信号
    emit levelCompleted(m_levelNumber);
}