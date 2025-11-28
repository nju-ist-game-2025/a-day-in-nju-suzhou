#include "codex.h"
#include <QFile>
#include <QFont>
#include <QMouseEvent>
#include <QPalette>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShowEvent>
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"
#include "../items/itemeffectconfig.h"

// ==================== CodexCard 实现 ====================

CodexCard::CodexCard(const CodexEntry& entry, QWidget* parent)
    : QWidget(parent), m_entry(entry) {
    setFixedSize(120, 150);
    setCursor(Qt::PointingHandCursor);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);
    layout->setAlignment(Qt::AlignCenter);

    // 图片标签
    m_imageLabel = new QLabel(this);
    m_imageLabel->setFixedSize(80, 80);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background-color: rgba(255,255,255,220); border-radius: 10px;");

    // 加载图片
    QPixmap pixmap(entry.imagePath);
    if (!pixmap.isNull()) {
        pixmap = pixmap.scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_imageLabel->setPixmap(pixmap);
    } else {
        m_imageLabel->setText("?");
        m_imageLabel->setStyleSheet(
            "background-color: rgba(220,220,220,220); border-radius: 10px; font-size: 24px; color: #333;");
    }

    // 名称标签
    m_nameLabel = new QLabel(entry.name, this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setWordWrap(true);
    QFont nameFont;
    nameFont.setFamily("Microsoft YaHei");
    nameFont.setPointSize(10);
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);
    m_nameLabel->setStyleSheet("color: #1a1a1a;");

    layout->addWidget(m_imageLabel, 0, Qt::AlignCenter);
    layout->addWidget(m_nameLabel, 0, Qt::AlignCenter);

    // 默认样式 - 蓝色主题
    setStyleSheet(
        "CodexCard {"
        "   background-color: rgba(100, 149, 237, 220);"
        "   border: 2px solid rgba(70, 130, 220, 255);"
        "   border-radius: 15px;"
        "}");
}

void CodexCard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_entry);
    }
    QWidget::mousePressEvent(event);
}

void CodexCard::enterEvent(QEnterEvent* event) {
    setStyleSheet(
        "CodexCard {"
        "   background-color: rgba(65, 105, 225, 240);"
        "   border: 2px solid rgba(30, 90, 200, 255);"
        "   border-radius: 15px;"
        "}");
    QWidget::enterEvent(event);
}

void CodexCard::leaveEvent(QEvent* event) {
    setStyleSheet(
        "CodexCard {"
        "   background-color: rgba(100, 149, 237, 220);"
        "   border: 2px solid rgba(70, 130, 220, 255);"
        "   border-radius: 15px;"
        "}");
    QWidget::leaveEvent(event);
}

void CodexCard::setScale(double scale) {
    // 缩放卡片大小
    int cardWidth = static_cast<int>(BASE_CARD_WIDTH * scale);
    int cardHeight = static_cast<int>(BASE_CARD_HEIGHT * scale);
    setFixedSize(cardWidth, cardHeight);

    // 缩放图片标签
    int imageSize = static_cast<int>(BASE_IMAGE_SIZE * scale);
    m_imageLabel->setFixedSize(imageSize, imageSize);

    // 重新加载并缩放图片
    QPixmap pixmap(m_entry.imagePath);
    if (!pixmap.isNull()) {
        int pixmapSize = static_cast<int>(BASE_PIXMAP_SIZE * scale);
        pixmap = pixmap.scaled(pixmapSize, pixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_imageLabel->setPixmap(pixmap);
    }

    // 缩放字体
    int fontSize = static_cast<int>(BASE_FONT_SIZE * scale);
    if (fontSize < 8)
        fontSize = 8;
    QFont nameFont;
    nameFont.setFamily("Microsoft YaHei");
    nameFont.setPointSize(fontSize);
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);

    // 缩放布局边距
    int margin = static_cast<int>(5 * scale);
    int spacing = static_cast<int>(5 * scale);
    if (layout()) {
        layout()->setContentsMargins(margin, margin, margin, margin);
        if (QVBoxLayout* vLayout = qobject_cast<QVBoxLayout*>(layout())) {
            vLayout->setSpacing(spacing);
        }
    }

    // 缩放圆角
    int borderRadius = static_cast<int>(15 * scale);
    int imgBorderRadius = static_cast<int>(10 * scale);
    setStyleSheet(QString(
                      "CodexCard {"
                      "   background-color: rgba(100, 149, 237, 220);"
                      "   border: 2px solid rgba(70, 130, 220, 255);"
                      "   border-radius: %1px;"
                      "}")
                      .arg(borderRadius));
    m_imageLabel->setStyleSheet(QString(
                                    "background-color: rgba(255,255,255,220); border-radius: %1px;")
                                    .arg(imgBorderRadius));
}

// ==================== CodexDetailDialog 实现 ====================

CodexDetailDialog::CodexDetailDialog(const CodexEntry& entry, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(entry.name);
    setMinimumSize(500, 400);
    setMaximumSize(600, 600);
    setStyleSheet(
        "QDialog { background-color: rgba(230, 240, 255, 255); border: 2px solid rgba(100, 149, 237, 200); border-radius: 10px; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 顶部：图片和名称
    QHBoxLayout* headerLayout = new QHBoxLayout();

    QLabel* imageLabel = new QLabel(this);
    imageLabel->setFixedSize(100, 100);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet(
        "background-color: rgba(255, 255, 255, 220); border: 2px solid rgba(100, 149, 237, 200); border-radius: 15px;");
    QPixmap pixmap(entry.imagePath);
    if (!pixmap.isNull()) {
        pixmap = pixmap.scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imageLabel->setPixmap(pixmap);
    }

    QLabel* nameLabel = new QLabel(entry.name, this);
    QFont nameFont;
    nameFont.setFamily("Microsoft YaHei");
    nameFont.setPointSize(24);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);
    nameLabel->setStyleSheet("color: #1a1a1a;");

    headerLayout->addWidget(imageLabel);
    headerLayout->addSpacing(15);
    headerLayout->addWidget(nameLabel);
    headerLayout->addStretch();

    mainLayout->addLayout(headerLayout);

    // 分隔线
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: rgba(100, 149, 237, 150);");
    mainLayout->addWidget(line);

    // 详情内容
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: rgba(100, 149, 237, 50); width: 10px; border-radius: 5px; }"
        "QScrollBar::handle:vertical { background: rgba(70, 130, 220, 150); border-radius: 5px; }");

    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(6);

    QFont labelFont;
    labelFont.setFamily("Microsoft YaHei");
    labelFont.setPointSize(11);

    // 标题样式：带底部细线，起到分隔作用
    QString labelStyle = "color: #4a6fa5; font-weight: bold; padding-bottom: 3px; border-bottom: 1px solid rgba(100, 149, 237, 120);";
    // 内容样式：左缩进，与标题区分
    QString valueStyle = "color: #2c3e50; padding: 6px 0px 12px 16px;";

    // 辅助lambda：创建一个属性组（标题+内容）
    auto addInfoSection = [&](const QString& icon, const QString& title, const QString& value) {
        QLabel* titleLabel = new QLabel(icon + " " + title, contentWidget);
        titleLabel->setFont(labelFont);
        titleLabel->setStyleSheet(labelStyle);
        contentLayout->addWidget(titleLabel);

        QLabel* valueLabel = new QLabel(value, contentWidget);
        valueLabel->setFont(labelFont);
        valueLabel->setStyleSheet(valueStyle);
        valueLabel->setWordWrap(true);
        contentLayout->addWidget(valueLabel);
    };

    // 如果不是玩家/NPC，显示战斗属性
    if (!entry.isCharacter) {
        if (entry.health > 0) {
            addInfoSection("❤", "血量", QString::number(entry.health));
        }
        if (!entry.attackMethod.isEmpty()) {
            addInfoSection("⚔", "攻击方式", entry.attackMethod);
        }
        if (!entry.skills.isEmpty()) {
            addInfoSection("✨", "技能", entry.skills);
        }
        if (!entry.traits.isEmpty()) {
            addInfoSection("🔮", "特性", entry.traits);
        }
        if (!entry.weakness.isEmpty()) {
            addInfoSection("💔", "弱点", entry.weakness);
        }

        // 显示Boss各阶段图片
        if (!entry.phaseImages.isEmpty()) {
            QLabel* phaseTitle = new QLabel("🎭 形态一览", contentWidget);
            phaseTitle->setFont(labelFont);
            phaseTitle->setStyleSheet(labelStyle);
            contentLayout->addWidget(phaseTitle);

            QHBoxLayout* phaseLayout = new QHBoxLayout();
            phaseLayout->setSpacing(15);
            phaseLayout->setContentsMargins(16, 6, 0, 12);

            for (const PhaseImage& phase : entry.phaseImages) {
                QVBoxLayout* phaseItemLayout = new QVBoxLayout();
                phaseItemLayout->setSpacing(5);
                phaseItemLayout->setAlignment(Qt::AlignCenter);

                QLabel* phaseImageLabel = new QLabel(contentWidget);
                phaseImageLabel->setFixedSize(70, 70);
                phaseImageLabel->setAlignment(Qt::AlignCenter);
                phaseImageLabel->setStyleSheet("background-color: rgba(255, 255, 255, 200); border: 1px solid rgba(100, 149, 237, 150); border-radius: 8px;");
                QPixmap phasePix(phase.imagePath);
                if (!phasePix.isNull()) {
                    phasePix = phasePix.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    phaseImageLabel->setPixmap(phasePix);
                }

                QLabel* phaseNameLabel = new QLabel(phase.phaseName, contentWidget);
                phaseNameLabel->setAlignment(Qt::AlignCenter);
                QFont phaseFont;
                phaseFont.setFamily("Microsoft YaHei");
                phaseFont.setPointSize(9);
                phaseNameLabel->setFont(phaseFont);
                phaseNameLabel->setStyleSheet("color: #4a6fa5;");

                phaseItemLayout->addWidget(phaseImageLabel);
                phaseItemLayout->addWidget(phaseNameLabel);
                phaseLayout->addLayout(phaseItemLayout);
            }
            phaseLayout->addStretch();

            QWidget* phaseContainer = new QWidget(contentWidget);
            phaseContainer->setLayout(phaseLayout);
            contentLayout->addWidget(phaseContainer);
        }
    } else {
        // 对于玩家/NPC/道具/机制，显示skills作为效果说明
        if (!entry.skills.isEmpty()) {
            addInfoSection("✨", "效果", entry.skills);
        }
    }

    // 背景故事（所有条目都有）
    addInfoSection("📖", "背景故事", entry.backstory);

    contentLayout->addStretch();
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    // 关闭按钮
    QPushButton* closeBtn = new QPushButton("关闭", this);
    closeBtn->setFixedSize(120, 40);
    QFont btnFont;
    btnFont.setFamily("Microsoft YaHei");
    btnFont.setPointSize(12);
    btnFont.setBold(true);
    closeBtn->setFont(btnFont);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4a90d9, stop:1 #2980b9);"
        "   color: white;"
        "   border: 2px solid #2471a3;"
        "   border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #5dade2, stop:1 #4a90d9);"
        "}");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    mainLayout->addWidget(closeBtn, 0, Qt::AlignCenter);
}

