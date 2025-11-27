#include "level.h"
#include <QDebug>
#include <QFile>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QPointer>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QVariantAnimation>
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
#include "../entities/boss.h"
#include "../entities/clockboom.h"
#include "../entities/clockenemy.h"
#include "../entities/digitalsystemenemy.h"
#include "../entities/enemy.h"
#include "../entities/nightmareboss.h"
#include "../entities/optimizationenemy.h"
#include "../entities/pantsenemy.h"
#include "../entities/yanglinenemy.h"
#include "../entities/pillowenemy.h"
#include "../entities/player.h"
#include "../entities/projectile.h"
#include "../entities/sockenemy.h"
#include "../entities/sockshooter.h"
#include "../entities/teacherboss.h"
#include "../entities/usagi.h"
#include "../entities/walker.h"
#include "../entities/washmachineboss.h"
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
      m_hasEncounteredBossDoor(false),
      m_bossDoorsAlreadyOpened(false),
      m_dialogBox(nullptr),
      m_dialogText(nullptr),
      m_continueHint(nullptr),
      m_currentDialogIndex(0),
      m_isStoryFinished(false),
      m_isBossDialog(false)
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
    m_hasEncounteredBossDoor = false;
    m_bossDoorsAlreadyOpened = false;

    LevelConfig config;
    if (!config.loadFromFile(levelNumber))
    {
        qWarning() << "加载关卡配置失败，使用默认配置";
        return;
    }

    qDebug() << "加载关卡:" << config.getLevelName();
    qDebug() << "关卡描述条数:" << config.getDescription().size();

    // 开发者模式：非可视化模拟整个流程，直接显示boss对话
    if (m_skipToBoss)
    {
        qDebug() << "开发者模式: 非可视化模拟流程，直接进入Boss对话";
        // 直接初始化关卡（跳过开头对话的显示，但内部状态正确初始化）
        initializeLevelAfterStory(config);
        // 延迟发出storyFinished信号，确保GameView的连接已建立
        QTimer::singleShot(0, this, [this]()
                           { emit storyFinished(); });
        return;
    }

    // 正常流程：如果有剧情描述，显示剧情；否则直接初始化关卡
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

    bool isDevMode = m_skipToBoss; // 保存开发者模式状态

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
            qDebug() << "房间" << i << "标记为战斗房间（敌人数:" << roomCfg.enemyCount << "，Boss:" << roomCfg.hasBoss
                     << "）";
        }

        m_rooms.append(room);
    }

    visited.resize(config.getRoomCount());
    visited.fill(false);

    int startRoomIndex = config.getStartRoomIndex();
    int bossRoomIndex = -1;

    // 开发者模式：模拟遍历所有房间，直接进入boss房
    if (isDevMode)
    {
        // 找到Boss房索引
        for (int i = 0; i < config.getRoomCount(); ++i)
        {
            if (config.getRoom(i).hasBoss)
            {
                bossRoomIndex = i;
                break;
            }
        }

        if (bossRoomIndex >= 0)
        {
            // 标记所有非boss房间为已访问且已清除
            for (int i = 0; i < config.getRoomCount(); ++i)
            {
                if (!config.getRoom(i).hasBoss)
                {
                    visited[i] = true;
                    if (m_rooms[i])
                    {
                        m_rooms[i]->setCleared(true);
                    }
                }
            }
            visited_count = config.getRoomCount() - 1;

            // 设置boss门状态
            m_hasEncounteredBossDoor = true;
            m_bossDoorsAlreadyOpened = true;
            m_isStoryFinished = true;

            // 直接进入boss房
            m_currentRoomIndex = bossRoomIndex;
            qDebug() << "开发者模式: 模拟完成，进入Boss房" << bossRoomIndex;
        }
        else
        {
            // 没找到boss房，使用正常流程
            visited[startRoomIndex] = true;
            visited_count = 1;
            m_currentRoomIndex = startRoomIndex;
        }

        m_skipToBoss = false; // 重置标志
    }
    else
    {
        // 正常流程：从起始房间开始
        visited[startRoomIndex] = true;
        visited_count = 1;
        m_currentRoomIndex = startRoomIndex;
    }

    const RoomConfig &currentRoomCfg = config.getRoom(m_currentRoomIndex);

    // 创建背景图片项
    if (!m_backgroundItem)
    {
        m_backgroundItem = new QGraphicsPixmapItem();
        m_backgroundItem->setZValue(-1000); // 设置最低优先级
        m_scene->addItem(m_backgroundItem);
    }

    try
    {
        QString bgPath = ConfigManager::instance().getAssetPath(currentRoomCfg.backgroundImage);
        QPixmap bg = ResourceFactory::loadImage(bgPath);
        m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        m_backgroundItem->setPos(0, 0);
        // 保存当前与原始背景路径（相对assets/）
        m_currentBackgroundPath = currentRoomCfg.backgroundImage;
        m_originalBackgroundPath = m_currentBackgroundPath;
    }
    catch (const QString &e)
    {
        qWarning() << "加载房间背景失败:" << e;
    }

    // 开发者模式：显示boss对话，对话结束后初始化boss房
    if (isDevMode && bossRoomIndex >= 0 && !currentRoomCfg.bossDialog.isEmpty())
    {
        visited[bossRoomIndex] = true;
        visited_count++;
        showStoryDialog(currentRoomCfg.bossDialog, true, currentRoomCfg.bossDialogBackground);
    }
    else
    {
        // 正常初始化当前房间
        initCurrentRoom(m_rooms[m_currentRoomIndex]);
    }

    checkChange = new QTimer(this);
    connect(checkChange, &QTimer::timeout, this, &Level::enterNextRoom);
    checkChange->start(100);

    buildMinimapData();
}

