#include "codex.h"
#include <QFont>
#include <QPalette>
#include <QPixmap>
#include "../core/resourcefactory.h"

Codex::Codex(QWidget *parent) : QWidget(parent) {
    setupUI();
    loadMonsterData();
}

void Codex::setupUI() {
    // 设置窗口大小
    setFixedSize(800, 600);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel("怪物图鉴", this);
    QFont titleFont;
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: white; margin-bottom: 20px;");

    // 创建内容区域
    QHBoxLayout *contentLayout = new QHBoxLayout();

    // 左侧：怪物列表
    monsterList = new QListWidget();
    monsterList->setFixedWidth(200);
    monsterList->setStyleSheet(
            "QListWidget { background-color: rgba(255,255,255,200); border: 1px solid #CCCCCC; }"
            "QListWidget::item { padding: 8px; border-bottom: 1px solid #EEEEEE; }"
            "QListWidget::item:selected { background-color: #3498db; color: white; }");

    // 右侧：详细信息
    detailBrowser = new QTextBrowser();
    detailBrowser->setStyleSheet(
            "QTextBrowser { background-color: rgba(255,255,255,200); border: 1px solid #CCCCCC; font-size: 14px; }");

    contentLayout->addWidget(monsterList);
    contentLayout->addWidget(detailBrowser);

    // 返回按钮
    backButton = new QPushButton("返回主菜单", this);
    backButton->setFixedSize(150, 40);
    backButton->setStyleSheet(
            "QPushButton {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #e67e22, stop:1 #d35400);"
            "   color: white;"
            "   border: 2px solid #a04000;"
            "   border-radius: 10px;"
            "   font-size: 14px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f39c12, stop:1 #e67e22);"
            "}"
            "QPushButton:pressed {"
            "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #d35400, stop:1 #a04000);"
            "   padding-top: 2px;"
            "   padding-left: 2px;"
            "}");

    // 添加到主布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(contentLayout);
    mainLayout->addWidget(backButton, 0, Qt::AlignCenter);

    // 连接信号
    connect(backButton, &QPushButton::clicked, this, &Codex::returnToMenu);
    connect(monsterList, &QListWidget::itemSelectionChanged, this, [this]() {
        QListWidgetItem *item = monsterList->currentItem();
        if (item) {
            QString monsterName = item->text();
            detailBrowser->setHtml(monsterData.value(monsterName, "暂无详细信息"));
        }
    });

    // 设置背景
    try {
        QPixmap backgroundPixmap = ResourceFactory::loadBackgroundImage("background_main", 800, 600);
        QPalette palette;
        palette.setBrush(QPalette::Window, QBrush(backgroundPixmap));
        setAutoFillBackground(true);
        setPalette(palette);
    } catch (const QString &error) {
        setStyleSheet("background-color: #2c3e50;");
    }
}

void Codex::loadMonsterData() {
    // 怪物数据
    monsterData["闹钟Boss"] =
            "<h2>闹钟Boss</h2>"
            "<p><b>攻击方式：</b>时间停止、闹铃冲击波</p>"
            "<p><b>弱点：</b>贪睡按钮</p>"
            "<p><b>背景故事：</b>每天早上准时打扰你美梦的邪恶闹钟，拥有控制时间的能力。</p>";

    monsterData["洗衣机Boss"] =
            "<h2>洗衣机Boss</h2>"
            "<p><b>攻击方式：</b>漩涡攻击、衣物投掷</p>"
            "<p><b>弱点：</b>断电</p>"
            "<p><b>背景故事：</b>吞噬无数袜子的恐怖洗衣机，能够制造强大的水流漩涡。</p>";

    monsterData["字符串Boss"] =
            "<h2>字符串Boss</h2>"
            "<p><b>攻击方式：</b>编码错误、内存泄漏</p>"
            "<p><b>弱点：</b>调试器</p>"
            "<p><b>背景故事：</b>由无数bug代码组成的数字怪物，能够让程序崩溃。</p>";

    // 添加到列表
    for (auto it = monsterData.begin(); it != monsterData.end(); ++it) {
        monsterList->addItem(it.key());
    }
}

void Codex::returnToMenu() {
    emit backToMenu();
}
