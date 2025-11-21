#include "gameview.h"
#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include "../core/resourcefactory.h"
#include "../core/GameWindow.cpp"
#include "../core/audiomanager.h"
#include "level.h"

GameView::GameView(QWidget *parent) : QWidget(parent), player(nullptr), level(nullptr) {
    // 设置窗口大小
    setFixedSize(scene_bound_x, scene_bound_y);
    setFocusPolicy(Qt::StrongFocus);

    // 创建场景
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, scene_bound_x, scene_bound_y);

    // 创建视图
    view = new QGraphicsView(scene, this);
    view->setFixedSize(scene_bound_x, scene_bound_y);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFrameStyle(QFrame::NoFrame);

    // 设置视图更新模式，避免留下轨迹
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view->setCacheMode(QGraphicsView::CacheNone);

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

void GameView::initGame() {
    try {
        // 清理现有内容
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

        // 清理敌人列表（对象会被scene->clear()删除）

        // scene->clear()会自动删除所有图形项（包括player和enemies）
        scene->clear();
        player = nullptr;  // 清空指针引用

        // 背景图片现在由 Level 类负责加载和管理
        // 不再在这里设置背景，避免与 Level 冲突

        // 初始化音频系统
        initAudio();

        // 加载玩家图片
        int playerSize = 60;
        QPixmap playerPixmap = ResourceFactory::createPlayerImage(playerSize);

        // 创建玩家
        player = new Player(playerPixmap, 1.0);
        scene->addItem(player);

        connect(player, &Player::healthChanged, this, &GameView::updateHUD);

        // 设置玩家初始位置（屏幕中央）
        player->setPos(scene_bound_x / 2 - playerSize / 2, scene_bound_y / 2 - playerSize / 2);

        // 设置玩家在场景中的堆叠顺序，确保在最前面
        player->setZValue(100);

        // 设置射击冷却时间
        player->setShootCooldown(150);

        // 创建HUD
        hud = new HUD(player);
        scene->addItem(hud);
        hud->setZValue(9999);
        // 设置地图墙壁
        setupMap(scene);

        // 加载子弹图片
        QPixmap bulletPixmap = ResourceFactory::createBulletImage(50);
        player->setBulletPic(bulletPixmap);

        // 连接玩家死亡信号
        connect(player, &Player::playerDied, this, &GameView::handlePlayerDeath);

        // 连接玩家血量变化信号
        connect(player, &Player::healthChanged, this, &GameView::updateHUD);

        // 连接玩家受伤信号到HUD闪烁
        connect(player, &Player::playerDamaged, hud, &HUD::triggerDamageFlash);

        // 初始更新HUD
        updateHUD();

        // 删除旧的关卡对象
        if (level) {
            delete level;
            level = nullptr;
        }

        // 创建关卡（Level 会负责生成敌人、宝箱、Boss 等）
        level = new Level(player, scene, this);

        // 连接敌人清空信号，显示提示
        connect(level, &Level::enemiesCleared, this, &GameView::onEnemiesCleared);

        level->init(1);

    } catch (const QString &error) {
        QMessageBox::critical(this, "资源加载失败", error);
        emit backToMenu();
    }
}

void GameView::initAudio()
{
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

void GameView::keyPressEvent(QKeyEvent *event) {
    if (!event)
        return;

    // ESC键返回主菜单
    if (event->key() == Qt::Key_Escape) {
        emit backToMenu();
        return;
    }

    // 传递给玩家处理
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
            // 重新开始游戏
            initGame();
        } else if (msgBox.clickedButton() == menuButton) {
            // 返回主菜单
            emit backToMenu();
        } else if (msgBox.clickedButton() == quitButton) {
            // 退出游戏
            QApplication::quit();
        }
    });
}

void GameView::restartGame() {
    // 重新初始化游戏场景
    initGame();
}

void GameView::quitGame() {
    QApplication::quit();
}

void GameView::onEnemiesCleared(int roomIndex) {
    qDebug() << "GameView::onEnemiesCleared 被调用，房间:" << roomIndex;

    // 在场景中显示文字提示
    QGraphicsTextItem *hint = new QGraphicsTextItem("所有敌人已被击败！前往下一个房间的门已打开。");
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
