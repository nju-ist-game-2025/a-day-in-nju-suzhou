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

        // 创建HUD
        hud = new HUD(player);

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

        // 初始化关卡变量
        currentLevel = 1;  // 从第一关开始
        isLevelTransition = false;

        // 创建关卡
        level = new Level(player, scene, this);

        // 连接信号
        connect(level, &Level::enemiesCleared, this, &GameView::onEnemiesCleared);
        connect(level, &Level::levelCompleted, this, &GameView::onLevelCompleted);  // 新增连接
        connect(player, &Player::playerDamaged, hud, &HUD::triggerDamageFlash);

        level->init(currentLevel);

        m_isInStoryMode = true;
        connect(level, &Level::storyFinished, this, [this]() {
            m_isInStoryMode = false;
            onStoryFinished();
        });

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
        int playerSize = 60; // 需要与initGame中的一致
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
    //showPlayerEntranceAnimation();
}

void GameView::onLevelCompleted() {
    if (isLevelTransition) return;
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

        // 清理关卡特定的敌人和物品，但保留玩家
        level->clearCurrentRoomEntities();
    }

    // 重新初始化下一关
    isLevelTransition = false;

    // 初始化新关卡
    if (level) {
        player->setPos(1000, 800);
        level->init(currentLevel);

        m_isInStoryMode = true;
        connect(level, &Level::storyFinished, this, [this]() {
            m_isInStoryMode = false;
            onStoryFinished();
        });

        // 重新连接信号
        connect(level, &Level::levelCompleted, this, &GameView::onLevelCompleted);
        connect(level, &Level::enemiesCleared, this, &GameView::onEnemiesCleared);
    }

    // 更新HUD显示当前关卡
    updateHUD();
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

void GameView::mousePressEvent(QMouseEvent* event) {
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

    // ESC键返回主菜单（在任何模式下都有效）
    if (event->key() == Qt::Key_Escape) {
        emit backToMenu();
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

void GameView::onEnemiesCleared(int roomIndex, bool up, bool down, bool left, bool right) {
    qDebug() << "GameView::onEnemiesCleared 被调用，房间:" << roomIndex;

    // 在场景中显示文字提示
    QString text = "所有敌人已被击败！";
    if(up || down || left || right) text += QString("前往 ");
    if(up) text += QString("上方 ");
    if(down) text += QString("下方 ");
    if(left) text += QString("左侧 ");
    if(right) text += QString("右侧 ");
    if(up || down || left || right) text += QString("房间的门已打开");
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