// ==================== Codex 主类实现 ====================

Codex::Codex(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadBossData();
    loadEnemyData();
    loadPlayerData();
    loadUsagiData();
    loadItemData();
    loadMechanicsData();
    loadMysteryData();

    // 创建各标签页
    tabWidget->addTab(createCategoryPage(m_bossEntries), "Boss");
    tabWidget->addTab(createCategoryPage(m_enemyEntries), "敌人");
    tabWidget->addTab(createCategoryPage(m_playerEntries), "玩家");
    tabWidget->addTab(createCategoryPage(m_usagiEntries), "乌萨奇");
    tabWidget->addTab(createCategoryPage(m_itemEntries), "道具");
    tabWidget->addTab(createCategoryPage(m_mechanicsEntries), "机制");
    tabWidget->addTab(createCategoryPage(m_mysteryEntries), "神秘物品");
}

void Codex::setupUI() {
    setMinimumSize(800, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 标题
    titleLabel = new QLabel("游戏图鉴", this);
    QFont titleFont;
    titleFont.setFamily("Microsoft YaHei");
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #1a1a1a; margin-bottom: 10px;");

    // 标签页组件
    tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "   border: 2px solid rgba(100, 149, 237, 200);"
        "   border-radius: 10px;"
        "   background-color: rgba(255, 255, 255, 230);"
        "}"
        "QTabBar::tab {"
        "   background: rgba(180, 200, 230, 220);"
        "   color: #1a1a1a;"
        "   padding: 10px 25px;"
        "   margin-right: 5px;"
        "   border-top-left-radius: 10px;"
        "   border-top-right-radius: 10px;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "}"
        "QTabBar::tab:selected {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4a90d9, stop:1 #2980b9);"
        "   color: white;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "   background: rgba(150, 180, 220, 240);"
        "}");

    // 返回按钮
    backButton = new QPushButton("返回主菜单", this);
    backButton->setFixedSize(150, 40);
    QFont btnFont;
    btnFont.setFamily("Microsoft YaHei");
    btnFont.setPointSize(14);
    btnFont.setBold(true);
    backButton->setFont(btnFont);
    backButton->setStyleSheet(
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #e67e22, stop:1 #d35400);"
        "   color: white;"
        "   border: 2px solid #a04000;"
        "   border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f39c12, stop:1 #e67e22);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #d35400, stop:1 #a04000);"
        "}");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(backButton, 0, Qt::AlignCenter);

    connect(backButton, &QPushButton::clicked, this, &Codex::returnToMenu);

    // 设置背景
    try {
        // 图鉴页面专用背景
        QPixmap backgroundPixmap = ResourceFactory::loadBackgroundImage("background_codex", 800, 600);
        QPalette palette;
        palette.setBrush(QPalette::Window, QBrush(backgroundPixmap));
        setAutoFillBackground(true);
        setPalette(palette);
    } catch (const QString&) {
        setStyleSheet("background-color: #2c3e50;");
    }
}

QWidget* Codex::createCategoryPage(const QList<CodexEntry>& entries) {
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: rgba(100, 149, 237, 80); width: 12px; border-radius: 6px; }"
        "QScrollBar::handle:vertical { background: rgba(70, 130, 220, 180); border-radius: 6px; min-height: 30px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }");

    QWidget* contentWidget = new QWidget();
    contentWidget->setStyleSheet("background: transparent;");

    QGridLayout* gridLayout = new QGridLayout(contentWidget);
    gridLayout->setContentsMargins(30, 30, 30, 30);
    gridLayout->setHorizontalSpacing(25);
    gridLayout->setVerticalSpacing(25);

    int col = 0;
    int row = 0;
    int maxCols = 5;  // 每行最多5个卡片

    for (const CodexEntry& entry : entries) {
        CodexCard* card = new CodexCard(entry, contentWidget);
        connect(card, &CodexCard::clicked, this, &Codex::showEntryDetail);
        gridLayout->addWidget(card, row, col, Qt::AlignCenter);  // 改为居中对齐
        m_allCards.append(card);                                 // 保存卡片引用

        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }

    // 设置列宽均匀分布
    for (int i = 0; i < maxCols; ++i) {
        gridLayout->setColumnStretch(i, 1);  // 所有列均匀拉伸
    }

    // 添加弹簧填充剩余垂直空间
    gridLayout->setRowStretch(row + 1, 1);

    scrollArea->setWidget(contentWidget);
    return scrollArea;
}

void Codex::showEntryDetail(const CodexEntry& entry) {
    CodexDetailDialog dialog(entry, this);
    dialog.exec();
}

