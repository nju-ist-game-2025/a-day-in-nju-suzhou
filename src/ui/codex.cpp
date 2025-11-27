#include "codex.h"
#include <QFile>
#include <QFont>
#include <QMouseEvent>
#include <QPalette>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include "../core/resourcefactory.h"

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
        m_imageLabel->setStyleSheet("background-color: rgba(220,220,220,220); border-radius: 10px; font-size: 24px; color: #333;");
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

// ==================== CodexDetailDialog 实现 ====================

CodexDetailDialog::CodexDetailDialog(const CodexEntry& entry, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(entry.name);
    setMinimumSize(500, 400);
    setMaximumSize(600, 600);
    setStyleSheet("QDialog { background-color: rgba(230, 240, 255, 255); border: 2px solid rgba(100, 149, 237, 200); border-radius: 10px; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 顶部：图片和名称
    QHBoxLayout* headerLayout = new QHBoxLayout();

    QLabel* imageLabel = new QLabel(this);
    imageLabel->setFixedSize(100, 100);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("background-color: rgba(255, 255, 255, 220); border: 2px solid rgba(100, 149, 237, 200); border-radius: 15px;");
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

    // 创建各标签页
    tabWidget->addTab(createCategoryPage(m_bossEntries), "Boss");
    tabWidget->addTab(createCategoryPage(m_enemyEntries), "敌人");
    tabWidget->addTab(createCategoryPage(m_playerEntries), "玩家");
    tabWidget->addTab(createCategoryPage(m_usagiEntries), "乌萨奇");
}

void Codex::setupUI() {
    setMinimumSize(800, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 标题
    QLabel* titleLabel = new QLabel("游戏图鉴", this);
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
        gridLayout->addWidget(card, row, col, Qt::AlignLeft | Qt::AlignTop);

        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }

    // 设置列宽固定，让卡片左对齐
    for (int i = 0; i < maxCols; ++i) {
        gridLayout->setColumnStretch(i, 0);
    }
    // 最后添加一个弹性列，把所有内容推到左边
    gridLayout->setColumnStretch(maxCols, 1);

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
    // 梦魇Boss
    CodexEntry nightmare;
    nightmare.name = "梦魇";
    nightmare.imagePath = "assets/boss/Nightmare/Nightmare.png";
    nightmare.health = 250;
    nightmare.attackMethod = "单段冲刺攻击，二阶段瞬移突袭";
    nightmare.skills = "【亡语】一阶段死亡时击杀场上所有小怪并进入二阶段\n【噩梦缠绕】剥夺玩家视野3秒后瞬移至玩家身边\n【噩梦降临】召唤大量小怪并发动强制冲刺";
    nightmare.traits = "一阶段死亡后自动进入二阶段，二阶段血量300，30%伤害减免";
    nightmare.weakness = "炸弹闹钟可对其造成50点伤害";
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
    washmachine.health = 400;
    washmachine.attackMethod = "普通阶段：四方向水柱冲击\n愤怒阶段：高速冲刺\n变异阶段：毒气攻击（扩散+追踪）";
    washmachine.skills = "【愤怒】70%血量触发，召唤旋转臭袜子护盾\n【变异】40%血量触发，吸收场上所有物体进行强化，释放有毒气体";
    washmachine.traits = "三阶段Boss，每个阶段有独特的攻击模式，20%伤害减免";
    washmachine.weakness = "变异前的吸收阶段完全无敌，需要等待变异完成";
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
    teacher.health = 500;
    teacher.attackMethod = "授课阶段：正态分布弹幕、随机点名红圈\n期中考试：追踪考卷、极大似然估计陷阱、召唤监考员\n方差爆炸：环形弹幕、挂科警告、喜忧参半分裂弹";
    teacher.skills = "【正态分布弹幕】发射15发弹幕，角度服从N(μ,15°)\n【随机点名】在玩家位置生成延时伤害红圈\n【极大似然估计】预判玩家移动方向放置陷阱\n【喜忧参半】发射在2/3距离处分裂成5发的大型弹幕";
    teacher.traits = "三阶段Boss，调离阶段会飞出屏幕后以更强姿态返回，拥有全图视野";
    teacher.weakness = "弹幕服从正态分布，站在边缘位置可以降低命中概率";
    teacher.backstory = "奶牛张很凶悍，他是在概率论的海洋中成长的。他不在乎任何人的看法，无论是学霸还是学渣，他发出的考卷，是为了让所有人知道什么叫做正态分布。\n\n其实呢，奶牛张一直暗暗地希望有人能理解他的极大似然估计。他即将调往北京，对此表示喜忧参半。";
    teacher.isCharacter = false;
    teacher.phaseImages = {
        {"assets/boss/Teacher/cow.png", "授课"},
        {"assets/boss/Teacher/cowAngry.png", "期中考试"},
        {"assets/boss/Teacher/cowFinal.png", "方差爆炸"}};
    m_bossEntries.append(teacher);
}

void Codex::loadEnemyData() {
    // 第一关敌人
    CodexEntry clockNormal;
    clockNormal.name = "普通闹钟";
    clockNormal.imagePath = "assets/enemy/level_1/clock_normal.png";
    clockNormal.health = 10;
    clockNormal.attackMethod = "近战接触攻击";
    clockNormal.skills = "【惊吓】接触玩家时100%触发惊吓效果，使玩家移速增加但受伤提升150%，持续3秒";
    clockNormal.traits = "Z字形移动模式，难以预判";
    clockNormal.weakness = "血量较低，可以快速击杀";
    clockNormal.backstory = "普通闹钟很烦躁，它每天的工作就是在固定时间尖叫。它不理解为什么人类需要睡觉，也不理解为什么每次它完成工作后都会被狠狠地拍一下。\n\n它只知道，响铃是它的使命。";
    clockNormal.isCharacter = false;
    m_enemyEntries.append(clockNormal);

    CodexEntry clockBoom;
    clockBoom.name = "炸弹闹钟";
    clockBoom.imagePath = "assets/enemy/level_1/clock_boom.png";
    clockBoom.health = 6;
    clockBoom.attackMethod = "接触后2.5秒倒计时爆炸";
    clockBoom.skills = "【爆炸】对范围内玩家造成2点伤害，对梦魇Boss造成50点伤害，对其他敌人造成6点伤害";
    clockBoom.traits = "不会主动移动，被攻击可摧毁，同类不互相伤害";
    clockBoom.weakness = "可以远程击杀或引诱到Boss附近引爆";
    clockBoom.backstory = "炸弹闹钟是普通闹钟的极端版本。当它发现普通的响铃已经无法叫醒主人时，它选择了一种更激进的方式——物理意义上的叫醒。\n\n它的座右铭是：要么起床，要么永远睡下去。";
    clockBoom.isCharacter = false;
    m_enemyEntries.append(clockBoom);

    CodexEntry pillow;
    pillow.name = "枕头怪";
    pillow.imagePath = "assets/enemy/level_1/pillow.png";
    pillow.health = 20;
    pillow.attackMethod = "近战接触攻击";
    pillow.skills = "【昏睡】接触玩家时100%触发昏睡效果，使玩家无法移动1.5秒";
    pillow.traits = "绕圈移动模式，移速较快";
    pillow.weakness = "移动轨迹固定，容易预判";
    pillow.backstory = "枕头怪非常柔软，柔软到它认为任何碰到它的人都应该立刻睡着。它不明白为什么人类要挣扎着起床，在它看来，睡眠才是生命的真谛。\n\n它的梦想是让全世界都陷入永恒的睡眠。";
    pillow.isCharacter = false;
    m_enemyEntries.append(pillow);

    // 第二关敌人
    CodexEntry sockNormal;
    sockNormal.name = "普通臭袜子";
    sockNormal.imagePath = "assets/enemy/level_2/sock_normal.png";
    sockNormal.health = 10;
    sockNormal.attackMethod = "近战接触攻击";
    sockNormal.skills = "【中毒】50%概率触发中毒效果，每秒扣1点血，持续3秒";
    sockNormal.traits = "斜向移动模式，擅长躲避直线子弹";
    sockNormal.weakness = "血量较低，中毒概率只有50%";
    sockNormal.backstory = "普通臭袜子曾经是一只普通的袜子。在被主人连续穿了三天之后，它获得了自我意识。它现在只想做一件事——报复所有的脚。\n\n它散发的气味足以让人感到恶心。";
    sockNormal.isCharacter = false;
    m_enemyEntries.append(sockNormal);

    CodexEntry sockAngrily;
    sockAngrily.name = "愤怒臭袜子";
    sockAngrily.imagePath = "assets/enemy/level_2/sock_angrily.png";
    sockAngrily.health = 18;
    sockAngrily.attackMethod = "冲刺攻击";
    sockAngrily.skills = "【中毒】50%概率触发中毒效果\n【冲刺】1.2秒蓄力后高速冲向玩家";
    sockAngrily.traits = "移速提升150%，伤害提升150%";
    sockAngrily.weakness = "蓄力期间可以提前躲避";
    sockAngrily.backstory = "愤怒臭袜子是在洗衣机里被其他衣物霸凌后产生的变异体。它比普通臭袜子更臭、更快、更暴躁。\n\n它发誓要让所有把它扔进洗衣机的人付出代价。";
    sockAngrily.isCharacter = false;
    m_enemyEntries.append(sockAngrily);

    CodexEntry pants;
    pants.name = "内裤怪";
    pants.imagePath = "assets/enemy/level_2/pants.png";
    pants.health = 20;
    pants.attackMethod = "近战接触攻击 + 旋转技能";
    pants.skills = "【旋转】每20秒释放，持续5秒，期间移速x2，对范围内玩家每0.5秒造成3点伤害";
    pants.traits = "Z字形移动，开局立即释放一次旋转";
    pants.weakness = "旋转结束后有较长冷却时间";
    pants.backstory = "内裤怪是洗衣房最不愿意提起的存在。没人知道它是谁的内裤，也没人敢认领它。它在洗衣房的角落里待了太久，久到它开始产生了自己的想法。\n\n它的旋转攻击据说是在模仿洗衣机的滚筒。";
    pants.isCharacter = false;
    m_enemyEntries.append(pants);

    // 第三关敌人
    CodexEntry digitalSystem;
    digitalSystem.name = "数字系统";
    digitalSystem.imagePath = "assets/enemy/level_3/digital_system.png";
    digitalSystem.health = 25;
    digitalSystem.attackMethod = "近战接触攻击";
    digitalSystem.skills = "【成长】随时间逐渐变大，体型和伤害同步增加";
    digitalSystem.traits = "绕圈移动模式，初始较小但会不断成长";
    digitalSystem.weakness = "尽早击杀，避免它成长到难以对付的程度";
    digitalSystem.backstory = "数字系统是从《数字系统设计基础》这门课里跑出来的。它不断地成长，就像你对它的恐惧一样。\n\n据说只要你看懂了它，它就会消失。可惜没人看懂过。";
    digitalSystem.isCharacter = false;
    m_enemyEntries.append(digitalSystem);

    CodexEntry optimization;
    optimization.name = "优化问题";
    optimization.imagePath = "assets/enemy/level_3/optimization.png";
    optimization.health = 25;
    optimization.attackMethod = "近战接触攻击";
    optimization.skills = "【成长】随时间逐渐变大";
    optimization.traits = "绕圈移动模式";
    optimization.weakness = "与数字系统相同的弱点";
    optimization.backstory = "优化问题是每个工科生的噩梦。它的存在本身就是一个需要求解的问题，而答案永远是：再多学一遍。\n\n它在寻找全局最优解的过程中迷失了自己。";
    optimization.isCharacter = false;
    m_enemyEntries.append(optimization);

    CodexEntry yanglin;
    yanglin.name = "杨林";
    yanglin.imagePath = "assets/enemy/level_3/yanglin.png";
    yanglin.health = 200;
    yanglin.attackMethod = "近战接触攻击（5点伤害）+ 旋转技能";
    yanglin.skills = "【旋转】开局10秒后释放，之后每30秒释放，持续5秒，无可视圆但伤害范围随体型增加";
    yanglin.traits = "精英怪，全图视野，会随时间成长，属性接近Boss一阶段";
    yanglin.weakness = "旋转技能没有可视指示器，需要通过观察判断";
    yanglin.backstory = "杨林是凸优化考试中最可怕的大题。它庞大的身躯里装满了各种公式和定理，每一个都在等待着吞噬不及格的学生。\n\n传说中，能解出杨林的人会获得永恒的智慧。至今无人验证过这个传说。";
    yanglin.isCharacter = false;
    m_enemyEntries.append(yanglin);
}

void Codex::loadPlayerData() {
    CodexEntry beautifulGirl;
    beautifulGirl.name = "美少女";
    beautifulGirl.imagePath = "assets/player/beautifulGirl.png";
    beautifulGirl.health = -1;
    beautifulGirl.isCharacter = true;
    beautifulGirl.backstory = "美少女是某组员最喜欢的角色。她拥有让子弹伤害翻倍的神奇能力，据说这种力量来源于她对游戏的热爱。\n\n她的座右铭是：输出就是正义。";
    m_playerEntries.append(beautifulGirl);

    CodexEntry highGracePeople;
    highGracePeople.name = "高雅人士";
    highGracePeople.imagePath = "assets/player/HighGracePeople.png";
    highGracePeople.health = -1;
    highGracePeople.isCharacter = true;
    highGracePeople.backstory = "高雅人士是一只神秘优雅的企鹅，戴着墨镜，插着腰，脸上挂着迷之微笑。没人知道墨镜后面藏着什么样的眼神，也没人敢问。\n\n它的高雅不仅体现在姿态上，还体现在它额外的心之容器和魂心上。高雅，是要有代价的。";
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
    quanfuxia.backstory = "权服侠是一位神秘的英雄，专门帮助丢失物品的同学。丢了校园卡？找权服侠。丢了钥匙？找权服侠。丢了作业？呃...那还是自己重做吧。\n\n他随身携带炸弹和钥匙，还有一颗黑心——字面意义上的黑心，不是说他是坏人。";
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

    // 缩放UI元素
    double scaleX = event->size().width() / 800.0;
    double scaleY = event->size().height() / 600.0;
    double scale = qMin(scaleX, scaleY);

    int btnWidth = static_cast<int>(150 * scale);
    int btnHeight = static_cast<int>(40 * scale);
    backButton->setFixedSize(btnWidth, btnHeight);
}
