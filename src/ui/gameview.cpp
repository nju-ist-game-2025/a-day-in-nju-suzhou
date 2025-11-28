#include "gameview.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QtMath>
#include "../core/GameWindow.cpp"
#include "../core/audiomanager.h"
#include "../core/resourcefactory.h"
#include "../entities/sockenemy.h"
#include "../entities/walker.h"
#include "explosion.h"
#include "level.h"
#include "pausemenu.h"

GameView::GameView(QWidget* parent) : QWidget(parent), player(nullptr), level(nullptr), m_pauseMenu(nullptr), m_isPaused(false), m_playerCharacterPath("assets/player/player.png") {
    // 维持基础可玩尺寸，同时允许继续放大
    setMinimumSize(scene_bound_x, scene_bound_y);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);

    // 创建场景（保持原始逻辑大小）
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, scene_bound_x, scene_bound_y);

    // 创建视图
    view = new QGraphicsView(scene, this);
    view->setMinimumSize(scene_bound_x, scene_bound_y);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFrameStyle(QFrame::NoFrame);

    // 设置视图更新模式，避免留下轨迹
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view->setCacheMode(QGraphicsView::CacheNone);

    // 设置视图背景为黑色（用于填充等比例缩放时的边缘区域）
    view->setBackgroundBrush(QBrush(Qt::black));

    // 设置场景的默认背景为透明，让view的黑色背景透出来
    scene->setBackgroundBrush(Qt::NoBrush);

    // 设置布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view);

    setLayout(layout);
}

GameView::~GameView() {
    cleanupGame();  // 使用统一的清理函数
    if (scene) {
        delete scene;
        scene = nullptr;
    }
}

void GameView::cleanupGame() {
    qDebug() << "cleanupGame: 开始彻底清理游戏状态";

    // ===== 重置所有游戏状态标志 =====
    m_isPaused = false;
    m_isInStoryMode = false;
    isLevelTransition = false;
    currentLevel = 1;

    // ===== 清理暂停菜单 =====
    if (m_pauseMenu) {
        disconnect(m_pauseMenu, nullptr, this, nullptr);
        delete m_pauseMenu;
        m_pauseMenu = nullptr;
    }

    // ===== 清理死亡界面按钮信号（在scene->clear之前断开！） =====
    if (m_retryButton) {
        disconnect(m_retryButton, nullptr, this, nullptr);
        m_retryButton->blockSignals(true);
    }
    if (m_menuButton2) {
        disconnect(m_menuButton2, nullptr, this, nullptr);
        m_menuButton2->blockSignals(true);
    }
    if (m_quitButton2) {
        disconnect(m_quitButton2, nullptr, this, nullptr);
        m_quitButton2->blockSignals(true);
    }

    // ===== 清理胜利界面按钮信号（在scene->clear之前断开！） =====
    if (m_victoryMenuButton) {
        disconnect(m_victoryMenuButton, nullptr, this, nullptr);
        m_victoryMenuButton->blockSignals(true);
    }
    if (m_victoryAgainButton) {
        disconnect(m_victoryAgainButton, nullptr, this, nullptr);
        m_victoryAgainButton->blockSignals(true);
    }
    if (m_victoryQuitButton) {
        disconnect(m_victoryQuitButton, nullptr, this, nullptr);
        m_victoryQuitButton->blockSignals(true);
    }

    // ===== 清理Level（最重要，包含所有游戏实体） =====
    if (level) {
        // 断开所有与 level 相关的信号连接
        disconnect(level, nullptr, this, nullptr);
        disconnect(this, nullptr, level, nullptr);

        // 阻止Level发出新信号
        level->blockSignals(true);

        // 删除Level（Level的析构函数会清理所有房间、敌人等）
        delete level;
        level = nullptr;
    }

    // ===== 清理玩家信号连接 =====
    if (player) {
        disconnect(player, nullptr, this, nullptr);
        // player会被scene->clear()删除，这里只断开信号
    }

    // ===== 清理HUD =====
    if (hud) {
        if (scene && hud->scene() == scene) {
            scene->removeItem(hud);
        }
        delete hud;
        hud = nullptr;
    }

    // ===== 清理地图墙体 =====
    clearMapWalls();

    // ===== 清理场景中的所有对象 =====
    if (scene) {
        scene->clear();
    }

    // ===== 重置所有指针（scene->clear()已删除这些对象） =====
    player = nullptr;

    // 死亡界面相关
    m_deathOverlay = nullptr;
    m_retryButton = nullptr;
    m_menuButton2 = nullptr;
    m_quitButton2 = nullptr;
    m_retryProxy = nullptr;
    m_menuProxy2 = nullptr;
    m_quitProxy2 = nullptr;

    // 胜利界面相关
    m_victoryOverlay = nullptr;
    m_victoryMenuButton = nullptr;
    m_victoryAgainButton = nullptr;
    m_victoryQuitButton = nullptr;

    // ===== 重置开发者模式设置 =====
    m_startLevel = 1;
    m_isDevMode = false;
    m_devSkipToBoss = false;
    m_devMaxHealth = 3;
    m_devBulletDamage = 1;

    // ===== 停止音乐 =====
    AudioManager::instance().stopMusic();

    // ===== 清理静态冷却数据 =====
    PoisonTrail::clearCooldowns();
    SockEnemy::clearAllCooldowns();

    qDebug() << "cleanupGame: 游戏状态清理完成";
}