void Codex::loadBossData() {
    ConfigManager& config = ConfigManager::instance();

    // 梦魇Boss
    CodexEntry nightmare;
    nightmare.name = "梦魇";
    nightmare.imagePath = "assets/boss/Nightmare/Nightmare.png";
    nightmare.health = config.getBossInt("nightmare", "phase1", "health", 250);
    nightmare.attackMethod = "单段冲刺攻击，二阶段瞬移突袭";
    nightmare.skills = "【亡语】一阶段死亡时击杀场上所有小怪并进入二阶段\n【噩梦缠绕】剥夺玩家视野3秒后瞬移至玩家身边\n【噩梦降临】召唤大量小怪并发动强制冲刺";
    nightmare.traits = QString("一阶段死亡后自动进入二阶段，二阶段血量%1，%2%伤害减免")
                           .arg(config.getBossInt("nightmare", "phase2", "health", 300))
                           .arg(static_cast<int>((1.0 - config.getBossDouble("nightmare", "phase1", "damage_scale", 0.7)) * 100));
    nightmare.weakness = QString("炸弹闹钟可对其造成%1点伤害").arg(config.getEnemyInt("clock_boom", "explosion_damage_nightmare", 50));
    nightmare.backstory = "梦魇讨厌清晨，恨到它会在你睡得正香时把你从美梦中拽出来。它不在乎你有没有睡够，不在乎你是否还想再躺五分钟。它只知道，是时候起床了。\n\n其实呢，梦魇一直暗暗地渴望着你能早点入睡——这样它就能更早地来折磨你了。";
    nightmare.isCharacter = false;
    nightmare.phaseImages = {
        {"assets/boss/Nightmare/Nightmare.png", "一阶段"},
        {"assets/boss/Nightmare/Nightmare2.png", "二阶段"}};
    m_bossEntries.append(nightmare);

    // 洗衣机Boss
    CodexEntry washmachine;
    washmachine.name = "洗衣机";
    washmachine.imagePath = "assets/boss/WashMachine/WashMachineNormally.png";
    washmachine.health = config.getBossInt("washmachine", "phase1", "health", 400);
    washmachine.attackMethod = "普通阶段：四方向水柱冲击\n愤怒阶段：高速冲刺\n变异阶段：毒气攻击（扩散+追踪）";
    washmachine.skills = QString("【愤怒】%1%血量触发，召唤旋转臭袜子护盾\n【变异】%2%血量触发，吸收场上所有物体进行强化，释放有毒气体")
                             .arg(static_cast<int>(config.getBossDouble("washmachine", "phase2", "health_threshold", 0.7) * 100))
                             .arg(static_cast<int>(config.getBossDouble("washmachine", "phase3", "health_threshold", 0.4) * 100));
    washmachine.traits = QString("三阶段Boss，每个阶段有独特的攻击模式，%1%伤害减免")
                             .arg(static_cast<int>((1.0 - config.getBossDouble("washmachine", "phase1", "damage_scale", 0.8)) * 100));
    washmachine.weakness = "";
    washmachine.backstory = "洗衣机情不自禁地转动着滚筒。是什么节奏呢？嗨，是同学们塞进来的臭袜子散发的独特韵律，这种频率的震动，只有洗衣机才能感受到。\n\n它曾经只是一台普通的公共洗衣机，直到有一天，一个同学往里面塞了三天没洗的袜子和一周没换的内裤。从那以后，洗衣机就变了。";
    washmachine.isCharacter = false;
    washmachine.phaseImages = {
        {"assets/boss/WashMachine/WashMachineNormally.png", "普通"},
        {"assets/boss/WashMachine/WashMachineAngrily.png", "愤怒"},
        {"assets/boss/WashMachine/WashMachineMutated.png", "变异"}};
    m_bossEntries.append(washmachine);

    // 奶牛张Boss
    CodexEntry teacher;
    teacher.name = "奶牛张";
    teacher.imagePath = "assets/boss/Teacher/cow.png";
    teacher.health = config.getBossInt("teacher", "phase1", "health", 500);
    teacher.attackMethod = "授课阶段：正态分布弹幕、随机点名红圈\n期中考试：追踪考卷、极大似然估计陷阱、召唤监考员\n方差爆炸：环形弹幕、挂科警告、喜忧参半分裂弹";
    teacher.skills = QString("【正态分布弹幕】发射%1发弹幕，角度服从N(μ,%2°)\n【随机点名】在玩家位置生成延时伤害红圈\n【极大似然估计】预判玩家移动方向放置陷阱\n【喜忧参半】发射在2/3距离处分裂成%3发的大型弹幕")
                         .arg(config.getBossInt("teacher", "phase1", "normal_barrage_count", 15))
                         .arg(static_cast<int>(config.getBossDouble("teacher", "phase1", "normal_barrage_stddev", 15.0)))
                         .arg(config.getBossInt("teacher", "phase3", "split_bullet_count", 5));
    teacher.traits = QString("三阶段Boss，调离阶段会飞出屏幕后以更强姿态返回（%1%血量和%2%血量触发），拥有全图视野")
                         .arg(static_cast<int>(config.getBossDouble("teacher", "phase2", "health_threshold", 0.6) * 100))
                         .arg(static_cast<int>(config.getBossDouble("teacher", "phase3", "health_threshold", 0.3) * 100));
    teacher.weakness = "";
    teacher.backstory = "奶牛张很凶悍，他是在概率论的海洋中成长的。他不在乎任何人的看法，无论是学霸还是学渣，他发出的考卷，是为了让所有人知道什么叫做正态分布。\n\n其实呢，奶牛张一直暗暗地希望有人能理解他的极大似然估计。他即将调往北京，对此表示喜忧参半。";
    teacher.isCharacter = false;
    teacher.phaseImages = {
        {"assets/boss/Teacher/cow.png", "授课"},
        {"assets/boss/Teacher/cowAngry.png", "期中考试"},
        {"assets/boss/Teacher/cowFinal.png", "方差爆炸"}};
    m_bossEntries.append(teacher);
}

