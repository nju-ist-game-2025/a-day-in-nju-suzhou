#include "pausemenu.h"
#include <QApplication>
#include <QBrush>
#include <QFont>
#include <QGraphicsScene>
#include <QPen>
#include <QTimer>
#include "../constants.h"

PauseMenu::PauseMenu(QGraphicsScene* scene, QObject* parent)
    : QObject(parent), m_scene(scene), m_overlay(nullptr), m_menuBackground(nullptr), m_titleText(nullptr), m_resumeProxy(nullptr), m_menuProxy(nullptr), m_exitProxy(nullptr), m_resumeButton(nullptr), m_menuButton(nullptr), m_exitButton(nullptr), m_isVisible(false) {
    createUI();
}

PauseMenu::~PauseMenu() {
    // 先隐藏并从场景中移除
    if (m_overlay) {
        if (m_overlay->scene()) {
            m_scene->removeItem(m_overlay);
        }
        // 删除 overlay（子元素会自动删除）
        delete m_overlay;
        m_overlay = nullptr;
    }

    // 按钮是独立创建的，不是 overlay 的子对象，需要手动删除
    // 但它们通过 QGraphicsProxyWidget 管理，proxy 是 overlay 的子对象
    // 所以 delete overlay 后，proxy 和按钮都会被删除
    m_resumeButton = nullptr;
    m_menuButton = nullptr;
    m_exitButton = nullptr;
}

void PauseMenu::createUI() {
    // 半透明遮罩覆盖整个场景
    m_overlay = new QGraphicsRectItem(0, 0, scene_bound_x, scene_bound_y);
    m_overlay->setBrush(QBrush(QColor(0, 0, 0, 150)));
    m_overlay->setPen(Qt::NoPen);
    m_overlay->setZValue(20000);  // 确保在最上层，高于对话框(10000-10002)

    // 菜单背景
    int menuWidth = 300;
    int menuHeight = 280;
    int menuX = (scene_bound_x - menuWidth) / 2;
    int menuY = (scene_bound_y - menuHeight) / 2;

    m_menuBackground = new QGraphicsRectItem(menuX, menuY, menuWidth, menuHeight, m_overlay);
    m_menuBackground->setBrush(QBrush(QColor(50, 50, 50, 230)));
    m_menuBackground->setPen(QPen(QColor(100, 100, 100), 3));

    // 标题
    m_titleText = new QGraphicsTextItem("游戏暂停", m_overlay);
    QFont titleFont("Microsoft YaHei", 24, QFont::Bold);
    m_titleText->setFont(titleFont);
    m_titleText->setDefaultTextColor(Qt::white);
    // 居中标题
    qreal titleWidth = m_titleText->boundingRect().width();
    m_titleText->setPos((scene_bound_x - titleWidth) / 2, menuY + 20);

    // 按钮样式
    QString resumeButtonStyle =
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4CAF50, stop:1 #388E3C);"
        "   color: white;"
        "   border: 2px solid #2E7D32;"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66BB6A, stop:1 #43A047);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #388E3C, stop:1 #2E7D32);"
        "}";

    QString menuButtonStyle =
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2196F3, stop:1 #1976D2);"
        "   color: white;"
        "   border: 2px solid #1565C0;"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #42A5F5, stop:1 #1E88E5);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1976D2, stop:1 #1565C0);"
        "}";

    QString exitButtonStyle =
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f44336, stop:1 #d32f2f);"
        "   color: white;"
        "   border: 2px solid #c62828;"
        "   border-radius: 8px;"
        "   padding: 8px;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ef5350, stop:1 #e53935);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #d32f2f, stop:1 #c62828);"
        "}";

    int buttonWidth = 200;
    int buttonHeight = 45;
    int buttonX = (scene_bound_x - buttonWidth) / 2;
    int buttonStartY = menuY + 80;
    int buttonSpacing = 55;

    // 继续游戏按钮
    m_resumeButton = new QPushButton("继续游戏");
    m_resumeButton->setFixedSize(buttonWidth, buttonHeight);
    m_resumeButton->setStyleSheet(resumeButtonStyle);
    m_resumeProxy = new QGraphicsProxyWidget(m_overlay);
    m_resumeProxy->setWidget(m_resumeButton);
    m_resumeProxy->setPos(buttonX, buttonStartY);

    // 返回主菜单按钮
    m_menuButton = new QPushButton("返回主菜单");
    m_menuButton->setFixedSize(buttonWidth, buttonHeight);
    m_menuButton->setStyleSheet(menuButtonStyle);
    m_menuProxy = new QGraphicsProxyWidget(m_overlay);
    m_menuProxy->setWidget(m_menuButton);
    m_menuProxy->setPos(buttonX, buttonStartY + buttonSpacing);

    // 退出游戏按钮
    m_exitButton = new QPushButton("退出游戏");
    m_exitButton->setFixedSize(buttonWidth, buttonHeight);
    m_exitButton->setStyleSheet(exitButtonStyle);
    m_exitProxy = new QGraphicsProxyWidget(m_overlay);
    m_exitProxy->setWidget(m_exitButton);
    m_exitProxy->setPos(buttonX, buttonStartY + buttonSpacing * 2);

    // 连接信号
    connect(m_resumeButton, &QPushButton::clicked, this, [this]() {
        hide();
        emit resumeGame();
    });

    connect(m_menuButton, &QPushButton::clicked, this, [this]() {
        hide();
        // 延迟发出信号，确保按钮点击事件完全处理完毕
        QTimer::singleShot(0, this, [this]() {
            emit returnToMenu();
        });
    });

    connect(m_exitButton, &QPushButton::clicked, this, [this]() {
        emit exitGame();
        QApplication::quit();
    });

    // 初始隐藏
    m_overlay->setVisible(false);
}

void PauseMenu::show() {
    if (!m_overlay->scene()) {
        m_scene->addItem(m_overlay);
    }
    m_overlay->setVisible(true);
    m_isVisible = true;
}

void PauseMenu::hide() {
    m_overlay->setVisible(false);
    m_isVisible = false;
}
