#include "gameview.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include "../core/GameWindow.cpp"
#include "../core/audiomanager.h"
#include "../core/resourcefactory.h"
#include "explosion.h"
#include "level.h"
#include "pausemenu.h"

GameView::GameView(QWidget *parent) : QWidget(parent), player(nullptr), level(nullptr), m_pauseMenu(nullptr),
                                      m_isPaused(false), m_playerCharacterPath("assets/player/player.png") {
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
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view);

    setLayout(layout);
}

GameView::~GameView() {
    if (level) {
        delete level;
        level = nullptr;
    }
    if (scene) {
        delete scene;
    }
}

void GameView::setPlayerCharacter(const QString &characterPath) {
    m_playerCharacterPath = characterPath;
}

void GameView::initGame() {
    try {
        // ===== 重置暂停状态 =====
        m_isPaused = false;

        // 清理暂停菜单（它的元素在scene->clear()时会被删除，所以需要重新创建）
        if (m_pauseMenu) {
            // 断开信号连接
            disconnect(m_pauseMenu, nullptr, this, nullptr);
            delete m_pauseMenu;
            m_pauseMenu = nullptr;
        }

        // ===== 第一步：删除旧Level（让Level自己清理场景对象） =====
        if (level) {
            // 断开所有与 level 相关的信号连接
            disconnect(level, nullptr, this, nullptr);
            disconnect(this, nullptr, level, nullptr);

            // 阻止Level发出新信号
            level->blockSignals(true);

            // 立即删除（不使用deleteLater，因为需要在clear场景前完成清理）
            delete level;
            level = nullptr;
        }

        // ===== 第二步：清理场景和UI =====
        // 先断开信号连接
        if (player) {
            disconnect(player, &Player::playerDied, this, &GameView::handlePlayerDeath);
        }

        // 清理HUD
        if (hud) {
            scene->removeItem(hud);
            delete hud;
            hud = nullptr;
        }

        // 在清空场景前，先移除全局地图中的墙体，避免重复释放
        clearMapWalls();
        // scene->clear()会自动删除所有图形项（包括player和enemies）
        scene->clear();
        player = nullptr;  // 清空指针引用

        // ===== 第三步：重新初始化游戏 =====
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
            playerPixmap = QPixmap(characterPath).scaled(playerSize, playerSize, Qt::KeepAspectRatio,
                                                         Qt::SmoothTransformation);
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
    } catch (const QString &error) {
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

    QGraphicsTextItem *levelTextItem = new QGraphicsTextItem(QString("关卡完成！准备进入下一关..."));
    levelTextItem->setDefaultTextColor(Qt::black);
    levelTextItem->setFont(QFont("Arial", 20, QFont::Bold));
    levelTextItem->setPos(200, 200);
    levelTextItem->setZValue(10000);
    scene->addItem(levelTextItem);
    scene->update();

    // 3秒后自动移除
    QTimer::singleShot(2000, [levelTextItem, this]() {
        scene->removeItem(levelTextItem);
        delete levelTextItem;
    });

    // 延迟后进入下一关
    QTimer::singleShot(2000, this, &GameView::advanceToNextLevel);
}

void GameView::showVictoryUI()
{
    QRectF rect = scene->sceneRect();
    int W = rect.width();
    int H = rect.height();

    // ====== 半透明遮罩 ======
    auto *overlay = new QGraphicsRectItem(0, 0, W, H);
    overlay->setBrush(QColor(0, 0, 0, 160));
    overlay->setPen(Qt::NoPen);
    overlay->setZValue(30000);
    scene->addItem(overlay);

    // ====== 金色背景板 ======
    int bgW = 420;
    int bgH = 300;
    int bgX = (W - bgW) / 2;
    int bgY = (H - bgH) / 2;

    auto *bg = new QGraphicsRectItem(bgX, bgY, bgW, bgH, overlay);
    bg->setBrush(QColor(60, 45, 10, 220));  // 金棕色
    bg->setPen(QPen(QColor(255, 215, 0), 4)); // 金色边框


    // ====== 金色标题 ======
    QGraphicsTextItem *title = new QGraphicsTextItem("🎉 恭喜通关！🎉", overlay);
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
    QPushButton *menuBtn = new QPushButton("返回主菜单");
    menuBtn->setFixedSize(btnW, btnH);
    menuBtn->setStyleSheet(goldButtonStyle);

    auto *menuProxy = new QGraphicsProxyWidget(overlay);
    menuProxy->setWidget(menuBtn);
    menuProxy->setPos(btnX, btnY);

    // ====== 继续挑战（可选） ======
    QPushButton *againBtn = new QPushButton("再次挑战");
    againBtn->setFixedSize(btnW, btnH);
    againBtn->setStyleSheet(goldButtonStyle);

    auto *againProxy = new QGraphicsProxyWidget(overlay);
    againProxy->setWidget(againBtn);
    againProxy->setPos(btnX, btnY + spacing);

    // ====== 退出游戏 ======
    QPushButton *quitBtn = new QPushButton("退出游戏");
    quitBtn->setFixedSize(btnW, btnH);
    quitBtn->setStyleSheet(goldButtonStyle);

    auto *quitProxy = new QGraphicsProxyWidget(overlay);
    quitProxy->setWidget(quitBtn);
    quitProxy->setPos(btnX, btnY + spacing * 2);

    // ====== 信号连接 ======
    connect(menuBtn, &QPushButton::clicked, this, [this, overlay]() {
        overlay->hide();
        emit backToMenu();
    });

    connect(againBtn, &QPushButton::clicked, this, [this, overlay]() {
        overlay->hide();
        emit requestRestart();
    });

    connect(quitBtn, &QPushButton::clicked, this, []() {
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
    }

    // 更新HUD显示当前关卡
    updateHUD();
}

void GameView::initAudio() {
    AudioManager &audio = AudioManager::instance();

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

void GameView::mousePressEvent(QMouseEvent *event) {
    // 剧情模式下，任何鼠标点击都继续对话
    if (level && m_isInStoryMode) {
        level->nextDialog();
        event->accept();  // 标记事件已处理
        return;
    }
}

void GameView::keyPressEvent(QKeyEvent *event) {
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
        Room *r = level->currentRoom();
        if (r)
            QCoreApplication::sendEvent(r, event);
    }

    QWidget::keyPressEvent(event);
}

void GameView::keyReleaseEvent(QKeyEvent *event) {
    if (!event)
        return;

    // 传递给玩家处理
    if (player) {
        player->keyReleaseEvent(event);
    }
    // 同时传递给当前房间，更新按键释放状态
    if (level) {
        Room *r = level->currentRoom();
        if (r)
            QCoreApplication::sendEvent(r, event);
    }

    QWidget::keyReleaseEvent(event);
}

void GameView::applyCharacterAbility(Player *player, const QString &characterPath) {
    if (!player)
        return;

    const QString key = resolveCharacterKey(characterPath);
    if (key.isEmpty())
        return;

    if (key == "beautifulGirl") {
        player->setBulletHurt(player->getBulletHurt() * 2);
        qDebug() << "角色加成: 美少女 - 子弹伤害翻倍";
    } else if (key == "HighGracePeople") {
        player->addRedContainers(2);
        player->addRedHearts(2.0);
        player->addSoulHearts(2);
        qDebug() << "角色加成: 高雅人士 - 初始血量强化";
    } else if (key == "njuFish") {
        player->setSpeed(player->getSpeed() * 1.25);
        player->setshootSpeed(player->getshootSpeed() * 1.2);
        player->setShootCooldown(qMax(80, player->getShootCooldown() - 40));
        qDebug() << "角色加成: 小蓝鲸 - 高机动与射速";
    } else if (key == "quanfuxia") {
        player->addBombs(2);
        player->addKeys(2);
        player->addBlackHearts(1);
        qDebug() << "角色加成: 权服侠 - 初始资源富足";
    }
}

QString GameView::resolveCharacterKey(const QString &characterPath) const {
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

        auto *bg = new QGraphicsRectItem(bgX, bgY, bgWidth, bgHeight, m_deathOverlay);
        bg->setBrush(QBrush(QColor(50, 50, 50, 230)));
        bg->setPen(QPen(QColor(100, 100, 100), 3));

        // ====== 标题 ======
        QGraphicsTextItem *title = new QGraphicsTextItem("你死了", m_deathOverlay);
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

        // 信号连接
        connect(m_retryButton, &QPushButton::clicked, this, [this]() {
            m_deathOverlay->hide();
            emit requestRestart();
        });

        connect(m_menuButton2, &QPushButton::clicked, this, [this]() {
            m_deathOverlay->hide();
            emit backToMenu();
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
    QGraphicsTextItem *hint = new QGraphicsTextItem(text);
    hint->setDefaultTextColor(Qt::red);
    hint->setFont(QFont("Arial", 16, QFont::Bold));
    hint->setPos(150, 250);
    hint->setZValue(1000);  // 确保在最上层
    scene->addItem(hint);

    // 3秒后自动消失
    QTimer::singleShot(3000, [this, hint]() {
        if (scene && hint->scene() == scene) {
            scene->removeItem(hint);
            delete hint;
        }
    });
}

void GameView::onBossDoorsOpened() {
    qDebug() << "GameView::onBossDoorsOpened 被调用";

    // 在战斗房间文案下一行显示boss门开启提示（深紫色）
    QString text = "所有普通房间已肃清！boss房间开启，祝你好运";
    QGraphicsTextItem *hint = new QGraphicsTextItem(text);
    hint->setDefaultTextColor(QColor(75, 0, 130));  // 深紫色
    hint->setFont(QFont("Arial", 16, QFont::Bold));
    hint->setPos(150, 280);  // 在战斗文案（y=250）下方30像素
    hint->setZValue(1000);   // 确保在最上层
    scene->addItem(hint);

    // 3秒后自动消失
    QTimer::singleShot(3000, [this, hint]() {
        if (scene && hint->scene() == scene) {
            scene->removeItem(hint);
            delete hint;
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

void GameView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    adjustViewToWindow();
}

void GameView::resizeEvent(QResizeEvent *event) {
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
