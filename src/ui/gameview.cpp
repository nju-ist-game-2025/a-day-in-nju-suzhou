#include "gameview.h"
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>
#include "../constants.h"
#include "../core/resourcefactory.h"

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
    if (player) {
        delete player;
    }
    if (scene) {
        delete scene;
    }
}

void GameView::initGame() {
    try {
        // 清理现有内容
        scene->clear();

        // 加载游戏背景图片
        QPixmap backgroundPixmap = ResourceFactory::loadBackgroundImage("resources/background_game.png", scene_bound_x, scene_bound_y);
        scene->setBackgroundBrush(QBrush(backgroundPixmap));

        // 加载玩家图片
        int playerSize = 60;
        QPixmap playerPixmap = ResourceFactory::createPlayerImage(playerSize, "resources/player.png");

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
        QPixmap bulletPixmap = ResourceFactory::createBulletImage(50, "resources/bullet.png");
        player->setBulletPic(bulletPixmap);
    } catch (const QString& error) {
        QMessageBox::critical(this, "资源加载失败", error);
        emit backToMenu();
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
