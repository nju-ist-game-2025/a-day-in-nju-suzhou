#include "mainmenu.h"
#include <QFile>
#include <QFont>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include "../core/resourcefactory.h"

MainMenu::MainMenu(QWidget* parent) : QWidget(parent) {
    // 设置窗口大小
    setFixedSize(800, 600);

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(30);

    // 创建标题
    titleLabel = new QLabel("智科er的一天", this);
    QFont titleFont;
    titleFont.setPointSize(36);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: white;");

    // 创建开始游戏按钮
    startButton = new QPushButton("开始游戏", this);
    startButton->setFixedSize(200, 60);
    QFont buttonFont;
    buttonFont.setPointSize(16);
    startButton->setFont(buttonFont);
    startButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}");

    // 创建退出按钮
    exitButton = new QPushButton("退出游戏", this);
    exitButton->setFixedSize(200, 60);
    exitButton->setFont(buttonFont);
    exitButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #f44336;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #da190b;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #c1190b;"
        "}");

    // 添加到布局
    mainLayout->addStretch();
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(50);
    mainLayout->addWidget(startButton, 0, Qt::AlignCenter);
    mainLayout->addWidget(exitButton, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    // 加载主菜单背景图片
    try {
        QPixmap backgroundPixmap = ResourceFactory::loadBackgroundImage("assets/background_main.png", 800, 600);
        QPalette palette;
        palette.setBrush(QPalette::Window, QBrush(backgroundPixmap));
        setAutoFillBackground(true);
        setPalette(palette);
    } catch (const QString& error) {
        QMessageBox::critical(this, "资源加载失败", error);
        // 背景加载失败，使用默认颜色
        setStyleSheet("background-color: #2c3e50;");
    }

    // 连接信号和槽
    connect(startButton, &QPushButton::clicked, this, &MainMenu::startGameClicked);
    connect(exitButton, &QPushButton::clicked, this, &MainMenu::exitGameClicked);
}