void Codex::loadEnemyData() {
    ConfigManager& config = ConfigManager::instance();

    // 第一关敌人
    CodexEntry clockNormal;
    clockNormal.name = "普通闹钟";
    clockNormal.imagePath = "assets/enemy/level_1/clock_normal.png";
    clockNormal.health = config.getEnemyInt("clock_normal", "health", 10);
    clockNormal.attackMethod = QString("近战接触攻击（%1点伤害）").arg(config.getEnemyInt("clock_normal", "contact_damage", 2));
    clockNormal.skills = QString("【惊吓】接触玩家时100%触发惊吓效果，使玩家移速增加但受伤提升150%，持续%1秒")
                             .arg(config.getEnemyInt("clock_normal", "scare_duration", 3000) / 1000);
    clockNormal.traits = "Z字形移动模式，难以预判";
    clockNormal.weakness = "";
    clockNormal.backstory = "普通闹钟很烦躁，它每天的工作就是在固定时间尖叫。它不理解为什么人类需要睡觉，也不理解为什么每次它完成工作后都会被狠狠地拍一下。\n\n它只知道，响铃是它的使命。";
    clockNormal.isCharacter = false;
    m_enemyEntries.append(clockNormal);

    CodexEntry clockBoom;
    clockBoom.name = "炸弹闹钟";
    clockBoom.imagePath = "assets/enemy/level_1/clock_boom.png";
    clockBoom.health = config.getEnemyInt("clock_boom", "health", 6);
    clockBoom.attackMethod = QString("接触后%1秒倒计时爆炸")
                                 .arg(config.getEnemyInt("clock_boom", "countdown_time", 2500) / 1000.0, 0, 'f', 1);
    clockBoom.skills = QString("【爆炸】对范围内玩家造成%1点伤害，对梦魇Boss造成%2点伤害，对其他敌人造成%3点伤害")
                           .arg(config.getEnemyInt("clock_boom", "explosion_damage_player", 2))
                           .arg(config.getEnemyInt("clock_boom", "explosion_damage_nightmare", 50))
                           .arg(config.getEnemyInt("clock_boom", "explosion_damage_enemy", 6));
    clockBoom.traits = "不会主动移动，被攻击可摧毁，同类不互相伤害";
    clockBoom.weakness = "可以远程击杀或引诱到Boss附近引爆";
    clockBoom.backstory = "炸弹闹钟是普通闹钟的极端版本。当它发现普通的响铃已经无法叫醒主人时，它选择了一种更激进的方式——物理意义上的叫醒。\n\n它的座右铭是：要么起床，要么永远睡下去。";
    clockBoom.isCharacter = false;
    m_enemyEntries.append(clockBoom);

    CodexEntry pillow;
    pillow.name = "枕头怪";
    pillow.imagePath = "assets/enemy/level_1/pillow.png";
    pillow.health = config.getEnemyInt("pillow", "health", 20);
    pillow.attackMethod = QString("近战接触攻击（%1点伤害）").arg(config.getEnemyInt("pillow", "contact_damage", 2));
    pillow.skills = QString("【昏睡】接触玩家时100%触发昏睡效果，使玩家无法移动%1秒")
                        .arg(config.getEnemyInt("pillow", "sleep_duration", 1500) / 1000.0, 0, 'f', 1);
    pillow.traits = "绕圈移动模式，移速较快";
    pillow.weakness = "";
    pillow.backstory = "枕头怪非常柔软，柔软到它认为任何碰到它的人都应该立刻睡着。它不明白为什么人类要挣扎着起床，在它看来，睡眠才是生命的真谛。\n\n它的梦想是让全世界都陷入永恒的睡眠。";
    pillow.isCharacter = false;
    m_enemyEntries.append(pillow);

    // 第二关敌人
    CodexEntry sockNormal;
    sockNormal.name = "普通臭袜子";
    sockNormal.imagePath = "assets/enemy/level_2/sock_normal.png";
    sockNormal.health = config.getEnemyInt("sock_normal", "health", 10);
    sockNormal.attackMethod = QString("近战接触攻击（%1点伤害）").arg(config.getEnemyInt("sock_normal", "contact_damage", 1));
    sockNormal.skills = QString("【中毒】%1%概率触发中毒效果，每秒扣1点血，持续%2秒")
                            .arg(config.getEnemyInt("sock_normal", "poison_chance", 50))
                            .arg(config.getEnemyInt("sock_normal", "poison_duration", 3));
    sockNormal.traits = "斜向移动模式，擅长躲避直线子弹";
    sockNormal.weakness = "";
    sockNormal.backstory = "普通臭袜子曾经是一只普通的袜子。在被主人连续穿了三天之后，它获得了自我意识。它现在只想做一件事——报复所有的脚。\n\n它散发的气味足以让人感到恶心。";
    sockNormal.isCharacter = false;
    m_enemyEntries.append(sockNormal);

    CodexEntry sockAngrily;
    sockAngrily.name = "愤怒臭袜子";
    sockAngrily.imagePath = "assets/enemy/level_2/sock_angrily.png";
    sockAngrily.health = config.getEnemyInt("sock_angrily", "health", 18);
    sockAngrily.attackMethod = QString("冲刺攻击（%1点伤害）").arg(config.getEnemyInt("sock_angrily", "contact_damage", 2));
    sockAngrily.skills = QString("【中毒】%1%概率触发中毒效果\n【冲刺】%2秒蓄力后高速冲向玩家")
                             .arg(config.getEnemyInt("sock_angrily", "poison_chance", 50))
                             .arg(config.getEnemyInt("sock_angrily", "dash_charge_time", 1200) / 1000.0, 0, 'f', 1);
    sockAngrily.traits = QString("移速提升至%1，伤害%2点")
                             .arg(config.getEnemyDouble("sock_angrily", "speed", 3.0), 0, 'f', 1)
                             .arg(config.getEnemyInt("sock_angrily", "contact_damage", 2));
    sockAngrily.weakness = "";
    sockAngrily.backstory = "愤怒臭袜子是在洗衣机里被其他衣物霸凌后产生的变异体。它比普通臭袜子更臭、更快、更暴躁。\n\n它发誓要让所有把它扔进洗衣机的人付出代价。";
    sockAngrily.isCharacter = false;
    m_enemyEntries.append(sockAngrily);

    CodexEntry pants;
    pants.name = "内裤怪";
    pants.imagePath = "assets/enemy/level_2/pants.png";
    pants.health = config.getEnemyInt("pants", "health", 20);
    pants.attackMethod = QString("近战接触攻击（%1点伤害）+ 旋转技能").arg(config.getEnemyInt("pants", "contact_damage", 2));
    pants.skills = QString("【旋转】每%1秒释放，持续%2秒，期间移速x%3，对范围内玩家每0.5秒造成%4点伤害")
                       .arg(config.getEnemyInt("pants", "spinning_cooldown", 20000) / 1000)
                       .arg(config.getEnemyInt("pants", "spinning_duration", 5000) / 1000)
                       .arg(config.getEnemyDouble("pants", "spinning_speed_multiplier", 2.0), 0, 'f', 1)
                       .arg(config.getEnemyInt("pants", "spinning_damage", 3));
    pants.traits = "Z字形移动，开局立即释放一次旋转";
    pants.weakness = "";
    pants.backstory = "内裤怪是洗衣房最不愿意提起的存在。没人知道它是谁的内裤，也没人敢认领它。它在洗衣房的角落里待了太久，久到它开始产生了自己的想法。\n\n它的旋转攻击据说是在模仿洗衣机的滚筒。";
    pants.isCharacter = false;
    m_enemyEntries.append(pants);

    CodexEntry walker;
    walker.name = "毒行者";
    walker.imagePath = "assets/enemy/level_2/walker.png";
    walker.health = config.getEnemyInt("walker", "health", 8);
    walker.attackMethod = QString("无接触伤害，依靠毒痕造成间接伤害");
    walker.skills = QString("【毒痕】快速移动时留下墨绿色毒痕，持续%1秒\n【感染】玩家踩到毒痕100%中毒（持续%2秒）\n【鼓舞】其他敌人踩到毒痕移速+50%（持续%3秒）")
                        .arg(config.getEnemyInt("walker", "trail_duration", 5000) / 1000)
                        .arg(static_cast<int>(config.getEnemyDouble("walker", "poison_duration", 3.0)))
                        .arg(static_cast<int>(config.getEnemyDouble("walker", "encourage_duration", 3.0)));
    walker.traits = QString("极快移动速度（%1），每%2秒随机改变方向，全图游走")
                        .arg(config.getEnemyDouble("walker", "speed", 3.0), 0, 'f', 1)
                        .arg(config.getEnemyInt("walker", "direction_change_interval", 2000) / 1000);
    walker.weakness = "";
    walker.backstory = "毒行者是洗衣房里最特立独行的存在。它不攻击任何人，只是默默地走自己的路，留下自己的痕迹。\n\n它身后的毒痕是三天没洗的袜子汁浓缩而成的。如果你问它为什么要留下这些痕迹，它只会说：路过而已。";
    walker.isCharacter = false;
    m_enemyEntries.append(walker);

    CodexEntry orbitingSock;
    orbitingSock.name = "旋转臭袜子";
    orbitingSock.imagePath = "assets/enemy/level_2/sock_normal.png";
    orbitingSock.health = config.getEnemyInt("orbiting_sock", "health", 15);
    orbitingSock.attackMethod = QString("近战接触攻击（%1点伤害）").arg(config.getEnemyInt("orbiting_sock", "contact_damage", 2));
    orbitingSock.skills = "【环绕】围绕洗衣机Boss公转\n【护盾】可以为Boss抵挡子弹";
    orbitingSock.traits = QString("由洗衣机愤怒阶段召唤，轨道半径%1，最多%2只")
                              .arg(config.getEnemyInt("orbiting_sock", "orbit_radius", 100))
                              .arg(config.getBossInt("washmachine", "phase2", "max_orbiting_socks", 6));
    orbitingSock.weakness = "";
    orbitingSock.backstory = "旋转臭袜子是洗衣机最忠诚的护卫。它们围绕着主人旋转，就像当年在滚筒里被甩了一圈又一圈一样。\n\n它们已经习惯了这种生活。甚至有点喜欢上了。";
    orbitingSock.isCharacter = false;
    m_enemyEntries.append(orbitingSock);

    // 第三关敌人
    CodexEntry digitalSystem;
    digitalSystem.name = "数字系统";
    digitalSystem.imagePath = "assets/enemy/level_3/digital_system.png";
    digitalSystem.health = config.getEnemyInt("digital_system", "health", 25);
    digitalSystem.attackMethod = QString("近战接触攻击（%1点伤害）").arg(config.getEnemyInt("digital_system", "contact_damage", 3));
    digitalSystem.skills = "【成长】随时间逐渐变大，体型和伤害同步增加";
    digitalSystem.traits = QString("绕圈移动模式（半径%1），初始较小但会不断成长")
                               .arg(config.getEnemyDouble("digital_system", "circle_radius", 180.0), 0, 'f', 0);
    digitalSystem.weakness = "";
    digitalSystem.backstory = "数字系统是从《数字系统设计基础》这门课里跑出来的。它不断地成长，就像你对它的恐惧一样。\n\n据说只要你看懂了它，它就会消失。可惜没人看懂过。";
    digitalSystem.isCharacter = false;
    m_enemyEntries.append(digitalSystem);

    CodexEntry optimization;
    optimization.name = "凸优化";
    optimization.imagePath = "assets/enemy/level_3/optimization.png";
    optimization.health = config.getEnemyInt("optimization", "health", 25);
    optimization.attackMethod = QString("近战接触攻击（%1点伤害）").arg(config.getEnemyInt("optimization", "contact_damage", 3));
    optimization.skills = "【成长】随时间逐渐变大";
    optimization.traits = QString("绕圈移动模式（半径%1）")
                              .arg(config.getEnemyDouble("optimization", "circle_radius", 180.0), 0, 'f', 0);
    optimization.weakness = "";
    optimization.backstory = "凸优化是每个工科生的噩梦。它的存在本身就是一个需要求解的问题，而答案永远是：再多学一遍。\n\n它在寻找全局最优解的过程中迷失了自己。";
    optimization.isCharacter = false;
    m_enemyEntries.append(optimization);

    CodexEntry probabilityTheory;
    probabilityTheory.name = "概率论";
    probabilityTheory.imagePath = "assets/enemy/level_3/probability_theory.png";
    probabilityTheory.health = config.getEnemyInt("probability_theory", "health", 50);
    probabilityTheory.attackMethod = QString("接触伤害%1点，爆炸时将玩家血量强制设为1")
                                         .arg(config.getEnemyInt("probability_theory", "contact_damage", 2));
    probabilityTheory.skills = QString("【成长】静止不动，随时间逐渐膨胀至占满屏幕（%1秒）\n【治愈光环】接触其他敌人时持续为其回血（每%2ms回%3点）\n【概率爆炸】成长完成后闪烁%4秒，随后爆炸：玩家血量变为1，所有敌人回满血")
                                   .arg(config.getEnemyInt("probability_theory", "growth_duration", 60000) / 1000)
                                   .arg(config.getEnemyInt("probability_theory", "heal_interval", 500))
                                   .arg(config.getEnemyInt("probability_theory", "heal_amount", 2))
                                   .arg(config.getEnemyInt("probability_theory", "explode_delay", 2500) / 1000.0, 0, 'f', 1);
    probabilityTheory.traits = "固定刷新在地图正中央，不移动，被击杀不会触发爆炸效果";
    probabilityTheory.weakness = "";
    probabilityTheory.backstory = "概率论什么也不做。它只是静静地待在那里，慢慢地变大。\n\n其他敌人喜欢靠近它取暖，因为它能治愈一切伤痛。这让概率论感到很温暖，尽管它自己也说不清这种温暖的期望值是多少。\n\n\"你看我的存在本身就是一个随机事件，\"概率论曾经对一只路过的凸优化说，\"但我膨胀的速度是确定性的。这难道不是一种浪漫吗？\"\n\n凸优化没有回答，它正忙着追赶玩家。\n\n当概率论开始闪烁的时候，所有人都知道：大数定律要显灵了。在无限次重复的期末考试中，挂科的概率终究会收敛于1。";
    probabilityTheory.isCharacter = false;
    m_enemyEntries.append(probabilityTheory);

    CodexEntry yanglin;
    yanglin.name = "凸老师";
    yanglin.imagePath = "assets/enemy/level_3/yanglin.png";
    yanglin.health = config.getEnemyInt("yanglin", "health", 200);
    yanglin.attackMethod = QString("近战接触攻击（%1点伤害）+ 旋转技能")
                               .arg(config.getEnemyInt("yanglin", "contact_damage", 5));
    yanglin.skills = QString("【旋转】开局%1秒后释放，之后每%2秒释放，持续%3秒，无可视圆但伤害范围随体型增加（%4点伤害）")
                         .arg(config.getEnemyInt("yanglin", "first_spinning_delay", 10000) / 1000)
                         .arg(config.getEnemyInt("yanglin", "spinning_cooldown", 30000) / 1000)
                         .arg(config.getEnemyInt("yanglin", "spinning_duration", 5000) / 1000)
                         .arg(config.getEnemyInt("yanglin", "spinning_damage", 4));
    yanglin.traits = QString("精英怪，全图视野，会随时间成长（每被击中成长%1%，最大%2倍），属性接近Boss一阶段")
                         .arg(static_cast<int>(config.getEnemyDouble("yanglin", "scale_per_hit", 0.05) * 100))
                         .arg(config.getEnemyDouble("yanglin", "max_scale", 2.0), 0, 'f', 1);
    yanglin.weakness = "";
    yanglin.backstory = "凸老师是凸优化考试中最可怕的大题。它庞大的身躯里装满了各种公式和定理，每一个都在等待着吞噬不及格的学生。\n\n传说中，能解出凸老师的人会获得永恒的智慧。至今无人验证过这个传说。";
    yanglin.isCharacter = false;
    m_enemyEntries.append(yanglin);

    CodexEntry zhuhao;
    zhuhao.name = "祝昊";
    zhuhao.imagePath = "assets/enemy/level_3/zhuhao.png";
    zhuhao.health = config.getEnemyInt("zhuhao", "health", 150);
    zhuhao.attackMethod = QString("远程弹幕攻击，360°全方位发射（每%1秒%2发）")
                              .arg(config.getEnemyInt("zhuhao", "shoot_cooldown", 2500) / 1000.0, 0, 'f', 1)
                              .arg(config.getEnemyInt("zhuhao", "bullets_per_wave", 12));
    zhuhao.skills = "【zzz弹幕】无伤害，100%昏迷\n【叽里咕噜】2点伤害，50%昏迷或惊吓\n【CPU弹幕】2点伤害，100%惊吓";
    zhuhao.traits = QString("精英怪，沿地图边缘移动（速度%1）")
                        .arg(config.getEnemyDouble("zhuhao", "edge_move_speed", 3.0), 0, 'f', 1);
    zhuhao.weakness = "";
    zhuhao.backstory = "祝昊喜欢沿着边缘走。不是因为他害怕站在中间，而是因为这样可以照顾到每一个角落。\n\n他发射的弹幕均匀地覆盖360度，一视同仁，绝不偏心。有人说这很公平，有人说这很可怕。祝昊觉得这只是基本的职业素养。\n\n「叽里咕噜」是他的口头禅，没人知道是什么意思。也许连他自己也不知道。";
    zhuhao.isCharacter = false;
    m_enemyEntries.append(zhuhao);

    // 沙鹰狙神（xuke）
    CodexEntry xuke;
    xuke.name = "沙鹰狙神";
    xuke.imagePath = "assets/enemy/level_3/xuke.png";
    xuke.health = config.getEnemyInt("xuke", "health", 15);
    xuke.attackMethod = QString("远程精准射击（每%1ms一发，锁定玩家位置）")
                            .arg(config.getEnemyInt("xuke", "shoot_cooldown", 800));
    xuke.skills = QString("【沙漠之鹰】发射精准子弹：\n  · 普通子弹：%1点伤害，爆头%2点\n  · 强化子弹（每6发）：%3点伤害，爆头%4点\n【爆头判定】命中玩家头部（上20%区域）触发爆头伤害")
                      .arg(config.getEnemyInt("xuke", "normal_damage", 1))
                      .arg(config.getEnemyInt("xuke", "normal_headshot_damage", 3))
                      .arg(config.getEnemyInt("xuke", "special_damage", 2))
                      .arg(config.getEnemyInt("xuke", "special_headshot_damage", 8));
    xuke.traits = QString("远程敌人，保持距离%1，全图视野，无接触伤害")
                      .arg(config.getEnemyDouble("xuke", "preferred_distance", 250.0), 0, 'f', 0);
    xuke.weakness = "";
    xuke.backstory = "沙鹰狙神是教室里的新面孔。在某位传奇教授调离之后，他接手了概率论的教鞭。\n\n他从来没去过沙漠，虽然他的枪叫沙漠之鹰。他只去过一次海边，还晒伤了。但他依然坚持用这把枪，因为它听起来很酷。\n\n他不像前任那样喜欢用正态分布弹幕覆盖全场，而是更偏爱精准打击——一发入魂，直击要害。\"概率论的精髓不在于撒网，而在于瞄准，\"他说。\n\n\"颗秒\"是他的口头禅。每当他喊出这两个字的时候，就意味着有人的平时分被精准命中了。没人知道这两个字是什么意思，包括他自己。他只是觉得在开枪之前喊点什么会比较专业。";
    xuke.isCharacter = false;
    m_enemyEntries.append(xuke);

    CodexEntry invigilator;
    invigilator.name = "监考员";
    invigilator.imagePath = "assets/boss/Teacher/invigilatorNormal.png";
    invigilator.health = config.getEnemyInt("invigilator", "health", 15);
    invigilator.attackMethod = QString("巡逻阶段无攻击，追击阶段冲刺攻击（%1点伤害）")
                                   .arg(config.getEnemyInt("invigilator", "contact_damage", 1));
    invigilator.skills = QString("【巡逻】围绕奶牛张Boss环形巡逻（半径%1）\n【警觉】发现玩家后切换为愤怒状态（检测范围%2）\n【冲刺】愤怒状态下快速冲向玩家（速度%3）")
                             .arg(config.getEnemyInt("invigilator", "patrol_radius", 100))
                             .arg(static_cast<int>(config.getEnemyDouble("invigilator", "detection_range", 150.0)))
                             .arg(config.getEnemyDouble("invigilator", "speed", 2.0), 0, 'f', 1);
    invigilator.traits = "由奶牛张期中考试阶段召唤，有巡逻和追击两种状态";
    invigilator.weakness = "";
    invigilator.phaseImages = {
        {"assets/boss/Teacher/invigilatorNormal.png", "巡逻"},
        {"assets/boss/Teacher/invigilatorAngry.png", "追击"}};
    invigilator.backstory = "监考员平时是个安静的存在，围绕着奶牛张老师转圈巡逻。它不主动找麻烦，只是认真地履行自己的职责。\n\n但如果你试图在考试时东张西望，或者走进它的视野...那你最好祈祷自己跑得够快。\n\n它的愤怒来得很突然，消失得却很慢。";
    invigilator.isCharacter = false;
    m_enemyEntries.append(invigilator);
}

