#include "characterselector.h"
#include <QDir>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QResizeEvent>
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"

CharacterSelector::CharacterSelector(QWidget* parent)
    : QWidget(parent), m_selectedIndex(0) {
    setMinimumSize(800, 600);

    // 设置背景
    try {
        QPixmap backgroundPixmap = ResourceFactory::loadBackgroundImage("background_select_character", 800, 600);
        QPalette palette;
        palette.setBrush(QPalette::Window, QBrush(backgroundPixmap));
        setAutoFillBackground(true);
        setPalette(palette);
    } catch (const QString&) {
        setStyleSheet("background-color: #2c3e50;");
    }

    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(20);

    // 标题
    m_titleLabel = new QLabel("选择角色", this);
    QFont titleFont;
    titleFont.setFamily("Microsoft YaHei");
    titleFont.setPointSize(36);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("color: #000000;");

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(5);
    shadowEffect->setColor(QColor(0, 0, 0, 150));
    shadowEffect->setOffset(3, 3);
    m_titleLabel->setGraphicsEffect(shadowEffect);

    // 角色预览区域
    m_previewLabel = new QLabel(this);
    m_previewLabel->setFixedSize(150, 150);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet(
        "QLabel {"
        "   background-color: rgba(255, 255, 255, 50);"
        "   border: 3px solid #4CAF50;"
        "   border-radius: 10px;"
        "}");

    // 角色名称
    m_nameLabel = new QLabel(this);
    QFont nameFont;
    nameFont.setFamily("Microsoft YaHei");
    nameFont.setPointSize(18);
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setStyleSheet("color: #000000;");

    // 能力说明
    m_abilityLabel = new QLabel(this);
    m_abilityLabel->setWordWrap(true);
    QFont abilityFont;
    abilityFont.setFamily("Microsoft YaHei");
    abilityFont.setPointSize(14);
    m_abilityLabel->setFont(abilityFont);
    m_abilityLabel->setAlignment(Qt::AlignCenter);
    m_abilityLabel->setStyleSheet(
        "color: #1b5e20;"
        "background-color: rgba(255, 255, 255, 120);"
        "border: 2px solid #4CAF50;"
        "border-radius: 8px;"
        "padding: 10px;");

    // 角色选择按钮容器
    QWidget* characterContainer = new QWidget(this);
    m_characterLayout = new QHBoxLayout(characterContainer);
    m_characterLayout->setAlignment(Qt::AlignCenter);
    m_characterLayout->setSpacing(20);

    // 加载角色
    loadCharacters();

    // 按钮样式
    QString confirmButtonStyle =
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
        "}";

    QString backButtonStyle =
        "QPushButton {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #9E9E9E, stop:1 #757575);"
        "   color: white;"
        "   border: 2px solid #616161;"
        "   border-radius: 10px;"
        "   padding: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #BDBDBD, stop:1 #9E9E9E);"
        "}"
        "QPushButton:pressed {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #757575, stop:1 #616161);"
        "}";

    QFont buttonFont;
    buttonFont.setFamily("Microsoft YaHei");
    buttonFont.setPointSize(14);
    buttonFont.setBold(true);

    // 确认按钮
    m_confirmButton = new QPushButton("确认选择", this);
    m_confirmButton->setFixedSize(180, 50);
    m_confirmButton->setFont(buttonFont);
    m_confirmButton->setStyleSheet(confirmButtonStyle);

    // 返回按钮
    m_backButton = new QPushButton("返回", this);
    m_backButton->setFixedSize(180, 50);
    m_backButton->setFont(buttonFont);
    m_backButton->setStyleSheet(backButtonStyle);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(30);
    buttonLayout->addWidget(m_backButton);
    buttonLayout->addWidget(m_confirmButton);

    // 添加到主布局
    mainLayout->addStretch();
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_previewLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(m_nameLabel);
    mainLayout->addWidget(m_abilityLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(characterContainer);
    mainLayout->addSpacing(30);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    // 连接信号
    connect(m_confirmButton, &QPushButton::clicked, this, &CharacterSelector::onConfirmClicked);
    connect(m_backButton, &QPushButton::clicked, this, &CharacterSelector::onBackClicked);

    // 默认选中配置中的角色
    if (!m_characters.isEmpty()) {
        updateSelection(m_defaultSelectedIndex);
    }
}

void CharacterSelector::loadCharacters() {
    struct CharacterConfig {
        QString name;
        QString image;
        QString ability;
    };

    // 角色配置：名称、图片与能力说明
    QList<CharacterConfig> characterConfigs = {
        {"美少女", "beautifulGirl.png", "子弹伤害翻倍，适合喜欢爆发输出的玩家。"},
        {"高雅人士", "HighGracePeople.png", "初始心之容器 +2 且额外获得 2 点魂心，更耐打。"},
        {"小蓝鲸", "njuFish.png", "移动速度 +25%，子弹速度 +20%，射击冷却 -40ms，灵活敏捷。"},
        {"权服侠", "quanfuxia.png", "初始携带 2 枚炸弹、2 把钥匙和 1 点黑心，资源更充裕。"}};

    // 获取当前配置中的玩家角色路径
    QString currentPlayerPath = ConfigManager::instance().getAssetPath("player");
    int defaultSelectedIndex = 0;

    QString characterButtonStyle =
        "QPushButton {"
        "   background-color: rgba(255, 255, 255, 30);"
        "   border: 3px solid #888888;"
        "   border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(255, 255, 255, 60);"
        "   border-color: #AAAAAA;"
        "}"
        "QPushButton:checked {"
        "   background-color: rgba(76, 175, 80, 100);"
        "   border-color: #4CAF50;"
        "}";

    for (int i = 0; i < characterConfigs.size(); ++i) {
        const auto& config = characterConfigs[i];
        QString imagePath = QString("assets/player/%1").arg(config.image);

        // 检查文件是否存在
        if (!QFile::exists(imagePath)) {
            qWarning() << "角色图片不存在:" << imagePath;
            continue;
        }

        CharacterInfo info;
        info.name = config.name;
        info.imagePath = imagePath;
        info.ability = config.ability;

        // 创建角色选择按钮
        QPushButton* btn = new QPushButton(this);
        btn->setFixedSize(100, 100);
        btn->setCheckable(true);
        btn->setStyleSheet(characterButtonStyle);

        // 加载角色缩略图
        QPixmap thumbnail(imagePath);
        if (!thumbnail.isNull()) {
            thumbnail = thumbnail.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            btn->setIcon(QIcon(thumbnail));
            btn->setIconSize(QSize(80, 80));
        }

        info.button = btn;
        m_characters.append(info);
        m_characterLayout->addWidget(btn);

        // 检查是否是当前配置的角色
        if (imagePath == currentPlayerPath) {
            defaultSelectedIndex = m_characters.size() - 1;
        }

        // 连接点击信号
        int index = m_characters.size() - 1;
        connect(btn, &QPushButton::clicked, this, [this, index]() {
            onCharacterClicked(index);
        });
    }

    // 默认选择配置中的角色
    m_defaultSelectedIndex = defaultSelectedIndex;

    if (m_abilityLabel) {
        m_abilityLabel->setText("点击任意角色头像即可查看专属能力加成。");
    }
}

void CharacterSelector::onCharacterClicked(int index) {
    updateSelection(index);
}

void CharacterSelector::updateSelection(int index) {
    if (index < 0 || index >= m_characters.size())
        return;

    m_selectedIndex = index;
    m_selectedCharacter = m_characters[index].imagePath;
    m_selectedName = m_characters[index].name;

    // 更新按钮选中状态
    for (int i = 0; i < m_characters.size(); ++i) {
        m_characters[i].button->setChecked(i == index);
    }

    // 更新预览图
    QPixmap preview(m_selectedCharacter);
    if (!preview.isNull()) {
        preview = preview.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_previewLabel->setPixmap(preview);
    }

    // 更新名称
    m_nameLabel->setText(m_selectedName);

    // 更新能力说明
    if (m_abilityLabel) {
        QString abilityText = m_characters[index].ability.isEmpty()
                                  ? "该角色暂未配置能力说明。"
                                  : QString("能力：%1").arg(m_characters[index].ability);
        m_abilityLabel->setText(abilityText);
    }
}

void CharacterSelector::onConfirmClicked() {
    // 更新配置文件中的玩家角色路径
    ConfigManager::instance().setAssetPath("player", m_selectedCharacter);
    ConfigManager::instance().saveConfig();

    emit characterSelected(m_selectedCharacter);
}

void CharacterSelector::onBackClicked() {
    emit backToMenu();
}

void CharacterSelector::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // 更新背景图片以适应新的窗口大小
    try {
        QPixmap backgroundPixmap = ResourceFactory::loadBackgroundImage(
            "background_select_character", event->size().width(), event->size().height());
        QPalette palette;
        palette.setBrush(QPalette::Window, QBrush(backgroundPixmap));
        setPalette(palette);
    } catch (const QString&) {
        // 保持纯色背景
    }

    // 等比例缩放UI元素
    double scaleX = event->size().width() / 800.0;
    double scaleY = event->size().height() / 600.0;
    double scale = qMin(scaleX, scaleY);

    // 缩放按钮大小
    int confirmBtnWidth = static_cast<int>(180 * scale);
    int confirmBtnHeight = static_cast<int>(50 * scale);
    m_confirmButton->setFixedSize(confirmBtnWidth, confirmBtnHeight);
    m_backButton->setFixedSize(confirmBtnWidth, confirmBtnHeight);

    // 缩放角色选择按钮
    int charBtnSize = static_cast<int>(100 * scale);
    int charIconSize = static_cast<int>(80 * scale);
    for (auto& charInfo : m_characters) {
        charInfo.button->setFixedSize(charBtnSize, charBtnSize);
        charInfo.button->setIconSize(QSize(charIconSize, charIconSize));
    }

    // 缩放预览区域
    int previewSize = static_cast<int>(150 * scale);
    m_previewLabel->setFixedSize(previewSize, previewSize);

    // 缩放字体
    QFont titleFont;
    titleFont.setFamily("Microsoft YaHei");
    titleFont.setPointSize(static_cast<int>(36 * scale));
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    QFont nameFont;
    nameFont.setFamily("Microsoft YaHei");
    nameFont.setPointSize(static_cast<int>(18 * scale));
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);

    QFont buttonFont;
    buttonFont.setFamily("Microsoft YaHei");
    buttonFont.setPointSize(static_cast<int>(14 * scale));
    buttonFont.setBold(true);
    m_confirmButton->setFont(buttonFont);
    m_backButton->setFont(buttonFont);

    QFont abilityFont;
    abilityFont.setFamily("Microsoft YaHei");
    abilityFont.setPointSize(static_cast<int>(14 * scale));
    m_abilityLabel->setFont(abilityFont);

    // 更新预览图
    if (m_selectedIndex >= 0 && m_selectedIndex < m_characters.size()) {
        QPixmap preview(m_selectedCharacter);
        if (!preview.isNull()) {
            int previewImgSize = static_cast<int>(120 * scale);
            preview = preview.scaled(previewImgSize, previewImgSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_previewLabel->setPixmap(preview);
        }
    }
}