// 事件过滤器：处理对话框的点击和键盘事件
bool Level::eventFilter(QObject *watched, QEvent *event)
{
    // 如果游戏暂停，不拦截任何事件，让暂停菜单处理
    if (m_isPaused)
    {
        return QObject::eventFilter(watched, event);
    }

    if (watched == m_scene && m_dialogBox && !m_isStoryFinished)
    {
        if (event->type() == QEvent::GraphicsSceneMousePress ||
            event->type() == QEvent::KeyPress)
        {
            if (event->type() == QEvent::KeyPress)
            {
                auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
                // ESC键不拦截，让它传递给GameView处理暂停菜单
                if (keyEvent->key() == Qt::Key_Escape)
                {
                    return false; // 不拦截，让事件继续传播
                }
                if (!m_skipRequested &&
                    keyEvent->modifiers().testFlag(Qt::ControlModifier) &&
                    keyEvent->key() == Qt::Key_S)
                {
                    m_skipRequested = true;
                    finishStory();
                    return true;
                }
                if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter ||
                    keyEvent->key() == Qt::Key_Space)
                {
                    onDialogClicked();
                    return true; // 阻止事件继续传播
                }
            }
            else if (event->type() == QEvent::GraphicsSceneMousePress)
            {
                onDialogClicked();
                return true; // 阻止事件继续传播
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

// 处理鼠标点击继续对话
void Level::onDialogClicked()
{
    if (!m_isStoryFinished)
    {
        nextDialog();
    }
}

void Level::showStoryDialog(const QStringList &dialogs, bool isBossDialog, const QString &customBackground)
{
    m_currentDialogs = dialogs;
    m_currentDialogIndex = 0;
    m_isBossDialog = isBossDialog;
    m_isStoryFinished = false;
    m_isTeacherBossInitialDialog = false; // 重置标记

    // 发送对话开始信号
    emit dialogStarted();

    // 检查是否使用透明背景（不显示背景图片，保持游戏画面可见）
    bool useTransparentBackground = (customBackground == "transparent");

    if (!useTransparentBackground)
    {
        QString imagePath;
        // 如果指定了自定义背景，使用自定义背景
        if (!customBackground.isEmpty())
        {
            imagePath = customBackground;
        }
        else
        {
            // 使用默认背景
            if (m_levelNumber == 1)
                imagePath = "assets/galgame/l1.png";
            else if (m_levelNumber == 2)
                imagePath = "assets/galgame/l2.png";
            else
                imagePath = "assets/galgame/l3.png";
        }

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

        // 第三关Boss初始对话：添加cow入场飞行动画
        if (m_levelNumber == 3 && isBossDialog && !m_currentTeacherBoss)
        {
            m_isTeacherBossInitialDialog = true;

            // 加载cow图片
            QStringList cowPaths = {
                "assets/boss/Teacher/cow.png",
                "../assets/boss/Teacher/cow.png",
                "../../our_game/assets/boss/Teacher/cow.png"};

            QPixmap cowPixmap;
            for (const QString &path : cowPaths)
            {
                if (QFile::exists(path))
                {
                    cowPixmap = QPixmap(path);
                    break;
                }
            }

            if (!cowPixmap.isNull())
            {
                // 缩放cow图片（放大）
                cowPixmap = cowPixmap.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                m_dialogBossSprite = new QGraphicsPixmapItem(cowPixmap);
                m_dialogBossSprite->setPos(-200, 180); // 从屏幕左侧外开始
                m_dialogBossSprite->setZValue(10001);  // 在背景之上
                m_scene->addItem(m_dialogBossSprite);

                // 创建飞入动画：从左侧飞到中间偏左
                m_dialogBossFlyAnimation = new QPropertyAnimation(this);
                m_dialogBossFlyAnimation->setTargetObject(nullptr);
                m_dialogBossFlyAnimation->setDuration(2000);
                m_dialogBossFlyAnimation->setStartValue(QPointF(-200, 180));
                m_dialogBossFlyAnimation->setEndValue(QPointF(250, 180)); // 飞到更靠左位置
                m_dialogBossFlyAnimation->setEasingCurve(QEasingCurve::OutCubic);

                // 使用QVariantAnimation来更新位置
                connect(m_dialogBossFlyAnimation, &QPropertyAnimation::valueChanged, this, [this](const QVariant &value)
                        {
                    if (m_dialogBossSprite) {
                        m_dialogBossSprite->setPos(value.toPointF());
                    } });

                m_dialogBossFlyAnimation->start();
                qDebug() << "[Level] TeacherBoss初始对话：cow入场动画开始";
            }
        }
        // 第三关Boss二三阶段对话或击败后对话：让cow直接出现在中间
        else if (m_levelNumber == 3 && isBossDialog && m_currentTeacherBoss)
        {
            // 击败后用cowFinal.png，其他阶段都用cow.png
            QString cowImageName = m_bossDefeated ? "cowFinal.png" : "cow.png";
            QStringList cowPaths = {
                "assets/boss/Teacher/" + cowImageName,
                "../assets/boss/Teacher/" + cowImageName,
                "../../our_game/assets/boss/Teacher/" + cowImageName};

            QPixmap cowPixmap;
            for (const QString &path : cowPaths)
            {
                if (QFile::exists(path))
                {
                    cowPixmap = QPixmap(path);
                    break;
                }
            }

            if (!cowPixmap.isNull())
            {
                cowPixmap = cowPixmap.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                m_dialogBossSprite = new QGraphicsPixmapItem(cowPixmap);
                m_dialogBossSprite->setPos(250, 180); // 直接出现在更靠左位置
                m_dialogBossSprite->setZValue(10001);
                m_scene->addItem(m_dialogBossSprite);
                qDebug() << "[Level] TeacherBoss阶段" << m_currentTeacherBoss->getPhase() << "对话：cow直接出现";
            }
        }
    }
    else
    {
        // 透明背景模式：不创建背景图片，m_dialogBox 保持 nullptr
        m_dialogBox = nullptr;
    }

    QPixmap gradientPixmap(800, 250);
    gradientPixmap.fill(Qt::transparent);

    QPainter painter(&gradientPixmap);
    QLinearGradient gradient(0, 0, 0, 250);
    gradient.setColorAt(0.0, QColor(0, 0, 0, 0));   // 顶部完全透明
    gradient.setColorAt(0.3, QColor(0, 0, 0, 160)); // 渐变为半透明
    gradient.setColorAt(0.7, QColor(0, 0, 0, 200)); // 中间更深的半透明
    gradient.setColorAt(1.0, QColor(0, 0, 0, 250)); // 底部最深的半透明

    painter.fillRect(0, 0, 800, 250, gradient);
    painter.end();

    m_textBackground = new QGraphicsPixmapItem(gradientPixmap);
    m_textBackground->setPos(0, 350);
    m_textBackground->setZValue(10001);
    m_scene->addItem(m_textBackground);

    // 创建对话框文本
    m_dialogText = new QGraphicsTextItem();
    m_dialogText->setDefaultTextColor(Qt::white);
    m_dialogText->setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
    m_dialogText->setTextWidth(700);
    m_dialogText->setPos(50, 400);
    m_dialogText->setZValue(10002);
    m_scene->addItem(m_dialogText);

    // 创建继续提示
    m_continueHint = new QGraphicsTextItem("点击或按Enter键继续...");
    m_continueHint->setDefaultTextColor(QColor(255, 255, 255, 180));
    m_continueHint->setFont(QFont("Microsoft YaHei", 12, QFont::Light));
    m_continueHint->setPos(580, 550);
    m_continueHint->setZValue(10002);
    m_scene->addItem(m_continueHint);
    m_skipHint = new QGraphicsTextItem("Ctrl+S 跳过剧情");
    m_skipHint->setDefaultTextColor(QColor(255, 255, 255, 140));
    m_skipHint->setFont(QFont("Microsoft YaHei", 11, QFont::Light));
    m_skipHint->setPos(30, 550);
    m_skipHint->setZValue(10002);
    m_scene->addItem(m_skipHint);

    // 让渐变背景可点击
    m_textBackground->setFlag(QGraphicsItem::ItemIsFocusable);
    m_textBackground->setAcceptHoverEvents(true);

    // 先移除旧的事件过滤器（如果存在），再安装新的
    m_scene->removeEventFilter(this);
    m_scene->installEventFilter(this);

    // 暂停所有房间的定时器（防止对话期间玩家被攻击）
    if (isBossDialog)
    {
        for (Room *room : m_rooms)
        {
            if (room)
            {
                room->stopChangeTimer();
            }
        }
        // 暂停所有敌人的AI和攻击
        for (const QPointer<Enemy> &enemyPtr : m_currentEnemies)
        {
            if (enemyPtr)
            {
                enemyPtr->pauseTimers();
            }
        }
    }

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
    m_skipRequested = false;
    // 清理渐变背景
    if (m_textBackground)
    {
        m_scene->removeItem(m_textBackground);
        delete m_textBackground;
        m_textBackground = nullptr;
    }

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
    if (m_skipHint)
    {
        m_scene->removeItem(m_skipHint);
        delete m_skipHint;
        m_skipHint = nullptr;
    }

    // 清理对话期间的Boss角色图片（入场动画）
    if (m_dialogBossFlyAnimation)
    {
        m_dialogBossFlyAnimation->stop();
        delete m_dialogBossFlyAnimation;
        m_dialogBossFlyAnimation = nullptr;
    }
    if (m_dialogBossSprite)
    {
        m_scene->removeItem(m_dialogBossSprite);
        delete m_dialogBossSprite;
        m_dialogBossSprite = nullptr;
    }
    m_isTeacherBossInitialDialog = false;

    m_isStoryFinished = true;

    // 发送对话结束信号
    emit dialogFinished();

    // 如果是boss对话结束，初始化当前房间
    if (m_isBossDialog)
    {
        m_isBossDialog = false;

        // 恢复所有敌人的定时器
        for (const QPointer<Enemy> &enemyPtr : m_currentEnemies)
        {
            if (enemyPtr)
            {
                enemyPtr->resumeTimers();
            }
        }

        // 检查是否是WashMachineBoss的中途对话（变异阶段）
        if (m_currentWashMachineBoss && m_currentWashMachineBoss->getPhase() >= 2)
        {
            qDebug() << "WashMachineBoss对话结束，通知Boss继续战斗";

            // 释放所有被吸纳的实体（随机分布在场景中）
            for (int i = 0; i < m_absorbingItems.size(); ++i)
            {
                QGraphicsItem *item = m_absorbingItems[i];
                if (!item)
                    continue;

                if (item == m_player)
                {
                    // 恢复玩家
                    m_player->setVisible(true);
                    m_player->setCanMove(true);
                    m_player->setCanShoot(true);
                    m_player->setPermanentInvincible(false); // 取消持久无敌
                    m_player->setScale(1.0);
                    // 将玩家放到安全位置
                    m_player->setPos(400, 450);
                    qDebug() << "[吸纳] 玩家已恢复";
                }
                else if (item != m_currentWashMachineBoss)
                {
                    // 释放敌人，随机分布在场景中
                    Enemy *enemy = dynamic_cast<Enemy *>(item);
                    if (enemy)
                    {
                        // 随机位置
                        int x = QRandomGenerator::global()->bounded(100, 700);
                        int y = QRandomGenerator::global()->bounded(100, 500);
                        enemy->setPos(x, y);
                        enemy->setScale(1.0); // 恢复缩放
                        enemy->setVisible(true);
                        enemy->resumeTimers(); // 恢复敌人定时器
                        qDebug() << "释放敌人到位置:" << x << "," << y;
                    }
                }
            }

            // 清空吸纳列表
            m_absorbingItems.clear();
            m_absorbStartPositions.clear();
            m_absorbAngles.clear();

            m_currentWashMachineBoss->onDialogFinished();
        }
        else if (m_currentTeacherBoss && m_currentTeacherBoss->getPhase() >= 2)
        {
            // TeacherBoss的阶段转换对话（第二阶段或第三阶段）
            // 不需要initCurrentRoom，只需通知Boss继续战斗
            qDebug() << "TeacherBoss阶段转换对话结束，继续战斗";
            m_currentTeacherBoss->onDialogFinished();
        }
        else
        {
            // 初始Boss对话结束，或WashMachineBoss第一轮对话结束
            // 需要初始化房间（创建Boss和敌人）
            if (m_currentRoomIndex >= 0 && m_currentRoomIndex < m_rooms.size())
            {
                qDebug() << "Boss对话结束，初始化boss房间" << m_currentRoomIndex;
                initCurrentRoom(m_rooms[m_currentRoomIndex]);
            }
            // 在房间初始化后再通知Boss（这样文字会显示在最顶层）
            if (m_currentWashMachineBoss)
            {
                m_currentWashMachineBoss->onDialogFinished();
            }
            // TeacherBoss在构造函数中已经自动启动了第一阶段技能
        }
    }
    else
    {
        // 关卡开始对话结束
        emit storyFinished();
    }
}

void Level::showPhaseTransitionText(const QString &text)
{
    // 创建文字项，复用boss门开启提示的样式（深紫色）
    QGraphicsTextItem *textItem = new QGraphicsTextItem(text);
    textItem->setDefaultTextColor(QColor(75, 0, 130)); // 深紫色
    textItem->setFont(QFont("Arial", 16, QFont::Bold));

    // 居中显示
    QRectF textRect = textItem->boundingRect();
    textItem->setPos((800 - textRect.width()) / 2, 280); // 在屏幕中上方显示
    textItem->setZValue(1000);                           // 确保在最上层
    m_scene->addItem(textItem);

    // 2秒后自动移除，使用QPointer保护指针
    QPointer<QGraphicsScene> scenePtr(m_scene);
    QTimer::singleShot(2000, this, [textItem, scenePtr]()
                       {
        if (scenePtr && textItem->scene() == scenePtr) {
            scenePtr->removeItem(textItem);
        }
        delete textItem;
        qDebug() << "阶段转换文字已移除"; });
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
    levelTextItem->setFont(QFont("Arial", 36, QFont::Bold));

    int sceneWidth = 800;
    int sceneHeight = 600;
    levelTextItem->setPos(sceneWidth / 2 - 200, sceneHeight / 2 - 60);
    levelTextItem->setZValue(10010);
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
            if (m_backgroundItem)
            {
                m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                m_backgroundItem->setPos(0, 0);
            }
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
    qDebug() << "初始化房间" << m_currentRoomIndex << "完成，房间敌人数:" << room->currentEnemies.size() << "，全局敌人数:"
             << m_currentEnemies.size();

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
    Room *cur = m_rooms[roomIndex];

    // 如果房间已被标记为清除（开发者模式跳关），跳过敌人生成
    if (cur && cur->isCleared())
    {
        qDebug() << "房间" << roomIndex << "已被清除，跳过敌人生成";
        openDoors(cur);
        return;
    }

    try
    {
        bool hasBoss = roomCfg.hasBoss;

        // 计算总敌人数量（用于判断是否打开门）
        int totalEnemyCount = 0;
        for (const EnemySpawnConfig &enemyCfg : roomCfg.enemies)
        {
            totalEnemyCount += enemyCfg.count;
        }

        // 如果没有敌人且不是战斗房间，或者战斗已经结束，则打开门
        if (totalEnemyCount == 0 && (!cur->isBattleRoom() || cur->isBattleStarted()))
        {
            openDoors(cur);
        }

        // 根据配置生成各种类型的敌人
        for (const EnemySpawnConfig &enemyCfg : roomCfg.enemies)
        {
            QString enemyType = enemyCfg.type;
            int count = enemyCfg.count;

            // 特殊处理ClockBoom类型
            if (enemyType == "clock_boom")
            {
                // 使用新的配置接口获取具体敌人类型的尺寸
                int enemySize = ConfigManager::instance().getEntitySize("enemies", "clock_boom");
                if (enemySize <= 0)
                    enemySize = 40; // 默认值
                QPixmap boomNormalPic = ResourceFactory::createEnemyImage(enemySize, m_levelNumber, "clock_boom");

                for (int i = 0; i < count; ++i)
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
            else
            {
                // 普通敌人生成 - 使用新的配置接口获取具体敌人类型的尺寸
                int enemySize = ConfigManager::instance().getEntitySize("enemies", enemyType);
                if (enemySize <= 0)
                    enemySize = 40; // 默认值
                QPixmap enemyPix = ResourceFactory::createEnemyImage(enemySize, m_levelNumber, enemyType);

                for (int i = 0; i < count; ++i)
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
            }
        }

        if (hasBoss)
        {
            // 根据关卡号确定Boss类型
            QString bossType;
            if (m_levelNumber == 1)
            {
                bossType = "nightmare";
            }
            else
            {
                bossType = "washmachine";
            }

            // 加载对应的Boss图片（使用新的配置接口获取具体Boss类型的尺寸）
            int bossSize = ConfigManager::instance().getEntitySize("bosses", bossType);
            QPixmap bossPix = ResourceFactory::createBossImage(bossSize, m_levelNumber, bossType);

            int x = QRandomGenerator::global()->bounded(100, 700);
            int y = QRandomGenerator::global()->bounded(100, 500);

            if (qAbs(x - 400) < 100 && qAbs(y - 300) < 100)
            {
                x += 150;
                y += 150;
            }

            // 使用工厂方法创建对应的Boss
            Boss *boss = createBossByLevel(m_levelNumber, bossPix, 1.0);
            boss->setPos(x, y);
            boss->setPlayer(m_player);
            m_scene->addItem(boss);

            // 使用 QPointer<Enemy> 存储，兼容 Qt5 和 Qt6
            QPointer<Enemy> eptr(static_cast<Enemy *>(boss));
            m_currentEnemies.append(eptr);
            // Room::currentEnemies 也应为 QVector<QPointer<Enemy>>
            m_rooms[roomIndex]->currentEnemies.append(eptr);

            connect(boss, &Boss::dying, this, &Level::onEnemyDying);
            qDebug() << "创建boss类型:" << bossType << "位置:" << x << "," << y << "已连接dying信号";
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

// 渐变背景到目标图片路径（绝对或相对 assets/），duration 毫秒
void Level::fadeBackgroundTo(const QString &imagePath, int duration)
{
    if (!m_scene)
        return;

    QString fullPath = imagePath;
    // 如果路径看起来像相对assets路径（未包含 assets/ 前缀），尝试从 ConfigManager 获取
    if (!imagePath.startsWith("assets/"))
    {
        fullPath = ConfigManager::instance().getAssetPath(imagePath);
    }

    QPixmap newBg;
    try
    {
        newBg = ResourceFactory::loadImage(fullPath);
    }
    catch (const QString &e)
    {
        qWarning() << "fadeBackgroundTo: 加载图片失败:" << e;
        return;
    }

    if (newBg.isNull())
    {
        qWarning() << "fadeBackgroundTo: 无法加载图片:" << fullPath;
        return;
    }

    // 创建覆盖项（位于背景之上，但低于场景中其它元素）
    if (m_backgroundOverlay)
    {
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
    QVariantAnimation *anim = new QVariantAnimation(this);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setDuration(duration);
    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value)
            {
        if (m_backgroundOverlay)
            m_backgroundOverlay->setOpacity(value.toDouble()); });
    connect(anim, &QVariantAnimation::finished, this, [this, fullPath]()
            {
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
        } });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
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
        else if (enemyType == "pillow")
        {
            return new PillowEnemy(pic, scale);
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
        else if (enemyType == "pants")
        {
            return new PantsEnemy(pic, scale);
        }
        else if (enemyType == "sock_shooter")
        {
            return new SockShooter(pic, scale);
        }
        else if (enemyType == "walker")
        {
            return new Walker(pic, scale);
        }
    }
    else if (levelNumber == 3)
    {
        if (enemyType == "optimization")
        {
            return new OptimizationEnemy(pic, scale);
        }
        else if (enemyType == "digital_system")
        {
            return new DigitalSystemEnemy(pic, scale);
        }
        else if (enemyType == "yanglin")
        {
            return new YanglinEnemy(pic, scale);
        }
    }

    // 默认返回基础敌人
    qDebug() << "未知敌人类型，使用默认Enemy:" << enemyType;
    return new Enemy(pic, scale);
}

Boss *Level::createBossByLevel(int levelNumber, const QPixmap &pic, double scale)
{
    Boss *boss = nullptr;

    // 根据关卡号创建对应的Boss
    switch (levelNumber)
    {
    case 1:
    {
        // 第一关：Nightmare Boss
        qDebug() << "创建Nightmare Boss（第一关）";
        NightmareBoss *nightmareBoss = new NightmareBoss(pic, scale, m_scene);

        // 连接Nightmare Boss的特殊信号
        connect(nightmareBoss, &NightmareBoss::requestSpawnEnemies,
                this, &Level::spawnEnemiesForBoss);
        // 当一阶段亡语触发时，开始背景渐变到噩梦关专用背景
        connect(nightmareBoss, &NightmareBoss::phase1DeathTriggered,
                this, [this]()
                { this->fadeBackgroundTo(QString("assets/background/nightmare2_map.png"), 3000); });

        boss = nightmareBoss;
        break;
    }

    case 2:
    {
        // 第二关：WashMachine Boss
        qDebug() << "创建WashMachine Boss（第二关）";
        WashMachineBoss *washMachineBoss = new WashMachineBoss(pic, scale);

        // 连接WashMachine Boss的特殊信号
        connectWashMachineBossSignals(washMachineBoss);

        boss = washMachineBoss;
        break;
    }

    case 3:
    {
        // 第三关：Teacher Boss（奶牛张）
        qDebug() << "创建Teacher Boss（第三关）";
        TeacherBoss *teacherBoss = new TeacherBoss(pic, scale);

        // 连接Teacher Boss的特殊信号
        connectTeacherBossSignals(teacherBoss);

        boss = teacherBoss;
        break;
    }

    default:
        // 默认Boss
        qDebug() << "使用默认Boss";
        boss = new Boss(pic, scale);
        break;
    }

    return boss;
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
        hudNode.visited = false;       // Will be updated by HUD
        hudNode.hasBoss = cfg.hasBoss; // 标记boss房间
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
            // 同步 visited 状态（开发者模式跳关时需要）
            hud->syncVisitedRooms(visited);
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
    // 遮罩效果现在由NightmareBoss自己管理，其析构函数会自动清理

    // Delete all entities completely (used when resetting level)
    for (QPointer<Enemy> enemyPtr : m_currentEnemies)
    {
        Enemy *enemy = enemyPtr.data();
        if (enemy)
        {
            // 断开所有信号连接
            disconnect(enemy, nullptr, this, nullptr);
            disconnect(enemy, nullptr, nullptr, nullptr);
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

    // 检测是否首次进入boss房间（使用之前已声明的currentRoomCfg）
    const RoomConfig &newRoomCfg = config.getRoom(m_currentRoomIndex);
    bool isFirstEnterBossRoom = !visited[m_currentRoomIndex] && newRoomCfg.hasBoss;

    if (!visited[m_currentRoomIndex])
    {
        visited[m_currentRoomIndex] = true;
        visited_count++;

        // 检查是否遇到通往boss房间的门
        if (!m_hasEncounteredBossDoor && !newRoomCfg.hasBoss)
        {
            if ((newRoomCfg.doorUp >= 0 && config.getRoom(newRoomCfg.doorUp).hasBoss) ||
                (newRoomCfg.doorDown >= 0 && config.getRoom(newRoomCfg.doorDown).hasBoss) ||
                (newRoomCfg.doorLeft >= 0 && config.getRoom(newRoomCfg.doorLeft).hasBoss) ||
                (newRoomCfg.doorRight >= 0 && config.getRoom(newRoomCfg.doorRight).hasBoss))
            {
                m_hasEncounteredBossDoor = true;
                qDebug() << "首次遇到boss门，标记状态";
            }
        }

        // 如果是首次进入boss房间且有对话配置，显示对话
        if (isFirstEnterBossRoom && !newRoomCfg.bossDialog.isEmpty())
        {
            qDebug() << "首次进入boss房间" << m_currentRoomIndex << "，显示boss对话";
            // 传递boss对话标志和自定义背景（如果有）
            showStoryDialog(newRoomCfg.bossDialog, true, newRoomCfg.bossDialogBackground);
            // 等待对话结束后再初始化房间，通过finishStory处理
            // initCurrentRoom会在对话结束后由finishStory调用
        }
        else
        {
            initCurrentRoom(m_rooms[m_currentRoomIndex]);
        }
    }
    else
    {
        loadRoom(m_currentRoomIndex);
    }

    // 在进入非boss房间后，检查是否满足打开boss门的条件
    if (!newRoomCfg.hasBoss && m_hasEncounteredBossDoor && !m_bossDoorsAlreadyOpened)
    {
        if (canOpenBossDoor())
        {
            qDebug() << "进入房间后检测到满足boss门开启条件，立即打开boss门";
            m_bossDoorsAlreadyOpened = true;
            openBossDoors();
        }
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
            if (m_backgroundItem)
            {
                m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                m_backgroundItem->setPos(0, 0);
            }
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

    // 在重新加载非boss房间后，检查是否满足打开boss门的条件
    if (config.loadFromFile(m_levelNumber))
    {
        const RoomConfig &roomCfg = config.getRoom(roomIndex);
        if (!roomCfg.hasBoss && m_hasEncounteredBossDoor && !m_bossDoorsAlreadyOpened)
        {
            if (canOpenBossDoor())
            {
                qDebug() << "重新加载房间后检测到满足boss门开启条件，立即打开boss门";
                m_bossDoorsAlreadyOpened = true;
                openBossDoors();
            }
        }
    }
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
    // 移除频繁的调试输出以避免性能问题
    // qDebug() << "onEnemyDying 被调用";

    if (!enemy)
    {
        qWarning() << "onEnemyDying: enemy 为 null";
        return;
    }

    // 检查是否是Boss死亡
    bool isBoss = dynamic_cast<Boss *>(enemy) != nullptr;

    // 使用 QPointer 包装目标并用 removeAll 安全移除
    QPointer<Enemy> target(enemy);
    m_currentEnemies.removeAll(target);

    // 移除频繁的调试输出以避免性能问题
    // qDebug() << "已从全局敌人列表移除，剩余:" << m_currentEnemies.size();

    // 只有非召唤的敌人才触发bonusEffects
    // 延迟执行bonusEffects，避免在dying信号处理中访问不稳定状态
    if (!enemy->isSummoned())
    {
        QPointer<Level> levelPtr(this);
        QPointer<Player> playerPtr(m_player);
        QTimer::singleShot(100, this, [levelPtr, playerPtr]()
                           {
            if (levelPtr && playerPtr) {
                levelPtr->bonusEffects();
            } });
    }

    // 如果死亡的是NightmareBoss，恢复原始背景（3秒渐变）
    if (dynamic_cast<NightmareBoss *>(enemy))
    {
        // 如果有原始路径则恢复，否则不处理
        if (!m_originalBackgroundPath.isEmpty())
        {
            fadeBackgroundTo(m_originalBackgroundPath, 3000);
        }
    }

    for (Room *r : m_rooms)
    {
        if (!r)
            continue;
        r->currentEnemies.removeAll(target);
    }

    Room *cur = m_rooms[m_currentRoomIndex];
    // 移除频繁的调试输出以避免性能问题
    // qDebug() << "当前房间" << m_currentRoomIndex << "敌人数量:" << (cur ? cur->currentEnemies.size() : -1);

    // 如果是Boss死亡，无论房间是否清空，都标记Boss已被击败
    if (isBoss && !m_bossDefeated)
    {
        qDebug() << "[Level] Boss被击败，标记m_bossDefeated = true，当前房间剩余敌人:"
                 << (cur ? cur->currentEnemies.size() : 0);
        m_bossDefeated = true;
    }

    if (cur && cur->currentEnemies.isEmpty())
    {
        // 检查是否是Boss房间
        LevelConfig config;
        if (config.loadFromFile(m_levelNumber))
        {
            const RoomConfig &roomCfg = config.getRoom(m_currentRoomIndex);

            // 如果是Boss房间
            if (roomCfg.hasBoss)
            {
                // Boss刚死亡，记录标志并等待小怪清完
                if (isBoss)
                {
                    qDebug() << "[Level] Boss被击败，房间已清空，启动奖励流程";
                    m_bossDefeated = true;
                    // 延迟启动奖励流程，等待Boss死亡动画完成
                    QTimer::singleShot(1500, this, &Level::startBossRewardSequence);
                    return; // 不执行正常的开门逻辑
                }
                // 小怪死亡但Boss之前已被击败，现在房间清空，启动奖励流程
                else if (m_bossDefeated && !m_rewardSequenceActive)
                {
                    qDebug() << "[Level] Boss房间小怪已清空，Boss之前已被击败，启动奖励流程";
                    QTimer::singleShot(500, this, &Level::startBossRewardSequence);
                    return; // 不执行正常的开门逻辑
                }
            }
        }

        // 非Boss房间或奖励流程已在进行中，正常开门
        if (!m_rewardSequenceActive)
        {
            openDoors(cur);
        }

        // 在战斗房间清空敌人后，检查是否满足打开boss门的条件
        if (config.loadFromFile(m_levelNumber))
        {
            const RoomConfig &roomCfg = config.getRoom(m_currentRoomIndex);
            if (!roomCfg.hasBoss && m_hasEncounteredBossDoor && !m_bossDoorsAlreadyOpened)
            {
                if (canOpenBossDoor())
                {
                    qDebug() << "战斗房间清空后检测到满足boss门开启条件，立即打开boss门";
                    m_bossDoorsAlreadyOpened = true;
                    openBossDoors();
                }
            }
        }
    }
}

void Level::openDoors(Room *cur)
{
    LevelConfig config;
    bool up = false, down = false, left = false, right = false;
    bool anyDoorOpened = false; // 追踪是否有门被打开

    if (config.loadFromFile(m_levelNumber))
    {
        const RoomConfig &roomCfg = config.getRoom(m_currentRoomIndex);

        int doorIndex = 0;

        if (roomCfg.doorUp >= 0)
        {
            int neighborRoom = roomCfg.doorUp;
            const RoomConfig &neighborCfg = config.getRoom(neighborRoom);

            // 最高优先级：如果邻居是boss房间，必须所有非boss房间都已访问才能开门
            if (neighborCfg.hasBoss && !canOpenBossDoor())
            {
                qDebug() << "上门通往boss房间" << neighborRoom << "，但还有房间未探索，保持关闭";
                doorIndex++;
            }
            else
            {
                cur->setDoorOpenUp(true);
                up = true;
                anyDoorOpened = true;
                // 播放开门动画
                if (doorIndex < m_currentDoors.size())
                {
                    m_currentDoors[doorIndex]->open();
                }
                doorIndex++;
                qDebug() << "打开上门，通往房间" << neighborRoom;

                // 设置邻房间的对应门为打开状态（除非邻居是未访问的战斗房间）
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
        }
        if (roomCfg.doorDown >= 0)
        {
            int neighborRoom = roomCfg.doorDown;
            const RoomConfig &neighborCfg = config.getRoom(neighborRoom);

            // 最高优先级：如果邻居是boss房间，必须所有非boss房间都已访问才能开门
            if (neighborCfg.hasBoss && !canOpenBossDoor())
            {
                qDebug() << "下门通往boss房间" << neighborRoom << "，但还有房间未探索，保持关闭";
                doorIndex++;
            }
            else
            {
                cur->setDoorOpenDown(true);
                down = true;
                anyDoorOpened = true;
                if (doorIndex < m_currentDoors.size())
                {
                    m_currentDoors[doorIndex]->open();
                }
                doorIndex++;
                qDebug() << "打开下门，通往房间" << neighborRoom;

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
        }
        if (roomCfg.doorLeft >= 0)
        {
            int neighborRoom = roomCfg.doorLeft;
            const RoomConfig &neighborCfg = config.getRoom(neighborRoom);

            // 最高优先级：如果邻居是boss房间，必须所有非boss房间都已访问才能开门
            if (neighborCfg.hasBoss && !canOpenBossDoor())
            {
                qDebug() << "左门通往boss房间" << neighborRoom << "，但还有房间未探索，保持关闭";
                doorIndex++;
            }
            else
            {
                cur->setDoorOpenLeft(true);
                left = true;
                anyDoorOpened = true;
                if (doorIndex < m_currentDoors.size())
                {
                    m_currentDoors[doorIndex]->open();
                }
                doorIndex++;
                qDebug() << "打开左门，通往房间" << neighborRoom;

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
        }
        if (roomCfg.doorRight >= 0)
        {
            int neighborRoom = roomCfg.doorRight;
            const RoomConfig &neighborCfg = config.getRoom(neighborRoom);

            // 最高优先级：如果邻居是boss房间，必须所有非boss房间都已访问才能开门
            if (neighborCfg.hasBoss && !canOpenBossDoor())
            {
                qDebug() << "右门通往boss房间" << neighborRoom << "，但还有房间未探索，保持关闭";
                doorIndex++;
            }
            else
            {
                cur->setDoorOpenRight(true);
                right = true;
                anyDoorOpened = true;
                if (doorIndex < m_currentDoors.size())
                {
                    m_currentDoors[doorIndex]->open();
                }
                doorIndex++;
                qDebug() << "打开右门，通往房间" << neighborRoom;

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
    }

    // 只有在实际打开了门时才播放音效和记录日志
    if (anyDoorOpened)
    {
        AudioManager::instance().playSound("door_open");
        qDebug() << "房间" << m_currentRoomIndex << "敌人全部清空，门已打开";
    }
    else
    {
        qDebug() << "房间" << m_currentRoomIndex << "敌人已清空，但所有门都通往boss房间且条件不满足";
    }

    if (m_currentRoomIndex == m_rooms.size() - 1)
        emit levelCompleted(m_levelNumber);

    // 只有战斗房间才发射敌人清空信号
    if (cur && cur->isBattleRoom())
    {
        emit enemiesCleared(m_currentRoomIndex, up, down, left, right);
    }
}

bool Level::canOpenBossDoor() const
{
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber))
        return false;

    // 遍历所有房间，检查非boss房间是否都已访问且怪物已清空
    for (int i = 0; i < config.getRoomCount(); ++i)
    {
        const RoomConfig &roomCfg = config.getRoom(i);
        // 如果是非boss房间
        if (!roomCfg.hasBoss)
        {
            // 必须已访问
            if (!visited[i])
            {
                qDebug() << "房间" << i << "尚未访问，boss门保持关闭";
                return false;
            }
            // 必须怪物已清空（检查房间是否还有存活的敌人）
            if (i < m_rooms.size() && m_rooms[i])
            {
                Room *room = m_rooms[i];
                // 如果房间还有存活的敌人
                if (!room->currentEnemies.isEmpty())
                {
                    bool hasAliveEnemy = false;
                    for (const QPointer<Enemy> &ePtr : room->currentEnemies)
                    {
                        if (ePtr)
                        {
                            hasAliveEnemy = true;
                            break;
                        }
                    }
                    if (hasAliveEnemy)
                    {
                        qDebug() << "房间" << i << "还有存活敌人，boss门保持关闭";
                        return false;
                    }
                }
            }
        }
    }

    qDebug() << "所有非boss房间已访问并清空，可以打开boss门";

    // 所有非boss房间都已访问且怪物清空
    return true;
}

void Level::openBossDoors()
{
    LevelConfig config;
    if (!config.loadFromFile(m_levelNumber))
        return;

    qDebug() << "=== 开始打开所有通往boss房间的门 ===";

    bool shouldReloadCurrentRoom = false;

    // 遍历所有房间，找到所有通往boss房间的门并打开
    for (int roomIndex = 0; roomIndex < config.getRoomCount(); ++roomIndex)
    {
        const RoomConfig &roomCfg = config.getRoom(roomIndex);

        // 跳过boss房间本身
        if (roomCfg.hasBoss)
            continue;

        if (roomIndex >= m_rooms.size() || !m_rooms[roomIndex])
            continue;

        Room *room = m_rooms[roomIndex];

        qDebug() << "检查房间" << roomIndex << "的门（上:" << roomCfg.doorUp << "，下:" << roomCfg.doorDown
                 << "，左:" << roomCfg.doorLeft << "，右:" << roomCfg.doorRight << "）";

        // 检查当前房间的相邻房间是否有通往boss的门
        bool isAdjacentToBoss = false;
        if ((roomCfg.doorUp >= 0 && config.getRoom(roomCfg.doorUp).hasBoss) ||
            (roomCfg.doorDown >= 0 && config.getRoom(roomCfg.doorDown).hasBoss) ||
            (roomCfg.doorLeft >= 0 && config.getRoom(roomCfg.doorLeft).hasBoss) ||
            (roomCfg.doorRight >= 0 && config.getRoom(roomCfg.doorRight).hasBoss))
        {
            isAdjacentToBoss = true;
        }

        // 如果当前房间邻接boss房间，且是正在显示的房间，标记需要重新加载
        if (isAdjacentToBoss && roomIndex == m_currentRoomIndex)
        {
            shouldReloadCurrentRoom = true;
        }

        // 检查该房间的每个门是否通往boss房间
        if (roomCfg.doorUp >= 0)
        {
            const RoomConfig &neighborCfg = config.getRoom(roomCfg.doorUp);
            if (neighborCfg.hasBoss && !room->isDoorOpenUp())
            {
                qDebug() << "开启房间" << roomIndex << "通往boss房间" << roomCfg.doorUp << "的上门";
                room->setDoorOpenUp(true);

                // 找到并播放该门的开门动画
                // 如果是当前房间，使用 m_currentDoors
                if (roomIndex == m_currentRoomIndex && !m_currentDoors.isEmpty())
                {
                    for (Door *door : m_currentDoors)
                    {
                        if (door && door->direction() == Door::Up)
                        {
                            qDebug() << "  -> 在当前房间的 m_currentDoors 中找到上门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
                else if (m_roomDoors.contains(roomIndex))
                {
                    const QVector<Door *> &doors = m_roomDoors[roomIndex];
                    for (Door *door : doors)
                    {
                        if (door && door->direction() == Door::Up)
                        {
                            qDebug() << "  -> 在 m_roomDoors 中找到房间" << roomIndex << "的上门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
            }
        }

        if (roomCfg.doorDown >= 0)
        {
            const RoomConfig &neighborCfg = config.getRoom(roomCfg.doorDown);
            if (neighborCfg.hasBoss && !room->isDoorOpenDown())
            {
                qDebug() << "开启房间" << roomIndex << "通往boss房间" << roomCfg.doorDown << "的下门";
                room->setDoorOpenDown(true);

                if (roomIndex == m_currentRoomIndex && !m_currentDoors.isEmpty())
                {
                    for (Door *door : m_currentDoors)
                    {
                        if (door && door->direction() == Door::Down)
                        {
                            qDebug() << "  -> 在当前房间的 m_currentDoors 中找到下门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
                else if (m_roomDoors.contains(roomIndex))
                {
                    const QVector<Door *> &doors = m_roomDoors[roomIndex];
                    for (Door *door : doors)
                    {
                        if (door && door->direction() == Door::Down)
                        {
                            qDebug() << "  -> 在 m_roomDoors 中找到房间" << roomIndex << "的下门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
            }
        }

        if (roomCfg.doorLeft >= 0)
        {
            const RoomConfig &neighborCfg = config.getRoom(roomCfg.doorLeft);
            if (neighborCfg.hasBoss && !room->isDoorOpenLeft())
            {
                qDebug() << "开启房间" << roomIndex << "通往boss房间" << roomCfg.doorLeft << "的左门";
                room->setDoorOpenLeft(true);

                if (roomIndex == m_currentRoomIndex && !m_currentDoors.isEmpty())
                {
                    for (Door *door : m_currentDoors)
                    {
                        if (door && door->direction() == Door::Left)
                        {
                            qDebug() << "  -> 在当前房间的 m_currentDoors 中找到左门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
                else if (m_roomDoors.contains(roomIndex))
                {
                    const QVector<Door *> &doors = m_roomDoors[roomIndex];
                    for (Door *door : doors)
                    {
                        if (door && door->direction() == Door::Left)
                        {
                            qDebug() << "  -> 在 m_roomDoors 中找到房间" << roomIndex << "的左门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
            }
        }

        if (roomCfg.doorRight >= 0)
        {
            const RoomConfig &neighborCfg = config.getRoom(roomCfg.doorRight);
            if (neighborCfg.hasBoss && !room->isDoorOpenRight())
            {
                qDebug() << "开启房间" << roomIndex << "通往boss房间" << roomCfg.doorRight << "的右门";
                room->setDoorOpenRight(true);

                if (roomIndex == m_currentRoomIndex && !m_currentDoors.isEmpty())
                {
                    for (Door *door : m_currentDoors)
                    {
                        if (door && door->direction() == Door::Right)
                        {
                            qDebug() << "  -> 在当前房间的 m_currentDoors 中找到右门并打开";
                            door->open();
                            AudioManager::instance().playSound("door_open");
                        }
                    }
                }
                else if (m_roomDoors.contains(roomIndex))
                {
                    const QVector<Door *> &doors = m_roomDoors[roomIndex];
                    for (Door *door : doors)
                    {
                        if (door && door->direction() == Door::Right)
                        {
                            qDebug() << "  -> 在 m_roomDoors 中找到房间" << roomIndex << "的右门并打开";
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
    if (shouldReloadCurrentRoom)
    {
        qDebug() << "当前房间" << m_currentRoomIndex << "邻接boss房间，重新加载门以显示打开状态";

        // 清除当前所有门
        for (Door *door : m_currentDoors)
        {
            if (m_scene && door)
            {
                m_scene->removeItem(door);
            }
            delete door;
        }
        m_currentDoors.clear();

        // 从缓存中移除
        m_roomDoors.remove(m_currentRoomIndex);

        // 重新生成门（会读取房间状态，boss门已经被设置为打开）
        const RoomConfig &currentRoomCfg = config.getRoom(m_currentRoomIndex);
        spawnDoors(currentRoomCfg);

        qDebug() << "已重新加载当前房间的所有门";
    }
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

// Boss召唤敌人方法（通用）
void Level::spawnEnemiesForBoss(const QVector<QPair<QString, int>> &enemies)
{
    if (!m_scene)
        return;

    qDebug() << "Boss召唤敌人...";

    // 收集所有需要召唤的敌人，用于1秒后的操作
    QVector<QPointer<Enemy>> spawnedEnemies;

    for (const auto &enemyPair : enemies)
    {
        QString enemyType = enemyPair.first;
        int count = enemyPair.second;

        // 为每种敌人类型获取独立的尺寸配置
        int enemySize = ConfigManager::instance().getEntitySize("enemies", enemyType);
        if (enemySize <= 0)
            enemySize = 40; // 默认值

        qDebug() << "召唤" << count << "个" << enemyType << "尺寸:" << enemySize;

        if (enemyType == "clock_boom")
        {
            // 召唤clock_boom - 使用正确的图片路径
            QPixmap boomPic = ResourceFactory::createEnemyImage(enemySize, m_levelNumber, "clock_boom");

            // 如果加载失败，尝试直接加载
            if (boomPic.isNull())
            {
                boomPic = QPixmap("assets/enemy/level_1/clock_boom.png");
                if (!boomPic.isNull())
                {
                    boomPic = boomPic.scaled(enemySize, enemySize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
            }

            for (int i = 0; i < count; ++i)
            {
                // 立即随机生成在场景中
                int x = QRandomGenerator::global()->bounded(100, 700);
                int y = QRandomGenerator::global()->bounded(100, 500);

                ClockBoom *boom = new ClockBoom(boomPic, boomPic, 1.0);
                boom->setPos(x, y);
                boom->setPlayer(m_player);
                boom->setIsSummoned(true); // 标记为召唤的敌人，不触发bonus

                // 暂停敌人的AI和移动（等待1秒）
                boom->pauseTimers();

                m_scene->addItem(boom);

                QPointer<Enemy> eptr(boom);
                m_currentEnemies.append(eptr);
                spawnedEnemies.append(eptr);
                if (m_currentRoomIndex >= 0 && m_currentRoomIndex < m_rooms.size())
                {
                    m_rooms[m_currentRoomIndex]->currentEnemies.append(eptr);
                }

                connect(boom, &Enemy::dying, this, &Level::onEnemyDying);
            }
        }
        else if (enemyType == "clock_normal")
        {
            // 召唤clock_normal
            QPixmap normalPic = ResourceFactory::createEnemyImage(enemySize, m_levelNumber, "clock_normal");

            for (int i = 0; i < count; ++i)
            {
                // 立即随机生成在场景中
                int x = QRandomGenerator::global()->bounded(100, 700);
                int y = QRandomGenerator::global()->bounded(100, 500);

                Enemy *enemy = createEnemyByType(m_levelNumber, "clock_normal", normalPic, 1.0);
                enemy->setPos(x, y);
                enemy->setPlayer(m_player);
                enemy->setIsSummoned(true); // 标记为召唤的敌人，不触发bonus

                // 暂停敌人的AI和移动（等待1秒）
                enemy->pauseTimers();

                m_scene->addItem(enemy);

                QPointer<Enemy> eptr(enemy);
                m_currentEnemies.append(eptr);
                spawnedEnemies.append(eptr);
                if (m_currentRoomIndex >= 0 && m_currentRoomIndex < m_rooms.size())
                {
                    m_rooms[m_currentRoomIndex]->currentEnemies.append(eptr);
                }

                connect(enemy, &Enemy::dying, this, &Level::onEnemyDying);
            }
        }
        else if (enemyType == "pillow")
        {
            // 召唤pillow
            QPixmap pillowPic = ResourceFactory::createEnemyImage(enemySize, m_levelNumber, "pillow");

            for (int i = 0; i < count; ++i)
            {
                // 立即随机生成在场景中
                int x = QRandomGenerator::global()->bounded(100, 700);
                int y = QRandomGenerator::global()->bounded(100, 500);

                Enemy *enemy = createEnemyByType(m_levelNumber, "pillow", pillowPic, 1.0);
                enemy->setPos(x, y);
                enemy->setPlayer(m_player);
                enemy->setIsSummoned(true); // 标记为召唤的敌人，不触发bonus

                // 暂停敌人的AI和移动（等待1秒）
                enemy->pauseTimers();

                m_scene->addItem(enemy);

                QPointer<Enemy> eptr(enemy);
                m_currentEnemies.append(eptr);
                spawnedEnemies.append(eptr);
                if (m_currentRoomIndex >= 0 && m_currentRoomIndex < m_rooms.size())
                {
                    m_rooms[m_currentRoomIndex]->currentEnemies.append(eptr);
                }

                connect(enemy, &Enemy::dying, this, &Level::onEnemyDying);
            }
        }
    }

    // 1秒后：恢复所有敌人的移动，并让ClockBoom进入引爆动画
    QTimer::singleShot(1000, this, [spawnedEnemies]()
                       {
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
        qDebug() << "召唤完成，所有敌人开始移动，ClockBoom进入引爆动画"; });

    qDebug() << "Boss召唤敌人完成，当前场上敌人数:" << m_currentEnemies.size();
}

void Level::setPaused(bool paused)
{
    m_isPaused = paused;

    // 暂停/恢复所有当前敌人
    for (QPointer<Enemy> &enemyPtr : m_currentEnemies)
    {
        if (Enemy *enemy = enemyPtr.data())
        {
            if (paused)
            {
                enemy->pauseTimers();
            }
            else
            {
                enemy->resumeTimers();
            }
        }
    }

    // 暂停/恢复所有子弹
    if (m_scene)
    {
        QList<QGraphicsItem *> items = m_scene->items();
        for (QGraphicsItem *item : items)
        {
            if (Projectile *projectile = dynamic_cast<Projectile *>(item))
            {
                projectile->setPaused(paused);
            }
        }
    }

    qDebug() << "Level暂停状态:" << (paused ? "已暂停" : "已恢复");
}

// ==================== WashMachineBoss 相关方法 ====================

void Level::connectWashMachineBossSignals(WashMachineBoss *boss)
{
    if (!boss)
        return;

    m_currentWashMachineBoss = boss;

    // 连接请求对话信号
    connect(boss, &WashMachineBoss::requestShowDialog,
            this, &Level::onWashMachineBossRequestDialog);

    // 连接请求背景切换信号
    connect(boss, &WashMachineBoss::requestChangeBackground,
            this, &Level::onWashMachineBossRequestChangeBackground);

    // 连接请求显示文字提示信号
    connect(boss, &WashMachineBoss::requestShowTransitionText,
            this, &Level::showPhaseTransitionText);

    // 连接请求吸纳信号
    connect(boss, &WashMachineBoss::requestAbsorbAllEntities,
            this, &Level::onWashMachineBossRequestAbsorb);

    // 连接召唤敌人信号
    connect(boss, &WashMachineBoss::requestSpawnEnemies,
            this, &Level::spawnEnemiesForBoss);

    qDebug() << "WashMachineBoss信号已连接";
}

void Level::onWashMachineBossRequestDialog(const QStringList &dialogs, const QString &background)
{
    qDebug() << "WashMachineBoss请求显示对话";

    // 显示对话（使用boss对话模式）
    showStoryDialog(dialogs, true, background);

    // 对话结束后通知Boss继续
    // 注意：finishStory中会根据m_isBossDialog处理
}

void Level::onWashMachineBossRequestChangeBackground(const QString &backgroundPath)
{
    qDebug() << "WashMachineBoss请求更换背景:" << backgroundPath;
    changeBackground(backgroundPath);
}

void Level::onWashMachineBossRequestAbsorb()
{
    qDebug() << "WashMachineBoss请求执行吸纳动画";

    if (m_currentWashMachineBoss)
    {
        performAbsorbAnimation(m_currentWashMachineBoss.data());
    }
}

void Level::changeBackground(const QString &backgroundPath)
{
    if (!m_scene || !m_backgroundItem)
        return;

    QPixmap bg(backgroundPath);
    if (!bg.isNull())
    {
        m_backgroundItem->setPixmap(bg.scaled(800, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        qDebug() << "背景已切换为:" << backgroundPath;
    }
    else
    {
        qWarning() << "无法加载背景图片:" << backgroundPath;
    }
}

void Level::performAbsorbAnimation(WashMachineBoss *boss)
{
    if (!boss || !m_scene || !m_player)
        return;

    m_isAbsorbAnimationActive = true;

    // 计算Boss中心作为吸纳目标点
    m_absorbCenter = boss->pos() + QPointF(boss->pixmap().width() / 2.0, boss->pixmap().height() / 2.0);

    // 清理场景中所有子弹（避免吸纳期间伤害）
    if (m_scene)
    {
        QList<QGraphicsItem *> allItems = m_scene->items();
        for (QGraphicsItem *item : allItems)
        {
            if (Projectile *proj = dynamic_cast<Projectile *>(item))
            {
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
    m_player->setPermanentInvincible(true); // 吸纳时玩家持久无敌
    qDebug() << "[吸纳] 玩家已设置为无敌状态";

    // 添加所有敌人（除了Boss）
    for (QPointer<Enemy> &enemyPtr : m_currentEnemies)
    {
        if (Enemy *enemy = enemyPtr.data())
        {
            if (enemy != boss)
            {
                enemy->pauseTimers();
                m_absorbingItems.append(enemy);
                m_absorbStartPositions.append(enemy->pos());
                m_absorbAngles.append(0);
            }
        }
    }

    // Boss也停止攻击（通过 m_isAbsorbing 标志，已在 enterPhase3 设置）

    // 为每个实体设置初始螺旋角度
    for (int i = 0; i < m_absorbingItems.size(); ++i)
    {
        QPointF itemPos = m_absorbStartPositions[i];
        double dx = itemPos.x() - m_absorbCenter.x();
        double dy = itemPos.y() - m_absorbCenter.y();
        m_absorbAngles[i] = qAtan2(dy, dx);
    }

    m_absorbAnimationStep = 0;

    // 创建吸纳动画定时器
    if (!m_absorbAnimationTimer)
    {
        m_absorbAnimationTimer = new QTimer(this);
        connect(m_absorbAnimationTimer, &QTimer::timeout, this, &Level::onAbsorbAnimationStep);
    }
    m_absorbAnimationTimer->start(16); // 约60fps

    qDebug() << "吸纳动画开始，共" << m_absorbingItems.size() << "个实体";
}

void Level::onAbsorbAnimationStep()
{
    if (!m_isAbsorbAnimationActive)
    {
        m_absorbAnimationTimer->stop();
        return;
    }

    m_absorbAnimationStep++;

    const int totalSteps = 120; // 约2秒完成动画
    double progress = static_cast<double>(m_absorbAnimationStep) / totalSteps;

    if (progress >= 1.0)
    {
        // 动画完成
        m_absorbAnimationTimer->stop();
        m_isAbsorbAnimationActive = false;

        // 清理场景中所有子弹（避免对话期间伤害）
        if (m_scene)
        {
            QList<QGraphicsItem *> allItems = m_scene->items();
            for (QGraphicsItem *item : allItems)
            {
                if (Projectile *proj = dynamic_cast<Projectile *>(item))
                {
                    proj->destroy();
                }
            }
        }

        // 隐藏所有被吸纳的实体（不删除，等待对话后释放）
        for (int i = 0; i < m_absorbingItems.size(); ++i)
        {
            QGraphicsItem *item = m_absorbingItems[i];
            if (!item)
                continue;

            // 隐藏实体
            item->setVisible(false);

            // 如果是敌人，确保其定时器停止
            if (item != m_player)
            {
                Enemy *enemy = dynamic_cast<Enemy *>(item);
                if (enemy)
                {
                    enemy->pauseTimers();
                }
            }
        }

        // 保存吸纳的实体列表供对话后释放使用
        // m_absorbingItems 保持不清空

        qDebug() << "吸纳动画完成，通知Boss";

        // 通知Boss吸纳完成
        if (m_currentWashMachineBoss)
        {
            m_currentWashMachineBoss->onAbsorbComplete();
        }

        return;
    }

    // 使用缓动函数让动画更自然
    double easedProgress = 1 - qPow(1 - progress, 3); // easeOutCubic

    // 更新每个实体的位置（螺旋移动）
    for (int i = 0; i < m_absorbingItems.size(); ++i)
    {
        QGraphicsItem *item = m_absorbingItems[i];
        if (!item)
            continue;

        QPointF startPos = m_absorbStartPositions[i];
        double startAngle = m_absorbAngles[i];

        // 计算当前距离中心的距离（逐渐减小）
        double startDist = qSqrt(qPow(startPos.x() - m_absorbCenter.x(), 2) +
                                 qPow(startPos.y() - m_absorbCenter.y(), 2));
        double currentDist = startDist * (1 - easedProgress);

        // 计算当前角度（螺旋增加）
        double currentAngle = startAngle + easedProgress * 6 * M_PI; // 旋转3圈

        // 计算新位置
        double newX = m_absorbCenter.x() + currentDist * qCos(currentAngle);
        double newY = m_absorbCenter.y() + currentDist * qSin(currentAngle);

        item->setPos(newX, newY);

        // 缩小实体（可选视觉效果）
        if (item != m_player)
        {
            double scale = 1.0 - easedProgress * 0.8;
            item->setScale(scale);
        }
    }
}

// ==================== Boss奖励机制（乌萨奇） ====================

void Level::startBossRewardSequence()
{
    if (m_rewardSequenceActive)
        return;

    m_rewardSequenceActive = true;
    qDebug() << "[Level] 开始Boss奖励流程";

    // 获取当前房间的奖励配置
    QVector<BossRewardItem> rewardItems;
    LevelConfig config;
    if (config.loadFromFile(m_levelNumber))
    {
        const RoomConfig &roomCfg = config.getRoom(m_currentRoomIndex);
        rewardItems = roomCfg.bossRewardItems;
    }

    // 创建乌萨奇（它会自己处理整个奖励流程）
    m_usagi = new Usagi(m_scene, m_player, m_levelNumber, rewardItems, this);

    // 连接信号
    connect(m_usagi, &Usagi::requestShowDialog, this, &Level::onUsagiRequestShowDialog);
    connect(m_usagi, &Usagi::rewardSequenceCompleted, this, &Level::onUsagiRewardCompleted);

    // 启动奖励流程
    m_usagi->startRewardSequence();
}

void Level::onUsagiRequestShowDialog(const QStringList &dialog)
{
    qDebug() << "[Level] 乌萨奇请求显示对话";

    // 显示对话（使用透明背景，保持游戏画面可见）
    showStoryDialog(dialog, false, "transparent");

    // 对话结束后通知乌萨奇
    disconnect(this, &Level::storyFinished, nullptr, nullptr);
    connect(this, &Level::storyFinished, this, [this]()
            {
        if (m_usagi) {
            m_usagi->onDialogFinished();
        } }, Qt::SingleShotConnection);
}

void Level::onUsagiRewardCompleted()
{
    qDebug() << "[Level] 乌萨奇奖励流程完成，打开Boss房门";

    // 打开门
    Room *cur = m_rooms[m_currentRoomIndex];
    if (cur)
    {
        openDoors(cur);
    }

    // 发送关卡完成信号
    emit levelCompleted(m_levelNumber);

    m_rewardSequenceActive = false;
    m_usagi = nullptr;
}

// ==================== TeacherBoss 相关方法 ====================

void Level::connectTeacherBossSignals(TeacherBoss *boss)
{
    if (!boss)
        return;

    m_currentTeacherBoss = boss;

    // 连接请求对话信号
    connect(boss, &TeacherBoss::requestShowDialog,
            this, &Level::onTeacherBossRequestDialog);

    // 连接请求背景切换信号
    connect(boss, &TeacherBoss::requestChangeBackground,
            this, &Level::onTeacherBossRequestChangeBackground);

    // 连接请求显示文字提示信号
    connect(boss, &TeacherBoss::requestShowTransitionText,
            this, &Level::onTeacherBossRequestTransitionText);

    // 连接召唤敌人信号
    connect(boss, &TeacherBoss::requestSpawnEnemies,
            this, &Level::spawnEnemiesForBoss);

    // 设置场景引用
    boss->setScene(m_scene);

    qDebug() << "[Level] TeacherBoss信号已连接";
}

void Level::onTeacherBossRequestDialog(const QStringList &dialogs, const QString &background)
{
    qDebug() << "[Level] TeacherBoss请求显示对话";

    // 暂停所有敌人的定时器
    for (const QPointer<Enemy> &enemyPtr : m_currentEnemies)
    {
        if (enemyPtr)
        {
            enemyPtr->pauseTimers();
        }
    }

    // 显示Boss对话
    showStoryDialog(dialogs, true, background);
}

void Level::onTeacherBossRequestChangeBackground(const QString &backgroundPath)
{
    qDebug() << "[Level] TeacherBoss请求切换背景:" << backgroundPath;
    changeBackground(backgroundPath);
}

void Level::onTeacherBossRequestTransitionText(const QString &text)
{
    qDebug() << "[Level] TeacherBoss请求显示文字:" << text;
    showPhaseTransitionText(text);
}