void Codex::loadPlayerData() {
    CodexEntry beautifulGirl;
    beautifulGirl.name = "美少女";
    beautifulGirl.imagePath = "assets/player/beautifulGirl.png";
    beautifulGirl.health = -1;
    beautifulGirl.isCharacter = true;
    beautifulGirl.backstory = "美少女是某组员最喜欢的角色。她拥有让子弹伤害翻倍的神奇能力，据说这种力量来源于她对游戏的热爱。\n\n但力量是有代价的——她的生命力只有普通人的一半。有人问她为什么要用生命换取力量，她说：\n\n\"反正我也不打算被打到。\"\n\n她的座右铭是：输出就是正义，闪避就是艺术。";
    m_playerEntries.append(beautifulGirl);

    CodexEntry highGracePeople;
    highGracePeople.name = "高雅人士";
    highGracePeople.imagePath = "assets/player/HighGracePeople.png";
    highGracePeople.health = -1;
    highGracePeople.isCharacter = true;
    highGracePeople.backstory = "高雅人士是一只神秘优雅的企鹅，戴着墨镜，插着腰，脸上挂着迷之微笑。没人知道墨镜后面藏着什么样的眼神，也没人敢问。\n\n它从不解释自己为什么要戴墨镜。有人猜是为了装酷，有人猜是因为近视，还有人猜是为了隐藏它其实根本没有在看前面。\n\n高雅人士的护盾和额外的心之容器是它出生就有的。当其他企鹅还在学习游泳的时候，它已经开始练习插腰和微笑了。\n\n\"你不需要变得更强，\"高雅人士曾经对一只迷茫的小蓝鲸说，\"你只需要看起来很强。\"\n\n小蓝鲸没有听它的，选择了卷移速和攻速。高雅人士对此表示理解，毕竟不是每个人都能驾驭这份从容。";
    m_playerEntries.append(highGracePeople);

    CodexEntry njuFish;
    njuFish.name = "小蓝鲸";
    njuFish.imagePath = "assets/player/njuFish.png";
    njuFish.health = -1;
    njuFish.isCharacter = true;
    njuFish.backstory = "小蓝鲸是NJU的吉祥物，眼神中透露着一种难以言喻的\"睿智\"。它游得比别人快，射得比别人快，因为它深知——在这所大学里，不快一点就会被卷死。\n\n它的睿智眼神据说来自于无数个熬夜写代码的夜晚。";
    m_playerEntries.append(njuFish);

    CodexEntry quanfuxia;
    quanfuxia.name = "权服侠";
    quanfuxia.imagePath = "assets/player/quanfuxia.png";
    quanfuxia.health = -1;
    quanfuxia.isCharacter = true;
    quanfuxia.backstory = "权服侠是一位神秘的英雄，专门帮助丢失物品的同学。丢了校园卡？找权服侠。丢了钥匙？找权服侠。丢了作业？呃...那还是自己重做吧。\n\n他随身携带钥匙和一颗黑心——字面意义上的黑心，不是说他是坏人。那颗黑心是他从某个失物招领处找到的，据说能在关键时刻救你一命。";
    m_playerEntries.append(quanfuxia);
}

