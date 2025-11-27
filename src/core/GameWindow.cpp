#include "GameWindow.h"
#include <QDebug>
#include <QFile>
#include "world/map.h"

namespace {
Map& sharedMapInstance() {
    static Map map;
    return map;
}
}  // namespace

void clearMapWalls() {
    sharedMapInstance().clear();
}

GameWindow::GameWindow(QWidget* parent)
    : QMainWindow(parent), m_selectedCharacter("assets/player/player.png") {
    // 设置窗口属性
    setWindowTitle("智科er的一天");

    // 设置最小窗口大小和默认大小，允许调整大小和全屏
    setMinimumSize(800, 600);
    resize(800, 600);

    // 创建堆叠窗口部件
    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);

    // 创建主菜单
    mainMenu = new MainMenu(this);
    connect(mainMenu, &MainMenu::startGameClicked, this, &GameWindow::startGame);
    connect(mainMenu, &MainMenu::selectCharacterClicked, this, &GameWindow::showCharacterSelector);  // 角色选择连接
    connect(mainMenu, &MainMenu::codexClicked, this, &GameWindow::showCodex);                        // 新增连接
    connect(mainMenu, &MainMenu::exitGameClicked, this, &GameWindow::exitGame);
    connect(mainMenu, &MainMenu::devModeClicked, this, &GameWindow::startGameAtLevel);  // 开发者模式连接

    // 创建游戏视图
    gameView = new GameView(this);
    connect(gameView, &GameView::backToMenu, this, &GameWindow::showMainMenu);
    connect(gameView, &GameView::requestRestart, this, &GameWindow::onRequestRestart);

    // 创建图鉴界面（新增）
    codex = new Codex(this);
    connect(codex, &Codex::backToMenu, this, &GameWindow::showMainMenu);

    // 创建角色选择界面
    characterSelector = new CharacterSelector(this);
    connect(characterSelector, &CharacterSelector::characterSelected, this, &GameWindow::onCharacterSelected);
    connect(characterSelector, &CharacterSelector::backToMenu, this, &GameWindow::showMainMenu);

    // 添加到堆叠窗口
    stackedWidget->addWidget(mainMenu);
    stackedWidget->addWidget(gameView);
    stackedWidget->addWidget(codex);              // 新增图鉴界面
    stackedWidget->addWidget(characterSelector);  // 角色选择界面

    // 显示主菜单
    stackedWidget->setCurrentWidget(mainMenu);
}

void GameWindow::onRequestRestart() {
    // 使用单次事件循环调度，确保所有对话框和事件处理完毕后再重启
    QTimer::singleShot(0, gameView, &GameView::initGame);
}

GameWindow::~GameWindow() {
    // stackedWidget和其他组件会由Qt的父子关系自动清理
}

void GameWindow::showMainMenu() {
    // 彻底清理游戏状态，确保下次开始是全新的游戏
    gameView->cleanupGame();
    stackedWidget->setCurrentWidget(mainMenu);
}

void GameWindow::startGame() {
    gameView->initGame();
    stackedWidget->setCurrentWidget(gameView);
    gameView->setFocus();  // 确保游戏视图获得焦点以接收键盘事件
}

void GameWindow::startGameAtLevel(int level, int maxHealth, int bulletDamage, bool skipToBoss) {
    qDebug() << "开发者模式: 跳转到第" << level << "关, 血量上限:" << maxHealth << ", 子弹伤害:" << bulletDamage
             << ", 直接进入Boss房:" << skipToBoss;
    gameView->setStartLevel(level);
    gameView->setDevModeSettings(maxHealth, bulletDamage, skipToBoss);
    gameView->initGame();
    stackedWidget->setCurrentWidget(gameView);
    gameView->setFocus();
}

void GameWindow::showCodex() {
    stackedWidget->setCurrentWidget(codex);  // 切换到图鉴界面
}

void GameWindow::showCharacterSelector() {
    stackedWidget->setCurrentWidget(characterSelector);
}

void GameWindow::onCharacterSelected(const QString& characterPath) {
    m_selectedCharacter = characterPath;
    gameView->setPlayerCharacter(characterPath);
    showMainMenu();
}

void GameWindow::exitGame() {
    close();
}

void setupMap(QGraphicsScene* scene) {
    Map& map = sharedMapInstance();
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
