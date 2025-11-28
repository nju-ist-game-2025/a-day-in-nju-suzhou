#ifndef CODEX_H
#define CODEX_H

#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

// Boss阶段图片信息
struct PhaseImage {
    QString imagePath;  // 图片路径
    QString phaseName;  // 阶段名称
};

// 图鉴条目数据结构
struct CodexEntry {
    QString name;                   // 名称
    QString imagePath;              // 图片路径
    int health;                     // 血量（-1表示无血量显示，如玩家）
    QString attackMethod;           // 攻击方式
    QString skills;                 // 技能
    QString traits;                 // 特性
    QString weakness;               // 弱点
    QString backstory;              // 背景故事
    bool isCharacter;               // 是否为玩家/NPC（只显示背景故事）
    QList<PhaseImage> phaseImages;  // Boss各阶段图片（可选）
};

// 图鉴条目卡片组件
class CodexCard : public QWidget {
    Q_OBJECT
   public:
    explicit CodexCard(const CodexEntry& entry, QWidget* parent = nullptr);
    void setScale(double scale);  // 缩放卡片

   signals:

    void clicked(const CodexEntry& entry);

   protected:
    void mousePressEvent(QMouseEvent* event) override;

    void enterEvent(QEnterEvent* event) override;

    void leaveEvent(QEvent* event) override;

   private:
    CodexEntry m_entry;
    QLabel* m_imageLabel;
    QLabel* m_nameLabel;

    // 基础尺寸常量
    static constexpr int BASE_CARD_WIDTH = 120;
    static constexpr int BASE_CARD_HEIGHT = 150;
    static constexpr int BASE_IMAGE_SIZE = 80;
    static constexpr int BASE_PIXMAP_SIZE = 70;
    static constexpr int BASE_FONT_SIZE = 10;
};

// 详细信息弹窗
class CodexDetailDialog : public QDialog {
    Q_OBJECT
   public:
    explicit CodexDetailDialog(const CodexEntry& entry, QWidget* parent = nullptr);
};

// 主图鉴类
class Codex : public QWidget {
    Q_OBJECT

   public:
    explicit Codex(QWidget* parent = nullptr);

   public slots:

    void returnToMenu();

   signals:

    void backToMenu();

   private:
    void setupUI();

    void loadBossData();

    void loadEnemyData();

    void loadPlayerData();

    void loadUsagiData();

    void loadItemData();

    void loadMechanicsData();

    void loadMysteryData();  // 神秘物品（车票等）

    void refreshMysteryData();  // 刷新神秘物品数据（通关后更新）

    QWidget* createCategoryPage(const QList<CodexEntry>& entries);

    void showEntryDetail(const CodexEntry& entry);

    QTabWidget* tabWidget;
    QPushButton* backButton;
    QLabel* titleLabel;            // 标题标签，用于缩放
    QList<CodexCard*> m_allCards;  // 所有卡片，用于缩放

    QList<CodexEntry> m_bossEntries;
    QList<CodexEntry> m_enemyEntries;
    QList<CodexEntry> m_playerEntries;
    QList<CodexEntry> m_usagiEntries;
    QList<CodexEntry> m_itemEntries;
    QList<CodexEntry> m_mechanicsEntries;
    QList<CodexEntry> m_mysteryEntries;  // 神秘物品

   protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;  // 显示时刷新数据
};

#endif  // CODEX_H