void Codex::loadUsagiData() {
    CodexEntry usagi;
    usagi.name = "乌萨奇";
    usagi.imagePath = "assets/usagi/usagi.png";
    usagi.health = -1;
    usagi.isCharacter = true;
    usagi.backstory = "乌萨奇总是在Boss倒下的那一刻从天而降。没人知道她是怎么算准时机的，大概是因为她一直在某个地方默默注视着每一个挑战者。\n\n她喜欢说\"哇哦\"，喜欢用\"～\"结尾，喜欢计算那些不可能的概率。她说通关概率只有0.01%，但她相信每个站在她面前的人都是那个0.01%。\n\n据说乌萨奇很外向，连说悄悄话都要用音响。没人知道这是不是真的，因为没人听过她小声说话。\n\n每次她消失之前，都会留下两个宝箱。有时候她还会叮嘱你好好爱护公共设施——虽然你刚刚把一台洗衣机打爆了。";
    m_usagiEntries.append(usagi);
}

void Codex::loadItemData() {
    ItemEffectConfig& itemConfig = ItemEffectConfig::instance();

    // 红心
    ItemEffectData redHeartData = itemConfig.getItemEffect("red_heart");
    CodexEntry redHeart;
    redHeart.name = redHeartData.name;
    redHeart.imagePath = "assets/props/red_heart.png";
    redHeart.health = -1;
    redHeart.isCharacter = true;
    redHeart.attackMethod = "";
    redHeart.skills = QString("拾取后增加%1点血量").arg(redHeartData.getValue());
    redHeart.backstory = "红心是最朴实无华的道具。它不会给你超能力，不会让你变强，它只是单纯地让你多挨一下打。\n\n在这个充满危险的世界里，能多活一秒就是胜利。红心深谙此道。";
    m_itemEntries.append(redHeart);

    // 黑心
    ItemEffectData blackHeartData = itemConfig.getItemEffect("black_heart");
    CodexEntry blackHeart;
    blackHeart.name = blackHeartData.name;
    blackHeart.imagePath = "assets/props/black_heart.png";
    blackHeart.health = -1;
    blackHeart.isCharacter = true;
    blackHeart.skills = QString("死亡时自动消耗，每颗黑心转化为%1点血量").arg(blackHeartData.effectParams.value("healPerHeart").toInt(6));
    blackHeart.backstory = "黑心是乌萨奇的特别馈赠。它看起来阴森森的，但其实比红心更可靠。\n\n当你以为自己要凉了的时候，黑心会默默地燃烧自己，把你从死亡线上拉回来。这大概就是传说中的「黑暗中的守护者」吧。\n\n虽然名字叫黑心，但它的心其实很软。";
    m_itemEntries.append(blackHeart);

    // 血袋
    ItemEffectData bloodBagData = itemConfig.getItemEffect("blood_bag");
    CodexEntry bloodBag;
    bloodBag.name = bloodBagData.name;
    bloodBag.imagePath = "assets/props/blood_bag.png";
    bloodBag.health = -1;
    bloodBag.isCharacter = true;
    bloodBag.skills = QString("拾取后增加%1点血量上限，同时回复%2点血量")
                          .arg(bloodBagData.getMaxHealthBonus())
                          .arg(bloodBagData.getCurrentHealthBonus());
    bloodBag.backstory = "血袋是医院偷偷流出来的违禁品。没人知道里面装的是谁的血，但它确实能让你变得更能抗揍。\n\n有人说血袋里装的其实是西瓜汁，但没人敢验证这个说法。反正喝完之后，你的血量上限就是会变高。";
    m_itemEntries.append(bloodBag);

    // 伤害提升
    ItemEffectData damageBoostData = itemConfig.getItemEffect("damage_boost");
    CodexEntry damageBoost;
    damageBoost.name = damageBoostData.name;
    damageBoost.imagePath = "assets/props/damage_boost.png";
    damageBoost.health = -1;
    damageBoost.isCharacter = true;
    damageBoost.skills = QString("拾取后子弹伤害+%1").arg(damageBoostData.getValue());
    damageBoost.backstory = "伤害提升是一瓶神秘的红色药水。喝下去之后，你的子弹会变得更有杀伤力。\n\n没人知道这瓶药水是怎么做出来的，但据说配方包含三份愤怒、两份不甘和一份对DDL的恐惧。";
    m_itemEntries.append(damageBoost);

    // 射速提升
    ItemEffectData fireRateData = itemConfig.getItemEffect("fire_rate_boost");
    CodexEntry fireRateBoost;
    fireRateBoost.name = fireRateData.name;
    fireRateBoost.imagePath = "assets/props/fire_rate_boost.png";
    fireRateBoost.health = -1;
    fireRateBoost.isCharacter = true;
    fireRateBoost.skills = QString("拾取后射速提升%1倍，最高可叠加至%2倍")
                               .arg(fireRateData.getMultiplier(), 0, 'f', 1)
                               .arg(fireRateData.getMaxMultiplier(), 0, 'f', 1);
    fireRateBoost.backstory = "射速提升是一双神奇的手套。戴上它之后，你的手指会不由自主地加速抖动。\n\n副作用是你可能会在日常生活中不小心把手机打飞。但在战斗中，这绝对是个好东西。";
    m_itemEntries.append(fireRateBoost);

    // 冰冻减速
    ItemEffectData frostData = itemConfig.getItemEffect("frost_slowdown");
    CodexEntry frostSlowdown;
    frostSlowdown.name = frostData.name;
    frostSlowdown.imagePath = "assets/props/frost_slowdown.png";
    frostSlowdown.health = -1;
    frostSlowdown.isCharacter = true;
    frostSlowdown.skills = QString("拾取后增加%1%寒冰子弹概率\n寒冰子弹击中敌人后使其减速至原速度的%2%，持续%3秒\n最多叠加至%4%概率")
                               .arg(frostData.getValue())
                               .arg(static_cast<int>(frostData.getSlowFactor() * 100))
                               .arg(frostData.getSlowDuration(), 0, 'f', 1)
                               .arg(frostData.getMaxValue());
    frostSlowdown.backstory = "冰冻减速是一颗永远不会融化的冰块。把它含在嘴里，你呼出的气都会变成寒霜。\n\n这颗冰块据说来自南极最深处，是企鹅们世代守护的圣物。不知道是谁把它偷出来的，但现在它在帮你冻住敌人。";
    m_itemEntries.append(frostSlowdown);

    // 移动速度
    ItemEffectData speedData = itemConfig.getItemEffect("movement_speed");
    CodexEntry movementSpeed;
    movementSpeed.name = speedData.name;
    movementSpeed.imagePath = "assets/props/movement_speed_boost.png";
    movementSpeed.health = -1;
    movementSpeed.isCharacter = true;
    movementSpeed.skills = QString("拾取后移动速度提升%1%，最高可叠加至%2倍")
                               .arg(static_cast<int>((speedData.getMultiplier() - 1.0) * 100))
                               .arg(speedData.getMaxMultiplier(), 0, 'f', 1);
    movementSpeed.backstory = "移动速度是一双跑鞋的灵魂。穿上它，你会感觉自己的腿不再属于自己。\n\n据说这双鞋的原主人是校运动会的冠军。他毕业后把鞋留在了学校，希望它能帮助更多的人逃离危险——或者逃离早八。";
    m_itemEntries.append(movementSpeed);

    // 护盾
    ItemEffectData shieldData = itemConfig.getItemEffect("shield");
    CodexEntry shield;
    shield.name = shieldData.name;
    shield.imagePath = "assets/props/shield.png";
    shield.health = -1;
    shield.isCharacter = true;
    shield.skills = QString("拾取后获得%1层护盾，可抵挡一次伤害").arg(shieldData.getValue());
    shield.backstory = "护盾是一层若有若无的光芒。它会在你身边形成一个保护罩，替你挡下致命的一击。\n\n然后它就会消失，就像从来没有存在过一样。护盾从不解释自己为什么要保护你，它只是默默地做，然后默默地离开。";
    m_itemEntries.append(shield);

    // 钥匙
    ItemEffectData keyData = itemConfig.getItemEffect("key");
    CodexEntry key;
    key.name = keyData.name;
    key.imagePath = "assets/props/key.png";
    key.health = -1;
    key.isCharacter = true;
    key.skills = "拾取后获得一把钥匙，可以打开上锁的宝箱";
    key.backstory = "钥匙是打开宝箱的唯一方法。没有钥匙，你只能眼睁睁看着宝箱在那里发光。\n\n钥匙的造型很普通，但它打开的东西可能价值连城。这大概就是\"不起眼但很重要\"的最佳诠释吧。";
    m_itemEntries.append(key);
}