void GameView::setPlayerCharacter(const QString& characterPath) {
    m_playerCharacterPath = characterPath;
}

void GameView::initGame() {
    try {
        // ===== 保存开发者模式设置（在cleanupGame之前） =====
        int savedStartLevel = m_startLevel;
        bool savedIsDevMode = m_isDevMode;
        int savedDevMaxHealth = m_devMaxHealth;
        int savedDevBulletDamage = m_devBulletDamage;
        bool savedDevSkipToBoss = m_devSkipToBoss;

        // ===== 首先彻底清理旧游戏状态 =====
        cleanupGame();

        // ===== 恢复开发者模式设置 =====
        m_startLevel = savedStartLevel;
        m_isDevMode = savedIsDevMode;
        m_devMaxHealth = savedDevMaxHealth;
        m_devBulletDamage = savedDevBulletDamage;
        m_devSkipToBoss = savedDevSkipToBoss;

        // ===== 重新初始化游戏 =====
        // 预加载爆炸动画帧（只在首次加载）
        if (!Explosion::isFramesLoaded()) {
            Explosion::preloadFrames();
        }

        // 初始化音频系统
        initAudio();

        // 加载玩家图片（优先使用配置文件中的角色，其次使用选定的角色）
        int playerSize = ConfigManager::instance().getSize("player");
        if (playerSize <= 0)
            playerSize = 60;  // 默认值
        QPixmap playerPixmap;

        // 从配置文件获取角色路径
        QString configCharacterPath = ConfigManager::instance().getAssetPath("player");
        QString characterPath = configCharacterPath.isEmpty() ? m_playerCharacterPath : configCharacterPath;

        if (!characterPath.isEmpty() && QFile::exists(characterPath)) {
            playerPixmap = QPixmap(characterPath).scaled(playerSize, playerSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            playerPixmap = ResourceFactory::createPlayerImage(playerSize);
        }

        // 创建玩家
        player = new Player(playerPixmap, 1.0);
        applyCharacterAbility(player, characterPath);

        // 预加载碰撞掩码（避免运行时生成）
        player->preloadCollisionMask();

        // 应用开发者模式设置（如果启用）
        if (m_isDevMode) {
            // 直接设置血量上限（无限制）
            player->setMaxHealth(m_devMaxHealth);
            // 设置子弹伤害
            player->setBulletHurt(m_devBulletDamage);
            qDebug() << "开发者模式: 应用血量上限" << m_devMaxHealth << ", 子弹伤害" << m_devBulletDamage;
        }

        // 创建HUD
        hud = new HUD(player);

        // 设置地图墙壁
        setupMap(scene);

        // 加载子弹图片（使用新的子弹分类配置）
        int bulletSize = ConfigManager::instance().getBulletSize("player");
        if (bulletSize <= 0)
            bulletSize = 20;  // 默认值
        QPixmap bulletPixmap = ResourceFactory::createBulletImage(bulletSize);
        player->setBulletPic(bulletPixmap);

        // 连接玩家死亡信号
        connect(player, &Player::playerDied, this, &GameView::handlePlayerDeath);

        // 连接玩家血量变化信号
        connect(player, &Player::healthChanged, this, &GameView::updateHUD);

        // 连接玩家受伤信号到HUD闪烁
        connect(player, &Player::playerDamaged, hud, &HUD::triggerDamageFlash);

        // 连接黑心复活信号
        connect(player, &Player::blackHeartReviveStarted, this, &GameView::onBlackHeartRevive);

        // 初始更新HUD
        updateHUD();

        // 初始化关卡变量
        currentLevel = 1;  // 从第一关开始
        isLevelTransition = false;

        // 创建关卡
        level = new Level(player, scene, this);

        // 如果是开发者模式且选择直接进入Boss房
        if (m_isDevMode && m_devSkipToBoss) {
            level->setSkipToBoss(true);
        }

        // 连接信号
        connect(level, &Level::enemiesCleared, this, &GameView::onEnemiesCleared);
        connect(level, &Level::bossDoorsOpened, this, &GameView::onBossDoorsOpened);
        connect(level, &Level::levelCompleted, this, &GameView::onLevelCompleted);
        connect(level, &Level::dialogStarted, this, [this]() { m_isInStoryMode = true; });
        connect(level, &Level::dialogFinished, this, [this]() { m_isInStoryMode = false; });
        connect(level, &Level::ticketPickedUp, this, &GameView::onTicketPickedUp);
        connect(player, &Player::playerDamaged, hud, &HUD::triggerDamageFlash);

        // 连接房间进入信号到HUD小地图更新 - 必须在创建level之后
        connect(level, &Level::roomEntered, this, [this](int roomIndex) {
            if (hud) {
                hud->updateMinimap(roomIndex, QVector<int>());
                qDebug() << "GameView: Updating minimap for room" << roomIndex;
            }
        });

        // 使用开发者设置的起始关卡（默认为1）
        currentLevel = m_startLevel;
        level->init(currentLevel);

        // 重置起始关卡为1（下次正常开始游戏时从第1关开始）
        m_startLevel = 1;

        // 重置开发者模式标志（下次正常开始游戏时不应用开发者设置）
        m_isDevMode = false;
        m_devSkipToBoss = false;

        // 初始化小地图
        if (hud)
            hud->updateMinimap(0, QVector<int>());

        connect(level, &Level::storyFinished, this, &GameView::onStoryFinished);

        // 确保初始化后视图立即拉伸到当前窗口大小
        adjustViewToWindow();
    } catch (const QString& error) {
        QMessageBox::critical(this, "资源加载失败", error);
        emit backToMenu();
    }
}

// 实现
void GameView::onStoryFinished() {
    qDebug() << "剧情结束，显示玩家和HUD";

    // 将玩家添加到场景
    if (player && !player->scene()) {
        scene->addItem(player);

        // 设置玩家初始位置（屏幕中央）
        int playerSize = 60;  // 需要与initGame中的一致
        player->setPos(scene_bound_x / 2 - playerSize / 2, scene_bound_y / 2 - playerSize / 2);
        player->setZValue(100);

        // 初始化护盾显示（如果有护盾的话）
        player->updateShieldDisplay();
    }

    // 将HUD添加到场景
    if (hud && !hud->scene()) {
        scene->addItem(hud);
        hud->setZValue(9999);
    }

    // 更新HUD显示
    updateHUD();

    // 可以在这里添加一些入场动画效果
    // showPlayerEntranceAnimation();
}

void GameView::onLevelCompleted() {
    if (isLevelTransition)
        return;
    isLevelTransition = true;

    QGraphicsTextItem* levelTextItem = new QGraphicsTextItem(QString("关卡完成！准备进入下一关..."));
    levelTextItem->setDefaultTextColor(Qt::black);
    levelTextItem->setFont(QFont("Arial", 20, QFont::Bold));
    levelTextItem->setPos(200, 200);
    levelTextItem->setZValue(10000);
    scene->addItem(levelTextItem);
    scene->update();

    // 3秒后自动移除 - 使用QPointer来安全地检查对象是否仍然存在
    QPointer<QGraphicsTextItem> textPtr = levelTextItem;
    QPointer<QGraphicsScene> scenePtr = scene;
    QTimer::singleShot(2000, this, [textPtr, scenePtr]() {
        if (textPtr && scenePtr && textPtr->scene() == scenePtr) {
            scenePtr->removeItem(textPtr);
            delete textPtr;
        }
    });

    // 延迟后进入下一关
    QTimer::singleShot(2000, this, &GameView::advanceToNextLevel);
}

void GameView::showVictoryUI() {
    QRectF rect = scene->sceneRect();
    int W = rect.width();
    int H = rect.height();

    // ====== 半透明遮罩 ======
    m_victoryOverlay = new QGraphicsRectItem(0, 0, W, H);
    m_victoryOverlay->setBrush(QColor(0, 0, 0, 160));
    m_victoryOverlay->setPen(Qt::NoPen);
    m_victoryOverlay->setZValue(30000);
    scene->addItem(m_victoryOverlay);

    // ====== 金色背景板 ======
    int bgW = 420;
    int bgH = 300;
    int bgX = (W - bgW) / 2;
    int bgY = (H - bgH) / 2;

    auto* bg = new QGraphicsRectItem(bgX, bgY, bgW, bgH, m_victoryOverlay);
    bg->setBrush(QColor(60, 45, 10, 220));     // 金棕色
    bg->setPen(QPen(QColor(255, 215, 0), 4));  // 金色边框

    // ====== 金色标题 ======
    QGraphicsTextItem* title = new QGraphicsTextItem("🎉 恭喜通关！🎉", m_victoryOverlay);
    QFont titleFont("Microsoft YaHei", 28, QFont::Bold);
    title->setFont(titleFont);
    title->setDefaultTextColor(QColor(255, 230, 150));  // 柔金色

    qreal tW = title->boundingRect().width();
    title->setPos((W - tW) / 2, bgY + 25);

    // ====== 统一的金色按钮样式 ======
    QString goldButtonStyle =
        "QPushButton {"
        "   background-color: qlineargradient("
        "       x1:0, y1:0, x2:0, y2:1,"
        "       stop:0 #FFD700, stop:1 #E6BE8A"
        "   );"
        "   color: #4a3500;"
        "   border: 2px solid #cfa300;"
        "   border-radius: 10px;"
        "   padding: 8px;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   letter-spacing: 2px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient("
        "       x1:0, y1:0, x2:0, y2:1,"
        "       stop:0 #FFE066, stop:1 #F1C27D"
        "   );"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient("
        "       x1:0, y1:0, x2:0, y2:1,"
        "       stop:0 #E6BE8A, stop:1 #C9A368"
        "   );"
        "}";

    int btnW = 240;
    int btnH = 48;
    int btnX = (W - btnW) / 2;
    int btnY = bgY + 110;
    int spacing = 60;

    // ====== 返回主菜单 ======
    m_victoryMenuButton = new QPushButton("返回主菜单");
    m_victoryMenuButton->setFixedSize(btnW, btnH);
    m_victoryMenuButton->setStyleSheet(goldButtonStyle);

    auto* menuProxy = new QGraphicsProxyWidget(m_victoryOverlay);
    menuProxy->setWidget(m_victoryMenuButton);
    menuProxy->setPos(btnX, btnY);

    // ====== 继续挑战（可选） ======
    m_victoryAgainButton = new QPushButton("再次挑战");
    m_victoryAgainButton->setFixedSize(btnW, btnH);
    m_victoryAgainButton->setStyleSheet(goldButtonStyle);

    auto* againProxy = new QGraphicsProxyWidget(m_victoryOverlay);
    againProxy->setWidget(m_victoryAgainButton);
    againProxy->setPos(btnX, btnY + spacing);

    // ====== 退出游戏 ======
    m_victoryQuitButton = new QPushButton("退出游戏");
    m_victoryQuitButton->setFixedSize(btnW, btnH);
    m_victoryQuitButton->setStyleSheet(goldButtonStyle);

    auto* quitProxy = new QGraphicsProxyWidget(m_victoryOverlay);
    quitProxy->setWidget(m_victoryQuitButton);
    quitProxy->setPos(btnX, btnY + spacing * 2);

    // ====== 信号连接 - 使用延迟确保按钮点击事件完全处理完毕 ======
    connect(m_victoryMenuButton, &QPushButton::clicked, this, [this]() {
        if (m_victoryOverlay) {
            m_victoryOverlay->hide();
        }
        QTimer::singleShot(0, this, [this]() {
            emit backToMenu();
        });
    });

    connect(m_victoryAgainButton, &QPushButton::clicked, this, [this]() {
        if (m_victoryOverlay) {
            m_victoryOverlay->hide();
        }
        QTimer::singleShot(0, this, [this]() {
            emit requestRestart();
        });
    });

    connect(m_victoryQuitButton, &QPushButton::clicked, this, []() {
        QApplication::quit();
    });
}

void GameView::advanceToNextLevel() {
    currentLevel++;

    // 检查是否所有关卡都已完成
    if (currentLevel > 3) {
        // 游戏通关
        showVictoryUI();
        return;
    }

    // 清理当前关卡（保留玩家）
    if (level) {
        // 断开连接，避免重复信号
        disconnect(level, &Level::levelCompleted, this, &GameView::onLevelCompleted);
        disconnect(level, &Level::enemiesCleared, this, &GameView::onEnemiesCleared);
        disconnect(level, &Level::bossDoorsOpened, this, &GameView::onBossDoorsOpened);

        // 清理关卡特定的敌人和物品，但保留玩家
        level->clearCurrentRoomEntities();
    }

    // 重新初始化下一关
    isLevelTransition = false;

    // 初始化新关卡
    if (level) {
        player->setPos(1000, 800);
        level->init(currentLevel);

        connect(level, &Level::storyFinished, this, &GameView::onStoryFinished);

        // 重新连接信号
        connect(level, &Level::levelCompleted, this, &GameView::onLevelCompleted);
        connect(level, &Level::enemiesCleared, this, &GameView::onEnemiesCleared);
        connect(level, &Level::bossDoorsOpened, this, &GameView::onBossDoorsOpened);
        connect(level, &Level::dialogStarted, this, [this]() { m_isInStoryMode = true; });
        connect(level, &Level::dialogFinished, this, [this]() { m_isInStoryMode = false; });
        connect(level, &Level::ticketPickedUp, this, &GameView::onTicketPickedUp);
    }

    // 更新HUD显示当前关卡
    updateHUD();
}

void GameView::initAudio() {
    AudioManager& audio = AudioManager::instance();

    // 预加载音效
    audio.preloadSound("player_shoot", "assets/sounds/shoot.wav");
    audio.preloadSound("player_death", "assets/sounds/player_death.wav");
    audio.preloadSound("enemy_death", "assets/sounds/enemy_death.wav");
    audio.preloadSound("chest_open", "assets/sounds/chest_open.wav");
    audio.preloadSound("door_open", "assets/sounds/door_open.wav");
    audio.preloadSound("enter_room", "assets/sounds/enter_room.wav");
    audio.preloadSound("player_teleport", "assets/sounds/teleport.wav");

    // 播放背景音乐
    audio.playMusic("assets/music/background.mp3");

    qDebug() << "音频系统初始化完成";
}

void GameView::mousePressEvent(QMouseEvent* event) {
    // 剧情模式下，任何鼠标点击都继续对话
    if (level && m_isInStoryMode) {
        level->nextDialog();
        event->accept();  // 标记事件已处理
        return;
    }
}

void GameView::keyPressEvent(QKeyEvent* event) {
    if (!event)
        return;

    // ESC键切换暂停状态
    if (event->key() == Qt::Key_Escape) {
        togglePause();
        return;
    }

    // 如果游戏暂停，不处理其他按键
    if (m_isPaused) {
        return;
    }

    // G键进入下一关（在Boss房间奖励完成后激活）
    if (event->key() == Qt::Key_G && level && level->isGKeyEnabled()) {
        level->triggerNextLevelByGKey();
        return;
    }

    // 检查是否在剧情模式下
    if (level && m_isInStoryMode) {
        // 剧情模式下，空格键或回车键继续对话
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
            level->nextDialog();
            return;  // 事件已处理，不传递给玩家
        }
        return;
    }

    if (!hasFocus()) {
        setFocus();
    }

    // 正常游戏模式：传递给玩家处理
    if (player) {
        player->keyPressEvent(event);
    }
    // 同时传递给当前房间（用于触发切换检测）
    if (level) {
        Room* r = level->currentRoom();
        if (r)
            QCoreApplication::sendEvent(r, event);
    }

    QWidget::keyPressEvent(event);
}

void GameView::keyReleaseEvent(QKeyEvent* event) {
    if (!event)
        return;

    // 传递给玩家处理
    if (player) {
        player->keyReleaseEvent(event);
    }
    // 同时传递给当前房间，更新按键释放状态
    if (level) {
        Room* r = level->currentRoom();
        if (r)
            QCoreApplication::sendEvent(r, event);
    }

    QWidget::keyReleaseEvent(event);
}

void GameView::applyCharacterAbility(Player* player, const QString& characterPath) {
    if (!player)
        return;

    const QString key = resolveCharacterKey(characterPath);
    if (key.isEmpty())
        return;

    if (key == "beautifulGirl") {
        player->setBulletHurt(player->getBulletHurt() * 2);
        // 美少女初始血量减半（使用负数调用addRedContainers）
        int currentMax = static_cast<int>(player->getMaxHealth());
        int reduction = currentMax / 2;
        player->addRedContainers(-reduction);
        // 同时调整当前血量到新的上限
        double newMax = player->getMaxHealth();
        player->setCurrentHealth(newMax);
        qDebug() << "角色加成: 美少女 - 子弹伤害翻倍，初始血量减半 (" << currentMax << " -> " << newMax << ")";
    } else if (key == "HighGracePeople") {
        player->addRedContainers(2);
        player->addRedHearts(2.0);
        player->addShield(2);
        qDebug() << "角色加成: 高雅人士 - 初始血量强化+2护盾";
    } else if (key == "njuFish") {
        player->setSpeed(player->getSpeed() * 1.25);
        player->setshootSpeed(player->getshootSpeed() * 1.2);
        player->setShootCooldown(qMax(80, player->getShootCooldown() - 40));
        qDebug() << "角色加成: 小蓝鲸 - 高机动与射速";
    } else if (key == "quanfuxia") {
        player->addKeys(2);
        player->addBlackHearts(1);
        qDebug() << "角色加成: 权服侠 - 初始资源富足";
    }
}

QString GameView::resolveCharacterKey(const QString& characterPath) const {
    if (characterPath.isEmpty())
        return QString();

    QFileInfo info(characterPath);
    return info.baseName();
}

void GameView::updateHUD() {
    if (!player || !hud)
        return;

    // 获取玩家当前血量
    float currentHealth = player->getCurrentHealth();
    float maxHealth = player->getMaxHealth();

    // 更新HUD显示
    hud->updateHealth(currentHealth, maxHealth);
}

void GameView::handlePlayerDeath() {
    // 让 Level 处理敌人状态切换（Level::onPlayerDied 会被调用下方）

    // 断开信号连接，避免重复触发
    if (player) {
        disconnect(player, &Player::playerDied, this, &GameView::handlePlayerDeath);
    }

    // 强制更新HUD显示血量为0
    if (hud && player) {
        hud->updateHealth(0, player->getMaxHealth());
    }

    // 通知 Level 玩家已死亡，以便 Level 能让所有敌人失去玩家引用
    if (level) {
        level->onPlayerDied();
    }

    // 使用 QTimer::singleShot 延迟显示对话框
    QTimer::singleShot(100, this, [this]() {
        QRectF rect = scene->sceneRect();
        int sceneW = rect.width();
        int sceneH = rect.height();

        // ====== 半透明遮罩 ======
        if (!m_deathOverlay) {
            m_deathOverlay = new QGraphicsRectItem(0, 0, sceneW, sceneH);
            m_deathOverlay->setBrush(QBrush(QColor(0, 0, 0, 150)));
            m_deathOverlay->setPen(Qt::NoPen);
            m_deathOverlay->setZValue(20000);
            scene->addItem(m_deathOverlay);
        } else {
            m_deathOverlay->setRect(0, 0, sceneW, sceneH);
            m_deathOverlay->show();
        }

        // ====== 中心背景板 ======
        int bgWidth = 300;
        int bgHeight = 280;
        int bgX = (sceneW - bgWidth) / 2;
        int bgY = (sceneH - bgHeight) / 2;

        auto* bg = new QGraphicsRectItem(bgX, bgY, bgWidth, bgHeight, m_deathOverlay);
        bg->setBrush(QBrush(QColor(50, 50, 50, 230)));
        bg->setPen(QPen(QColor(100, 100, 100), 3));

        // ====== 标题 ======
        QGraphicsTextItem* title = new QGraphicsTextItem("你死了", m_deathOverlay);
        QFont titleFont("Microsoft YaHei", 24, QFont::Bold);
        title->setFont(titleFont);
        title->setDefaultTextColor(Qt::white);

        qreal titleW = title->boundingRect().width();
        title->setPos((sceneW - titleW) / 2, bgY + 20);

        // ====== 统一按钮样式（渐变 + 圆角 + 粗体） ======
        QString retryButtonStyle =
            "QPushButton {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4CAF50, stop:1 #388E3C);"
            "   color: white;"
            "   border: 2px solid #2E7D32;"
            "   border-radius: 8px;"
            "   padding: 8px;"
            "   font-family: 'Microsoft YaHei';"
            "   font-size: 14px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66BB6A, stop:1 #43A047);"
            "}"
            "QPushButton:pressed {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #388E3C, stop:1 #2E7D32);"
            "}";

        QString menuButtonStyle =
            "QPushButton {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2196F3, stop:1 #1976D2);"
            "   color: white;"
            "   border: 2px solid #1565C0;"
            "   border-radius: 8px;"
            "   padding: 8px;"
            "   font-family: 'Microsoft YaHei';"
            "   font-size: 14px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #42A5F5, stop:1 #1E88E5);"
            "}"
            "QPushButton:pressed {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1976D2, stop:1 #1565C0);"
            "}";

        QString quitButtonStyle =
            "QPushButton {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f44336, stop:1 #d32f2f);"
            "   color: white;"
            "   border: 2px solid #c62828;"
            "   border-radius: 8px;"
            "   padding: 8px;"
            "   font-family: 'Microsoft YaHei';"
            "   font-size: 14px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ef5350, stop:1 #e53935);"
            "}"
            "QPushButton:pressed {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #d32f2f, stop:1 #c62828);"
            "}";

        // ====== 按钮布局 ======
        int buttonW = 200;
        int buttonH = 45;
        int buttonX = (sceneW - buttonW) / 2;
        int buttonStartY = bgY + 80;
        int spacing = 55;

        // 再试一次
        m_retryButton = new QPushButton("再试一次");
        m_retryButton->setFixedSize(buttonW, buttonH);
        m_retryButton->setStyleSheet(retryButtonStyle);
        m_retryProxy = new QGraphicsProxyWidget(m_deathOverlay);
        m_retryProxy->setWidget(m_retryButton);
        m_retryProxy->setPos(buttonX, buttonStartY);

        // 返回主菜单
        m_menuButton2 = new QPushButton("返回主菜单");
        m_menuButton2->setFixedSize(buttonW, buttonH);
        m_menuButton2->setStyleSheet(menuButtonStyle);
        m_menuProxy2 = new QGraphicsProxyWidget(m_deathOverlay);
        m_menuProxy2->setWidget(m_menuButton2);
        m_menuProxy2->setPos(buttonX, buttonStartY + spacing);

        // 退出游戏
        m_quitButton2 = new QPushButton("退出游戏");
        m_quitButton2->setFixedSize(buttonW, buttonH);
        m_quitButton2->setStyleSheet(quitButtonStyle);
        m_quitProxy2 = new QGraphicsProxyWidget(m_deathOverlay);
        m_quitProxy2->setWidget(m_quitButton2);
        m_quitProxy2->setPos(buttonX, buttonStartY + spacing * 2);

        // 信号连接 - 使用 QueuedConnection 确保在事件处理完成后才执行槽函数
        connect(m_retryButton, &QPushButton::clicked, this, [this]() {
            if (m_deathOverlay) {
                m_deathOverlay->hide();
            }
            // 使用 QTimer::singleShot 延迟发出信号，确保按钮点击事件完全处理完毕
            QTimer::singleShot(0, this, [this]() {
                emit requestRestart();
            });
        });

        connect(m_menuButton2, &QPushButton::clicked, this, [this]() {
            if (m_deathOverlay) {
                m_deathOverlay->hide();
            }
            // 使用 QTimer::singleShot 延迟发出信号，确保按钮点击事件完全处理完毕
            QTimer::singleShot(0, this, [this]() {
                emit backToMenu();
            });
        });

        connect(m_quitButton2, &QPushButton::clicked, this, []() {
            QApplication::quit();
        });
    });
}

void GameView::onEnemiesCleared(int roomIndex, bool up, bool down, bool left, bool right) {
    qDebug() << "GameView::onEnemiesCleared 被调用，房间:" << roomIndex;

    // 在场景中显示文字提示
    QString text = "所有敌人已被击败！";
    if (up || down || left || right)
        text += QString("前往 ");
    if (up)
        text += QString("上方 ");
    if (down)
        text += QString("下方 ");
    if (left)
        text += QString("左侧 ");
    if (right)
        text += QString("右侧 ");
    if (up || down || left || right)
        text += QString("房间的门已打开");
    QGraphicsTextItem* hint = new QGraphicsTextItem(text);
    hint->setDefaultTextColor(Qt::red);
    hint->setFont(QFont("Arial", 16, QFont::Bold));
    hint->setPos(150, 250);
    hint->setZValue(1000);  // 确保在最上层
    scene->addItem(hint);

    // 3秒后自动消失 - 使用QPointer来安全地检查对象是否仍然存在
    QPointer<QGraphicsTextItem> hintPtr = hint;
    QPointer<QGraphicsScene> scenePtr = scene;
    QTimer::singleShot(3000, this, [scenePtr, hintPtr]() {
        if (scenePtr && hintPtr && hintPtr->scene() == scenePtr) {
            scenePtr->removeItem(hintPtr);
            delete hintPtr;
        }
    });
}

void GameView::onBossDoorsOpened() {
    qDebug() << "GameView::onBossDoorsOpened 被调用";

    // 在战斗房间文案下一行显示boss门开启提示（深紫色）
    QString text = "所有普通房间已肃清！boss房间开启，祝你好运";
    QGraphicsTextItem* hint = new QGraphicsTextItem(text);
    hint->setDefaultTextColor(QColor(75, 0, 130));  // 深紫色
    hint->setFont(QFont("Arial", 16, QFont::Bold));
    hint->setPos(150, 280);  // 在战斗文案（y=250）下方30像素
    hint->setZValue(1000);   // 确保在最上层
    scene->addItem(hint);

    // 3秒后自动消失 - 使用QPointer来安全地检查对象是否仍然存在
    QPointer<QGraphicsTextItem> hintPtr = hint;
    QPointer<QGraphicsScene> scenePtr = scene;
    QTimer::singleShot(3000, this, [scenePtr, hintPtr]() {
        if (scenePtr && hintPtr && hintPtr->scene() == scenePtr) {
            scenePtr->removeItem(hintPtr);
            delete hintPtr;
        }
    });
}

void GameView::togglePause() {
    if (m_isPaused) {
        resumeGame();
    } else {
        pauseGame();
    }
}

void GameView::pauseGame() {
    if (m_isPaused)
        return;

    m_isPaused = true;

    // 创建暂停菜单（如果还没有）
    if (!m_pauseMenu) {
        m_pauseMenu = new PauseMenu(scene, this);
        connect(m_pauseMenu, &PauseMenu::resumeGame, this, &GameView::resumeGame);
        connect(m_pauseMenu, &PauseMenu::returnToMenu, this, [this]() {
            // 返回主菜单前，重置暂停状态
            m_isPaused = false;
            if (m_pauseMenu) {
                m_pauseMenu->hide();
            }
            emit backToMenu();
        });
    }

    // 暂停玩家
    if (player) {
        player->setPaused(true);
    }

    // 暂停关卡（敌人等）
    if (level) {
        level->setPaused(true);
    }

    // 显示暂停菜单
    m_pauseMenu->show();
}

void GameView::resumeGame() {
    if (!m_isPaused)
        return;

    m_isPaused = false;

    // 隐藏暂停菜单
    if (m_pauseMenu) {
        m_pauseMenu->hide();
    }

    // 恢复玩家
    if (player) {
        player->setPaused(false);
    }

    // 恢复关卡（敌人等）
    if (level) {
        level->setPaused(false);
    }

    // 确保游戏视图获得焦点
    setFocus();
}

void GameView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    adjustViewToWindow();
}

