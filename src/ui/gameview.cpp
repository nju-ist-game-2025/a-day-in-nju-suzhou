#include "gameview.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include "../core/GameWindow.cpp"
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
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
    if (level) {
        delete level;
        level = nullptr;
    }
    if (scene) {
        delete scene;
    }
}

void GameView::setPlayerCharacter(const QString& characterPath) {
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
            playerPixmap = QPixmap(characterPath).scaled(playerSize, playerSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            playerPixmap = ResourceFactory::createPlayerImage(playerSize);
        }

        // 创建玩家
        player = new Player(playerPixmap, 1.0);

        // 创建HUD
        hud = new HUD(player);

        // 设置地图墙壁
        setupMap(scene);

        // 加载子弹图片
        int bulletSize = ConfigManager::instance().getSize("bullet");
        if (bulletSize <= 0)
            bulletSize = 50;  // 默认值
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
            } });

        // 使用开发者设置的起始关卡（默认为1）
        currentLevel = m_startLevel;
        level->init(currentLevel);
        
        // 重置起始关卡为1（下次正常开始游戏时从第1关开始）
        m_startLevel = 1;

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
        player->setShootCooldown(150);
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

    // 3秒后自动移除
    QTimer::singleShot(2000, [levelTextItem, this]() {
        scene->removeItem(levelTextItem);
        delete levelTextItem; });

    // 延迟后进入下一关
    QTimer::singleShot(2000, this, &GameView::advanceToNextLevel);
}

void GameView::advanceToNextLevel() {
    currentLevel++;

    // 检查是否所有关卡都已完成
    if (currentLevel > 3) {
        // 游戏通关
        QMessageBox::information(this, "恭喜", "你已通关所有关卡！");
        emit backToMenu();
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
    AudioManager& audio = AudioManager::instance();

    // 预加载音效
    audio.preloadSound("player_shoot", "assets/sounds/shoot.wav");
    audio.preloadSound("player_death", "assets/sounds/player_death.wav");
    audio.preloadSound("enemy_death", "assets/sounds/enemy_death.wav");
    audio.preloadSound("chest_open", "assets/sounds/chest_open.wav");
    audio.preloadSound("door_open", "assets/sounds/door_open.wav");
    audio.preloadSound("enter_room", "assets/sounds/enter_room.wav");

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
        // 创建自定义对话框
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("游戏结束");
        msgBox.setText("你已死亡！");
        msgBox.setIcon(QMessageBox::Information);

        // 添加三个按钮
        QPushButton *retryButton = msgBox.addButton("再试一次", QMessageBox::ActionRole);
        QPushButton *menuButton = msgBox.addButton("返回主菜单", QMessageBox::ActionRole);
        QPushButton *quitButton = msgBox.addButton("退出游戏", QMessageBox::ActionRole);

        msgBox.exec();

        // 根据用户选择执行相应操作
        if (msgBox.clickedButton() == retryButton) {
            // 请求上层窗口处理重试（避免在 GameView 内部直接重入）
            emit requestRestart();
        } else if (msgBox.clickedButton() == menuButton) {
            // 返回主菜单
            emit backToMenu();
        } else if (msgBox.clickedButton() == quitButton) {
            // 退出游戏
            QApplication::quit();
        } });
}

void GameView::restartGame() {
    // 重新初始化游戏场景
    initGame();
}

void GameView::quitGame() {
    QApplication::quit();
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

    // 3秒后自动消失
    QTimer::singleShot(3000, [this, hint]() {
        if (scene && hint->scene() == scene) {
            scene->removeItem(hint);
            delete hint;
        } });
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

    // 3秒后自动消失
    QTimer::singleShot(3000, [this, hint]() {
        if (scene && hint->scene() == scene) {
            scene->removeItem(hint);
            delete hint;
        } });
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