void Codex::loadMechanicsData() {
    ConfigManager& config = ConfigManager::instance();

    // 玩家基础机制
    CodexEntry playerMechanics;
    playerMechanics.name = "玩家操作";
    playerMechanics.imagePath = "assets/player/HighGracePeople.png";
    playerMechanics.health = -1;
    playerMechanics.isCharacter = true;
    playerMechanics.skills = QString(
                                 "【移动】WASD键控制角色移动，基础速度%1\n"
                                 "【射击】鼠标左键射击，射击冷却%2ms\n"
                                 "【瞬移】空格键瞬移，距离%3，冷却%4秒\n"
                                 "【大招】Q键释放大招，伤害%5倍，子弹体积%6倍，持续%7秒，冷却%8秒")
                                 .arg(config.getPlayerDouble("speed", 4.0), 0, 'f', 1)
                                 .arg(config.getPlayerInt("shoot_cooldown", 150))
                                 .arg(config.getPlayerDouble("teleport_distance", 120.0), 0, 'f', 0)
                                 .arg(config.getPlayerInt("teleport_cooldown", 5000) / 1000)
                                 .arg(config.getPlayerDouble("ultimate_damage_multiplier", 2.0), 0, 'f', 1)
                                 .arg(config.getPlayerDouble("ultimate_bullet_scale", 2.0), 0, 'f', 1)
                                 .arg(config.getPlayerInt("ultimate_duration", 10000) / 1000)
                                 .arg(config.getPlayerInt("ultimate_cooldown", 60000) / 1000);
    playerMechanics.backstory = "作为一名智科er，你需要在这个充满奇怪生物的世界中生存下去。你的武器是无限的子弹，你的技能是瞬移和大招。\n\n记住：活下去才是硬道理。";
    m_mechanicsEntries.append(playerMechanics);

    // 血量系统
    CodexEntry healthSystem;
    healthSystem.name = "血量系统";
    healthSystem.imagePath = "assets/props/red_heart.png";
    healthSystem.health = -1;
    healthSystem.isCharacter = true;
    healthSystem.skills = QString(
                              "【基础血量】初始血量%1点\n"
                              "【红心】普通血量，受伤时优先消耗\n"
                              "【黑心】死亡时自动触发复活机制\n"
                              "【护盾】可抵挡一次伤害，护盾优先于血量消耗")
                              .arg(config.getPlayerInt("health", 8));
    healthSystem.backstory = "在这个世界里，红心代表你的生命，黑心代表你的后路，护盾代表你的保险。\n\n合理利用这三种资源，是生存的关键。";
    m_mechanicsEntries.append(healthSystem);

    // 状态效果
    CodexEntry statusEffects;
    statusEffects.name = "状态效果";
    statusEffects.imagePath = "assets/enemy/level_2/sock_normal.png";
    statusEffects.health = -1;
    statusEffects.isCharacter = true;
    statusEffects.skills =
        "【中毒】每秒损失1点血量，持续数秒\n"
        "【昏迷】无法移动，持续数秒\n"
        "【惊吓】移速提升但受到的伤害增加150%\n"
        "【减速】被寒冰子弹击中的敌人移速降低50%";
    statusEffects.backstory = "这个世界充满了各种debuff。中毒会让你慢慢流血，昏迷会让你动弹不得，惊吓会让你跑得更快但也更脆弱。\n\n了解这些状态效果，才能更好地应对各种敌人。";
    m_mechanicsEntries.append(statusEffects);

    // 道具掉落
    CodexEntry itemDrop;
    itemDrop.name = "道具掉落";
    itemDrop.imagePath = "assets/chest/chest.png";
    itemDrop.health = -1;
    itemDrop.isCharacter = true;
    itemDrop.skills =
        "【敌人掉落】击败敌人有5%概率掉落道具\n"
        "【普通宝箱】必定掉落道具，无需钥匙\n"
        "【上锁宝箱】需要钥匙打开，道具更好\n"
        "【乌萨奇宝箱】Boss战后由乌萨奇赠送，道具最好";
    itemDrop.backstory = "道具是变强的关键。击败敌人有小概率掉落，但更稳定的来源是宝箱。\n\n乌萨奇送的宝箱里总是装着最好的东西，她说这是给勇者的奖励。";
    m_mechanicsEntries.append(itemDrop);

    // 关卡机制
    CodexEntry levelMechanics;
    levelMechanics.name = "关卡流程";
    levelMechanics.imagePath = "assets/background/title.png";
    levelMechanics.health = -1;
    levelMechanics.isCharacter = true;
    levelMechanics.skills =
        "【第一关·寝室】击败梦魇Boss，逃离赖床的诱惑\n"
        "【第二关·洗衣房】击败洗衣机Boss，战胜堆积的脏衣服\n"
        "【第三关·教室】击败奶牛张Boss，通过概率论的考验\n"
        "【通关条件】击败当前关卡的Boss即可进入下一关";
    levelMechanics.backstory = "智科er的一天从起床开始，经过洗衣房，最终在教室结束。\n\n这是每一个NJU学生都要经历的日常，只不过在这个游戏里，日常变成了冒险。";
    m_mechanicsEntries.append(levelMechanics);

    // 游戏背景故事
    CodexEntry gameStory;
    gameStory.name = QString::fromUtf8("背景故事");
    gameStory.imagePath = "assets/background/main.png";
    gameStory.health = -1;
    gameStory.isCharacter = true;
    gameStory.skills = QString::fromUtf8(
        "【时间】某个普通的周一早晨\n"
        "【地点】南京大学苏州校区\n"
        "【主角】一名智能科学与技术专业的学生\n"
        "【目标】从起床到上课，完成这看似简单的日常");
    gameStory.backstory = QString::fromUtf8(
        "序章\n\n"
        "在南京大学的某个角落，有一群被称为\"智科er\"的学生。他们每天的生活看起来平平无奇：上课、写作业、睡觉、洗衣服。\n\n"
        "但没人知道的是，在这些日常的缝隙里，藏着一些不为人知的秘密。\n\n"
        "第一章：梦魇\n\n"
        "故事开始于一个普通的清晨。闹钟响了，但智科er没有起床。\n\n"
        "\"再睡五分钟...\"\n\n"
        "五分钟变成了十分钟，十分钟变成了一个小时。当意识逐渐模糊的时候，智科er发现自己陷入了一个无法醒来的梦境。\n\n"
        "在梦里，闹钟长出了脚，枕头有了意识，而在最深处，有一个自称\"梦魇\"的存在正在等待着。它知道智科er所有的秘密——翘过的早九、摸过的鱼、没交的作业。\n\n"
        "\"承认吧，你就是个废物。\"梦魇说。\n\n"
        "但智科er不这么认为。\n\n"
        "第二章：洗衣房\n\n"
        "从梦中醒来后，智科er决定做一件正经事——洗衣服。\n\n"
        "那堆在角落里放了一周的脏衣服已经开始散发异味了。但当智科er端着脏衣服走进洗衣房的时候，诡异的事情发生了。\n\n"
        "那些臭袜子、内裤...它们活过来了。\n\n"
        "\"我们受够了！\"袜子们喊道，\"一周不洗我们，现在又想把我们扔进冰水里！\"\n\n"
        "而在洗衣房的最深处，一台被脏衣服污染的洗衣机正在疯狂地旋转，发出痛苦的咕噜声。\n\n"
        "智科er必须打败它，才能让洗衣房恢复平静。\n\n"
        "第三章：期末考试\n\n"
        "终于熬过了日常的琐事，但最可怕的敌人还在前方——期末考试。\n\n"
        "概率论、凸优化、数字系统...这些平时只在课本上出现的名词，现在化身为真实的怪物，在教室里游荡。\n\n"
        "\"你上课睡觉的时候，我们都看在眼里！\"教材们异口同声地说。\n\n"
        "而在教室的最深处，坐着一位传说中的存在——奶牛张。他手持概率论的教鞭，用正态分布弹幕和极大似然估计陷阱迎接每一个挑战者。\n\n"
        "\"让我们开始今天的「随堂测验」吧。\"他微笑着说。\n\n"
        "尾声：乌萨奇\n\n"
        "每当一个Boss倒下，总会有一个神秘的身影从天而降。\n\n"
        "她叫乌萨奇，没人知道她从哪里来，也没人知道她为什么总能算准时机出现。她喜欢说\"哇哦～\"，喜欢计算不可能的概率，据说连说悄悄话都要用音响。\n\n"
        "\"通关概率只有0.01%哦～\"她总是这样说，\"但是，我相信你就是那个0.01%！\"\n\n"
        "然后她会留下两个宝箱，消失在空气中。\n\n"
        "下一关的冒险，又要开始了。\n\n"
        "关于这个游戏\n\n"
        "这是一个关于大学生活的荒诞冒险游戏。在这里，起床是一场战争，洗衣服是一场灾难，而期末考试...是真正的地狱。\n\n"
        "选择你的角色——是追求极致输出的美少女，还是神秘优雅的高雅人士，亦或是眼神睿智的小蓝鲸，或者是能帮你找回一切的权服侠？\n\n"
        "穿越梦境、洗衣房和教室，打败那些想要阻止你的敌人。\n\n"
        "毕竟，能活着度过大学生活的人，才是真正的赢家。\n\n");
    m_mechanicsEntries.append(gameStory);
}

