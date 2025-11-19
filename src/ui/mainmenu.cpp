#include "mainmenu.h"
#include <QFile>
#include <QFont>
#include <QGraphicsDropShadowEffect>
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
    titleFont.setFamily("Microsoft YaHei");
    titleFont.setPointSize(48);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #FFFFFF;");

    // 添加标题阴影效果
    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(5);
    shadowEffect->setColor(QColor(0, 0, 0, 150));
    shadowEffect->setOffset(3, 3);
    titleLabel->setGraphicsEffect(shadowEffect);

    // 通用按钮样式
    QString buttonStyle =
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4CAF50, stop:1 #388E3C);"
        "   color: white;"
        "   border: 2px solid #2E7D32;"
        "   border-radius: 10px;"
        "   padding: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66BB6A, stop:1 #43A047);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #388E3C, stop:1 #2E7D32);"
        "   padding-top: 7px;"
        "   padding-left: 7px;"
        "}";

    QString codexButtonStyle =
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2196F3, stop:1 #1976D2);"
        "   color: white;"
        "   border: 2px solid #1565C0;"
        "   border-radius: 10px;"
        "   padding: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #42A5F5, stop:1 #1E88E5);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1976D2, stop:1 #1565C0);"
        "   padding-top: 7px;"
        "   padding-left: 7px;"
        "}";

    QString exitButtonStyle =
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f44336, stop:1 #d32f2f);"
        "   color: white;"
        "   border: 2px solid #c62828;"
        "   border-radius: 10px;"
        "   padding: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ef5350, stop:1 #e53935);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #d32f2f, stop:1 #c62828);"
        "   padding-top: 7px;"
        "   padding-left: 7px;"
        "}";

    // 创建开始游戏按钮
    startButton = new QPushButton("开始游戏", this);
    startButton->setFixedSize(220, 60);
    QFont buttonFont;
    buttonFont.setFamily("Microsoft YaHei");
    buttonFont.setPointSize(16);
    buttonFont.setBold(true);
    startButton->setFont(buttonFont);
    startButton->setStyleSheet(buttonStyle);

    // 创建图鉴按钮
    codexButton = new QPushButton("怪物图鉴", this);
    codexButton->setFixedSize(220, 60);
    codexButton->setFont(buttonFont);
    codexButton->setStyleSheet(codexButtonStyle);

    // 创建退出按钮
    exitButton = new QPushButton("退出游戏", this);
    exitButton->setFixedSize(220, 60);
    exitButton->setFont(buttonFont);
    exitButton->setStyleSheet(exitButtonStyle);

    // 添加到布局
    mainLayout->addStretch();
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(50);
    mainLayout->addWidget(startButton, 0, Qt::AlignCenter);
    mainLayout->addWidget(codexButton, 0, Qt::AlignCenter);  // 新增按钮
    mainLayout->addWidget(exitButton, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    // 加载主菜单背景图片
    try {
        QPixmap backgroundPixmap = ResourceFactory::loadBackgroundImage("background_main", 800, 600);
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
    connect(codexButton, &QPushButton::clicked, this, &MainMenu::codexClicked);  // 新增连接
    connect(exitButton, &QPushButton::clicked, this, &MainMenu::exitGameClicked);
}
