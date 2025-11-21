#include "GameWindow.h"
#include <QDebug>
#include <QFile>
#include "world/map.h"

GameWindow::GameWindow(QWidget* parent)
    : QMainWindow(parent) {
    // 设置窗口属性
    setWindowTitle("智科er的一天");
    setFixedSize(800, 600);

    // 创建堆叠窗口部件
    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);

    // 创建主菜单
    mainMenu = new MainMenu(this);
    connect(mainMenu, &MainMenu::startGameClicked, this, &GameWindow::startGame);
    connect(mainMenu, &MainMenu::codexClicked, this, &GameWindow::showCodex);  // 新增连接
    connect(mainMenu, &MainMenu::exitGameClicked, this, &GameWindow::exitGame);

    // 创建游戏视图
    gameView = new GameView(this);
    connect(gameView, &GameView::backToMenu, this, &GameWindow::showMainMenu);

    // 创建图鉴界面（新增）
    codex = new Codex(this);
    connect(codex, &Codex::backToMenu, this, &GameWindow::showMainMenu);

    // 添加到堆叠窗口
    stackedWidget->addWidget(mainMenu);
    stackedWidget->addWidget(gameView);
    stackedWidget->addWidget(codex);  // 新增图鉴界面

    // 显示主菜单
    stackedWidget->setCurrentWidget(mainMenu);
}

GameWindow::~GameWindow() {
    // stackedWidget和其他组件会由Qt的父子关系自动清理
}

void GameWindow::showMainMenu() {
    stackedWidget->setCurrentWidget(mainMenu);
}

void GameWindow::startGame() {
    gameView->initGame();
    stackedWidget->setCurrentWidget(gameView);
    gameView->setFocus();  // 确保游戏视图获得焦点以接收键盘事件
}

void GameWindow::showCodex() {
    stackedWidget->setCurrentWidget(codex);  // 切换到图鉴界面
}

void GameWindow::exitGame() {
    close();
}

void setupMap(QGraphicsScene* scene) {
    static Map map;
    const QString levelPath = QStringLiteral("assets/levels/level1_wall.json");
    // 墙壁配置是可选的，如果文件不存在也不影响游戏运行
    if (QFile::exists(levelPath)) {
        if (!map.loadFromFile(levelPath, scene)) {
            qWarning() << "加载地图失败:" << levelPath;
        }
    } else {
        qDebug() << "未找到墙壁配置文件:" << levelPath;
    }
}
