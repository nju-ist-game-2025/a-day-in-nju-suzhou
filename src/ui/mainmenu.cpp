#include "mainmenu.h"
#include <QFile>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QInputDialog>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QResizeEvent>
#include <QTimer>
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"

MainMenu::MainMenu(QWidget* parent) : QWidget(parent) {
    // 设置最小窗口大小，允许调整
    setMinimumSize(800, 600);

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(30);

    // 创建标题（优先尝试使用图片资源）
    titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignCenter);

    // 优先从配置获取标题图片路径
    QString titlePath = ConfigManager::instance().getAssetPath("title");
    if (titlePath.isEmpty()) {
        // 尝试常见默认位置（如果没有配置）
        titlePath = "assets/background/title.png";
    }

    QPixmap titlePixmap;
    if (!titlePath.isEmpty() && QFile::exists(titlePath)) {
        titlePixmap = QPixmap(titlePath);
    }

    if (!titlePixmap.isNull()) {
        // 使用图片作为标题
        m_titlePixmap = titlePixmap;  // 保存原始图用于缩放
        m_useTitleImage = true;

        QTimer::singleShot(0, this, &MainMenu::adjustTitlePixmap);
        titleLabel->setStyleSheet("background:transparent;");
    } else {
        // 回退到文本标题
        titleLabel->setText("智科er的一天");
        QFont titleFont;
        titleFont.setFamily("Microsoft YaHei");
        titleFont.setPointSize(48);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        titleLabel->setStyleSheet("color: #FFFFFF;");
    }

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

    QString characterButtonStyle =
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #9C27B0, stop:1 #7B1FA2);"
        "   color: white;"
        "   border: 2px solid #6A1B9A;"
        "   border-radius: 10px;"
        "   padding: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #AB47BC, stop:1 #8E24AA);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #7B1FA2, stop:1 #6A1B9A);"
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

    // 创建角色选择按钮
    characterButton = new QPushButton("选择角色", this);
    characterButton->setFixedSize(220, 60);
    characterButton->setFont(buttonFont);
    characterButton->setStyleSheet(characterButtonStyle);

    // 创建退出按钮
    exitButton = new QPushButton("退出游戏", this);
    exitButton->setFixedSize(220, 60);
    exitButton->setFont(buttonFont);
    exitButton->setStyleSheet(exitButtonStyle);

    // 创建开发者模式按钮（橙色样式）
    QString devButtonStyle =
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FF9800, stop:1 #F57C00);"
        "   color: white;"
        "   border: 2px solid #E65100;"
        "   border-radius: 10px;"
        "   padding: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFB74D, stop:1 #FFA726);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F57C00, stop:1 #E65100);"
        "   padding-top: 7px;"
        "   padding-left: 7px;"
        "}";
    devModeButton = new QPushButton("开发者模式", this);
    devModeButton->setFixedSize(220, 60);
    devModeButton->setFont(buttonFont);
    devModeButton->setStyleSheet(devButtonStyle);

    // 添加到布局
    mainLayout->addStretch();
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(50);
    mainLayout->addWidget(startButton, 0, Qt::AlignCenter);
    mainLayout->addWidget(characterButton, 0, Qt::AlignCenter);  // 角色选择按钮
    mainLayout->addWidget(codexButton, 0, Qt::AlignCenter);      // 新增按钮
    mainLayout->addWidget(devModeButton, 0, Qt::AlignCenter);    // 开发者模式按钮
    mainLayout->addWidget(exitButton, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    // 加载主菜单背景图片
    m_backgroundPath = ConfigManager::instance().getAssetPath("background_main");
    if (m_backgroundPath.isEmpty()) {
        m_backgroundPath = "assets/background/main.png";
    }

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
    connect(characterButton, &QPushButton::clicked, this, &MainMenu::selectCharacterClicked);  // 角色选择连接
    connect(codexButton, &QPushButton::clicked, this, &MainMenu::codexClicked);                // 新增连接
    connect(exitButton, &QPushButton::clicked, this, &MainMenu::exitGameClicked);
    
    // 开发者模式按钮 - 弹出关卡选择对话框
    connect(devModeButton, &QPushButton::clicked, this, [this]() {
        QStringList levels;
        levels << "第1关 - 时钟梦境" << "第2关 - 袜子王国" << "第3关 - 终极挑战";
        
        bool ok;
        QString selectedLevel = QInputDialog::getItem(this, "开发者模式", 
            "选择要跳转的关卡:", levels, 0, false, &ok);
        
        if (ok && !selectedLevel.isEmpty()) {
            int levelNum = levels.indexOf(selectedLevel) + 1;
            emit devModeClicked(levelNum);
        }
    });
}

void MainMenu::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // 重新加载并缩放背景图片
    try {
        QPixmap backgroundPixmap = ResourceFactory::loadBackgroundImage("background_main", 800, 600);
        if (!backgroundPixmap.isNull()) {
            QPalette palette;
            palette.setBrush(QPalette::Window, QBrush(backgroundPixmap.scaled(event->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
            setPalette(palette);
        }
    } catch (const QString&) {
        // 背景加载失败，保持当前样式
    }

    // 等比例缩放按钮和字体
    double scaleX = event->size().width() / 800.0;
    double scaleY = event->size().height() / 600.0;
    double scale = qMin(scaleX, scaleY);

    // 缩放按钮大小
    int buttonWidth = static_cast<int>(220 * scale);
    int buttonHeight = static_cast<int>(60 * scale);
    startButton->setFixedSize(buttonWidth, buttonHeight);
    characterButton->setFixedSize(buttonWidth, buttonHeight);
    codexButton->setFixedSize(buttonWidth, buttonHeight);
    devModeButton->setFixedSize(buttonWidth, buttonHeight);
    exitButton->setFixedSize(buttonWidth, buttonHeight);

    // 缩放按钮字体
    QFont buttonFont;
    buttonFont.setFamily("Microsoft YaHei");
    buttonFont.setPointSize(static_cast<int>(16 * scale));
    buttonFont.setBold(true);
    startButton->setFont(buttonFont);
    characterButton->setFont(buttonFont);
    codexButton->setFont(buttonFont);
    devModeButton->setFont(buttonFont);
    exitButton->setFont(buttonFont);

    // 标题：如果使用图片则缩放图片，否则缩放文本字体
    if (m_useTitleImage && !m_titlePixmap.isNull()) {
        adjustTitlePixmap();
    } else {
        QFont titleFont;
        titleFont.setFamily("Microsoft YaHei");
        titleFont.setPointSize(static_cast<int>(48 * scale));
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
    }
}

void MainMenu::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (m_useTitleImage && !m_titlePixmap.isNull()) {
        adjustTitlePixmap();
    }
}

void MainMenu::adjustTitlePixmap() {
    if (!m_useTitleImage || m_titlePixmap.isNull())
        return;

    const double widthRatio = 0.95;   // 宽度占窗口宽度的比例
    const double heightRatio = 0.45;  // 增大高度占比以让图片更显眼

    QSize targetSize(static_cast<int>(width() * widthRatio), static_cast<int>(height() * heightRatio));

    // 防止目标尺寸为0（窗口尚未布局），再尝试使用父窗口/默认值
    if (targetSize.width() <= 0 || targetSize.height() <= 0) {
        QSize fallback = QSize(800, 200);
        targetSize = fallback;
    }

    QPixmap scaled = m_titlePixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    titleLabel->setPixmap(scaled);
}