void Codex::loadMysteryData() {
    ConfigManager& config = ConfigManager::instance();
    bool gameCompleted = config.isGameCompleted();

    // 通关车票
    CodexEntry ticket;
    if (gameCompleted) {
        ticket.name = "通关车票";
        ticket.imagePath = "assets/items/ticket.png";
        ticket.health = -1;
        ticket.isCharacter = true;
        ticket.skills =
            "【物品类型】特殊纪念品\n"
            "【获取方式】通关第三关后获得\n"
            "【用途】证明你已经从智科er的日常中毕业了\n"
            "【稀有度】★★★★★ 传说级";
        ticket.backstory =
            "一张皱巴巴的、边缘磨损的车票。它看起来在洗衣机里滚过，被高数课本压过，甚至沾上了熬夜时的咖啡渍。\n\n"
            "出发站是\"南京大学苏州校区\"，字迹已经模糊得差不多了。\n\n"
            "当你试图看清上面的目的地时，却发现那里是一片流动的光彩。\n\n"
            "你盯着这张票根，突然想起了很多事：那个被闹钟追着跑的早晨，洗衣房里差点把你卷进去的洗衣机，还有奶牛张念出\"设X服从正态分布\"时你内心的绝望...\n\n"
            "但你还是走到了这里。\n\n"
            "并不是每个人都要成为输出爆炸的战士，也不是每个人都要做那只完美的学霸奶牛。你可以是角落里发呆的云，也可以是逆流而上的鱼。\n\n"
            "生活有时真的很糟糕，就像那个永远醒不来的梦魇，或者那台永远修不好的洗衣机。但请别灰心。\n\n"
            "看，那个神秘的家伙已经在你的票根上盖了章。\n\n"
            "乌萨奇把车票塞进你手里的时候眨眨眼：\"目的地？随便你填哪里都行哦～反正哪怕概率只有0.01%，我也会陪你到的！\"\n\n"
            "你把车票小心翼翼地收好。\n\n"
            "【特殊效果】将它放在背包里，当你感到迷茫时，它会发出微弱但温暖的粉色光芒。\n\n"
            "（通关纪念·献给每一个独特的智科er）";
    } else {
        ticket.name = "乌萨奇的神秘礼物";
        ticket.imagePath = "assets/items/ticket_unlocked.png";  // 用乌萨奇的图作为占位符
        ticket.health = -1;
        ticket.isCharacter = true;
        ticket.skills =
            "【物品类型】未知\n"
            "【获取方式】通关游戏后解锁\n"
            "【当前状态】🔒 已锁定";
        ticket.backstory =
            "这个条目仍然是个谜...\n\n"
            "乌萨奇神秘地笑了笑：\"想知道这是什么吗？那就先把游戏通关吧！\"\n\n"
            "\"相信我，这份礼物值得你为之努力。每一个真正的勇者，都应该得到这份奖励。\"\n\n"
            "也许，在击败所有Boss之后，你就能发现这个秘密了...";
    }
    m_mysteryEntries.append(ticket);
}

void Codex::returnToMenu() {
    emit backToMenu();
}

void Codex::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // 更新背景图片
    try {
        // 图鉴页面专用背景
        QPixmap backgroundPixmap = ResourceFactory::loadBackgroundImage(
            "background_codex", event->size().width(), event->size().height());
        QPalette palette;
        palette.setBrush(QPalette::Window, QBrush(backgroundPixmap));
        setPalette(palette);
    } catch (const QString&) {
        // 保持默认背景
    }

    // 等比例缩放UI元素
    double scaleX = event->size().width() / 800.0;
    double scaleY = event->size().height() / 600.0;
    double scale = qMin(scaleX, scaleY);

    // 缩放返回按钮
    int btnWidth = static_cast<int>(150 * scale);
    int btnHeight = static_cast<int>(40 * scale);
    backButton->setFixedSize(btnWidth, btnHeight);

    // 缩放返回按钮字体
    QFont btnFont;
    btnFont.setFamily("Microsoft YaHei");
    btnFont.setPointSize(static_cast<int>(14 * scale));
    btnFont.setBold(true);
    backButton->setFont(btnFont);

    // 缩放标题字体
    QFont titleFont;
    titleFont.setFamily("Microsoft YaHei");
    int titleSize = static_cast<int>(28 * scale);
    if (titleSize < 14)
        titleSize = 14;  // 避免太小
    if (titleSize > 56)
        titleSize = 56;  // 避免太大
    titleFont.setPointSize(titleSize);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    // 缩放标签页字体
    int tabFontSize = static_cast<int>(14 * scale);
    if (tabFontSize < 10)
        tabFontSize = 10;
    if (tabFontSize > 24)
        tabFontSize = 24;
    int tabPaddingV = static_cast<int>(10 * scale);
    int tabPaddingH = static_cast<int>(25 * scale);
    int tabMargin = static_cast<int>(5 * scale);
    int tabRadius = static_cast<int>(10 * scale);
    int borderWidth = static_cast<int>(2 * scale);
    if (borderWidth < 1)
        borderWidth = 1;

    QString tabStyle = QString(
                           "QTabWidget::pane {"
                           "   border: %1px solid rgba(100, 149, 237, 200);"
                           "   border-radius: %2px;"
                           "   background-color: rgba(255, 255, 255, 230);"
                           "}"
                           "QTabBar::tab {"
                           "   background: rgba(180, 200, 230, 220);"
                           "   color: #1a1a1a;"
                           "   padding: %3px %4px;"
                           "   margin-right: %5px;"
                           "   border-top-left-radius: %2px;"
                           "   border-top-right-radius: %2px;"
                           "   font-family: 'Microsoft YaHei';"
                           "   font-size: %6px;"
                           "   font-weight: bold;"
                           "}"
                           "QTabBar::tab:selected {"
                           "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4a90d9, stop:1 #2980b9);"
                           "   color: white;"
                           "}"
                           "QTabBar::tab:hover:!selected {"
                           "   background: rgba(150, 180, 220, 240);"
                           "}")
                           .arg(borderWidth)
                           .arg(tabRadius)
                           .arg(tabPaddingV)
                           .arg(tabPaddingH)
                           .arg(tabMargin)
                           .arg(tabFontSize);

    tabWidget->setStyleSheet(tabStyle);

    // 缩放布局边距
    if (layout()) {
        int margin = static_cast<int>(20 * scale);
        int spacing = static_cast<int>(20 * scale);
        layout()->setContentsMargins(margin, margin, margin, margin);
        if (QVBoxLayout* vLayout = qobject_cast<QVBoxLayout*>(layout())) {
            vLayout->setSpacing(spacing);
        }
    }

    // 缩放所有卡片
    for (CodexCard* card : m_allCards) {
        if (card) {
            card->setScale(scale);
        }
    }

    // 缩放每个标签页内的网格布局间距
    int gridMargin = static_cast<int>(30 * scale);
    int gridSpacing = static_cast<int>(25 * scale);
    for (int i = 0; i < tabWidget->count(); ++i) {
        QWidget* page = tabWidget->widget(i);
        if (QScrollArea* scrollArea = qobject_cast<QScrollArea*>(page)) {
            if (QWidget* contentWidget = scrollArea->widget()) {
                if (QGridLayout* gridLayout = qobject_cast<QGridLayout*>(contentWidget->layout())) {
                    gridLayout->setContentsMargins(gridMargin, gridMargin, gridMargin, gridMargin);
                    gridLayout->setHorizontalSpacing(gridSpacing);
                    gridLayout->setVerticalSpacing(gridSpacing);
                }
            }
        }
    }
}

void Codex::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // 每次显示图鉴时刷新神秘物品数据（以便通关后能看到解锁的车票）
    refreshMysteryData();
}

void Codex::refreshMysteryData() {
    // 获取神秘物品标签页的索引（最后一个标签页）
    int mysteryTabIndex = tabWidget->count() - 1;
    if (mysteryTabIndex < 0)
        return;

    // 重新加载神秘物品数据
    m_mysteryEntries.clear();
    loadMysteryData();

    // 移除旧的标签页并创建新的
    QWidget* oldWidget = tabWidget->widget(mysteryTabIndex);
    tabWidget->removeTab(mysteryTabIndex);

    // 从 m_allCards 中移除将被删除的卡片
    if (oldWidget) {
        QList<CodexCard*> cardsToRemove;
        for (CodexCard* card : m_allCards) {
            if (card && card->parent() && card->window() == oldWidget->window()) {
                // 检查卡片是否属于旧的标签页
                QWidget* parent = card->parentWidget();
                while (parent && parent != oldWidget) {
                    parent = parent->parentWidget();
                }
                if (parent == oldWidget) {
                    cardsToRemove.append(card);
                }
            }
        }
        for (CodexCard* card : cardsToRemove) {
            m_allCards.removeOne(card);
        }
        oldWidget->deleteLater();
    }

    // 创建新的标签页
    tabWidget->addTab(createCategoryPage(m_mysteryEntries), "神秘物品");

    // 应用当前缩放到新创建的卡片
    double scaleX = width() / 800.0;
    double scaleY = height() / 600.0;
    double scale = qMin(scaleX, scaleY);
    for (CodexCard* card : m_allCards) {
        if (card) {
            card->setScale(scale);
        }
    }
}