void GameView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    adjustViewToWindow();
}

void GameView::adjustViewToWindow() {
    if (!view || !scene)
        return;

    QRectF rect = scene->sceneRect();
    if (rect.isNull())
        return;

    view->fitInView(rect, Qt::KeepAspectRatio);
    view->centerOn(rect.center());
}

void GameView::onBlackHeartRevive() {
    qDebug() << "GameView: 黑心复活动画触发";

    // 暂停游戏
    if (level) {
        level->setPaused(true);
    }
    if (player) {
        player->setPaused(true);
    }

    // 创建半透明黑色背景遮罩
    QGraphicsRectItem* overlay = new QGraphicsRectItem(scene->sceneRect());
    overlay->setBrush(QColor(0, 0, 0, 150));
    overlay->setPen(Qt::NoPen);
    overlay->setZValue(2000);
    scene->addItem(overlay);

    // 加载黑心图片
    QPixmap blackHeartPix("assets/props/black_heart.png");
    if (blackHeartPix.isNull()) {
        qWarning() << "无法加载黑心图片";
        // 如果图片加载失败，直接结束动画
        scene->removeItem(overlay);
        delete overlay;
        if (level)
            level->setPaused(false);
        if (player)
            player->setPaused(false);
        updateHUD();
        return;
    }

    // 缩放黑心图片到较大尺寸（100x100）
    int heartSize = 100;
    blackHeartPix = blackHeartPix.scaled(heartSize, heartSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 创建黑心图片项，放在屏幕中央
    QGraphicsPixmapItem* blackHeart = new QGraphicsPixmapItem(blackHeartPix);
    blackHeart->setZValue(2001);
    QPointF centerPos(400 - heartSize / 2, 300 - heartSize / 2);
    blackHeart->setPos(centerPos);
    blackHeart->setTransformOriginPoint(heartSize / 2, heartSize / 2);
    scene->addItem(blackHeart);

    // 加载红心图片
    QPixmap redHeartPix("assets/props/red_heart.png");
    if (redHeartPix.isNull()) {
        redHeartPix = QPixmap(heartSize, heartSize);
        redHeartPix.fill(Qt::red);
    } else {
        redHeartPix = redHeartPix.scaled(heartSize, heartSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // 血条位置（HUD中血条的大致位置）
    QPointF healthBarPos(150, 22);

    // 动画时间线
    int flashDuration = 150;  // 每次闪烁持续时间
    int flashCount = 3;       // 闪烁次数
    int moveDuration = 800;   // 移动动画持续时间

    // 第一阶段：黑心闪烁
    for (int i = 0; i < flashCount; ++i) {
        int showTime = i * flashDuration * 2;
        int hideTime = showTime + flashDuration;

        QTimer::singleShot(showTime, this, [blackHeart, this]() {
            if (blackHeart && scene->items().contains(blackHeart)) {
                blackHeart->setOpacity(1.0);
            }
        });
        QTimer::singleShot(hideTime, this, [blackHeart, this]() {
            if (blackHeart && scene->items().contains(blackHeart)) {
                blackHeart->setOpacity(0.3);
            }
        });
    }

    // 第二阶段：黑心变成红心（闪烁结束后）
    int transformTime = flashCount * flashDuration * 2;
    QTimer::singleShot(transformTime, this, [this, blackHeart, redHeartPix, heartSize]() {
        if (blackHeart && scene->items().contains(blackHeart)) {
            blackHeart->setPixmap(redHeartPix);
            blackHeart->setOpacity(1.0);
            qDebug() << "黑心变成红心";
        }
    });

    // 第三阶段：红心缩小并移向血条
    int moveStartTime = transformTime + 300;
    QTimer::singleShot(moveStartTime, this, [this, blackHeart, healthBarPos, heartSize, moveDuration, overlay]() {
        if (!blackHeart || !scene->items().contains(blackHeart)) {
            return;
        }

        // 使用定时器实现平滑移动动画
        QTimer* animTimer = new QTimer(this);
        int* step = new int(0);
        int totalSteps = moveDuration / 16;  // 约60fps
        QPointF startPos = blackHeart->pos();
        double startScale = 1.0;
        double endScale = 0.3;

        connect(animTimer, &QTimer::timeout, this, [this, animTimer, step, totalSteps, blackHeart, startPos, healthBarPos, heartSize, startScale, endScale, overlay]() {
            (*step)++;
            double progress = static_cast<double>(*step) / totalSteps;

            if (progress >= 1.0 || !blackHeart || !scene->items().contains(blackHeart)) {
                animTimer->stop();
                animTimer->deleteLater();
                delete step;

                // 动画结束，清理
                if (blackHeart && scene->items().contains(blackHeart)) {
                    scene->removeItem(blackHeart);
                    delete blackHeart;
                }
                if (overlay && scene->items().contains(overlay)) {
                    scene->removeItem(overlay);
                    delete overlay;
                }

                // 恢复游戏
                if (level)
                    level->setPaused(false);
                if (player)
                    player->setPaused(false);

                // 更新HUD
                updateHUD();

                qDebug() << "黑心复活动画完成";
                return;
            }

            // 使用缓动函数让动画更自然
            double easedProgress = 1.0 - qPow(1.0 - progress, 3);  // easeOutCubic

            // 计算当前位置
            double newX = startPos.x() + (healthBarPos.x() - startPos.x()) * easedProgress;
            double newY = startPos.y() + (healthBarPos.y() - startPos.y()) * easedProgress;
            blackHeart->setPos(newX, newY);

            // 计算当前缩放
            double currentScale = startScale + (endScale - startScale) * easedProgress;
            blackHeart->setScale(currentScale);
        });

        animTimer->start(16);
    });
}

void GameView::onTicketPickedUp() {
    qDebug() << "GameView: 收到车票拾取信号，开始通关动画";

    // 暂停游戏
    if (level)
        level->setPaused(true);
    if (player)
        player->setPaused(true);

    QRectF rect = scene->sceneRect();
    int W = rect.width();
    int H = rect.height();

    // 创建半透明遮罩
    QGraphicsRectItem* overlay = new QGraphicsRectItem(0, 0, W, H);
    overlay->setBrush(QColor(0, 0, 0, 0));  // 初始透明
    overlay->setPen(Qt::NoPen);
    overlay->setZValue(29000);
    scene->addItem(overlay);

    // 加载车票图片
    QPixmap ticketPix("assets/items/ticket.png");
    if (ticketPix.isNull()) {
        ticketPix = QPixmap(100, 60);
        ticketPix.fill(QColor(255, 182, 193));  // 粉色占位符
    }

    QGraphicsPixmapItem* ticket = new QGraphicsPixmapItem(ticketPix);
    ticket->setZValue(30000);
    // 设置变换原点为图片中心
    ticket->setTransformOriginPoint(ticketPix.width() / 2.0, ticketPix.height() / 2.0);
    ticket->setScale(0.1);
    // 初始位置：让图片中心对齐屏幕中心
    ticket->setPos(W / 2.0 - ticketPix.width() / 2.0, H / 2.0 - ticketPix.height() / 2.0);
    scene->addItem(ticket);

    // 动画参数
    int animDuration = 5000;
    int totalSteps = animDuration / 16;
    int* step = new int(0);

    QTimer* animTimer = new QTimer(this);
    connect(animTimer, &QTimer::timeout, this, [this, animTimer, step, totalSteps, ticket, overlay, ticketPix, W, H]() {
        (*step)++;
        double progress = static_cast<double>(*step) / totalSteps;

        if (progress >= 1.0) {
            animTimer->stop();
            animTimer->deleteLater();
            delete step;

            // 动画完成，显示通关界面
            if (ticket && scene->items().contains(ticket)) {
                scene->removeItem(ticket);
                delete ticket;
            }
            if (overlay && scene->items().contains(overlay)) {
                scene->removeItem(overlay);
                delete overlay;
            }

            // 显示胜利界面
            showVictoryUI();
            return;
        }

        // 背景渐暗（前30%时间内完成）
        double overlayProgress = qMin(1.0, progress / 0.3);
        int alpha = static_cast<int>(180 * overlayProgress);
        overlay->setBrush(QColor(0, 0, 0, alpha));

        // 车票放大动画（使用缓动函数，更平滑）
        double easedProgress = 1.0 - qPow(1.0 - progress, 3);  // easeOutCubic
        double targetScale = 0.8;
        double currentScale = 0.1 + (targetScale - 0.1) * easedProgress;
        ticket->setScale(currentScale);
        // 位置不需要更新，因为TransformOriginPoint已设置为中心，缩放会围绕中心进行
    });

    animTimer->start(16);
}
