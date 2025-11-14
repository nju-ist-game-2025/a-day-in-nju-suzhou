#include "gameview.h"
#include <QApplication>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include "../constants.h"
#include "../core/resourcefactory.h"
#include "../entities/enemy.h"

GameView::GameView(QWidget* parent) : QWidget(parent), player(nullptr) {
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
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view);

    setLayout(layout);
}

GameView::~GameView() {
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

        // 清理敌人列表（对象会被scene->clear()删除）
        enemies.clear();

        // scene->clear()会自动删除所有图形项（包括player和enemies）
        scene->clear();
        player = nullptr;  // 清空指针引用

        // 加载游戏背景图片
        QPixmap backgroundPixmap = ResourceFactory::loadBackgroundImage("assets/background_game.png", scene_bound_x, scene_bound_y);
        scene->setBackgroundBrush(QBrush(backgroundPixmap));

        // 加载玩家图片
        int playerSize = 60;
        QPixmap playerPixmap = ResourceFactory::createPlayerImage(playerSize, "assets/player.png");

        // 创建玩家
        player = new Player(playerPixmap, 1.0);
        scene->addItem(player);

        // 设置玩家初始位置（屏幕中央）
        player->setPos(scene_bound_x / 2 - playerSize / 2, scene_bound_y / 2 - playerSize / 2);

        // 设置玩家在场景中的堆叠顺序，确保在最前面
        player->setZValue(100);

        // 设置射击冷却时间
        player->setShootCooldown(150);

        // 加载子弹图片
        QPixmap bulletPixmap = ResourceFactory::createBulletImage(50, "assets/bullet.png");
        player->setBulletPic(bulletPixmap);

        // 连接玩家死亡信号
        connect(player, &Player::playerDied, this, &GameView::handlePlayerDeath);

        // 生成敌人
        spawnEnemies();

    } catch (const QString& error) {
        QMessageBox::critical(this, "资源加载失败", error);
        emit backToMenu();
    }
}

void GameView::spawnEnemies() {
    // 使用 Qt6 的 QRandomGenerator 生成随机数
    // 加载敌人图片
    int enemySize = 40;
    QPixmap enemyPixmap = ResourceFactory::createEnemyImage(enemySize, "assets/enemy.png");

    // 生成5只敌人在随机位置
    for (int i = 0; i < 5; i++) {
        // 生成随机位置（避开玩家初始位置中心区域）
        double x, y;
        do {
            x = 50 + QRandomGenerator::global()->bounded(scene_bound_x - 100);
            y = 50 + QRandomGenerator::global()->bounded(scene_bound_y - 100);

            // 确保不会太靠近玩家初始位置
            double playerX = scene_bound_x / 2;
            double playerY = scene_bound_y / 2;
            double dist = qSqrt((x - playerX) * (x - playerX) + (y - playerY) * (y - playerY));

            if (dist > 150)
                break;  // 距离玩家至少150像素
        } while (true);

        // 创建敌人
        Enemy* enemy = new Enemy(enemyPixmap, 1.0);
        enemy->setPlayer(player);  // 设置玩家引用
        enemy->setPos(x, y);

        // 配置敌人属性
        enemy->setSpeed(1.5);            // 移动速度
        enemy->setHealth(3);             // 生命值3点（子弹打3下）
        enemy->setContactDamage(1);      // 接触伤害1点
        enemy->setHurt(1.0);             // 设置怪物攻击伤害（可调）
        enemy->setVisionRange(300);      // 视野范围300像素
        enemy->setAttackRange(35);       // 攻击范围35像素（近战）
        enemy->setAttackCooldown(1000);  // 攻击冷却1秒

        // 添加到场景
        scene->addItem(enemy);
        enemy->setZValue(50);  // 设置层级（低于玩家）

        // 添加到列表
        enemies.append(enemy);
    }
}

void GameView::keyPressEvent(QKeyEvent* event) {
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

    QWidget::keyPressEvent(event);
}

void GameView::keyReleaseEvent(QKeyEvent* event) {
    if (!event)
        return;

    // 传递给玩家处理
    if (player) {
        player->keyReleaseEvent(event);
    }

    QWidget::keyReleaseEvent(event);
}

void GameView::handlePlayerDeath() {
    // 让所有敌人失去玩家引用，转为随机漫游
    for (Enemy* enemy : enemies) {
        if (enemy) {
            enemy->setPlayer(nullptr);  // 移除玩家引用，敌人将进入WANDER状态
        }
    }

    // 断开信号连接，避免重复触发
    if (player) {
        disconnect(player, &Player::playerDied, this, &GameView::handlePlayerDeath);
    }

    // 创建自定义对话框
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("游戏结束");
    msgBox.setText("你已死亡！");
    msgBox.setIcon(QMessageBox::Information);

    // 添加三个按钮
    QPushButton* retryButton = msgBox.addButton("再试一次", QMessageBox::ActionRole);
    QPushButton* menuButton = msgBox.addButton("返回主菜单", QMessageBox::ActionRole);
    QPushButton* quitButton = msgBox.addButton("退出游戏", QMessageBox::ActionRole);

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
}

void GameView::restartGame() {
    // 重新初始化游戏场景
    initGame();
}

void GameView::quitGame() {
    QApplication::quit();
}
