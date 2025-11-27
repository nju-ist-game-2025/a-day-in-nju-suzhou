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

// 图鉴条目数据结构
struct CodexEntry {
    QString name;          // 名称
    QString imagePath;     // 图片路径
    int health;            // 血量（-1表示无血量显示，如玩家）
    QString attackMethod;  // 攻击方式
    QString skills;        // 技能
    QString traits;        // 特性
    QString weakness;      // 弱点
    QString backstory;     // 背景故事
    bool isCharacter;      // 是否为玩家/NPC（只显示背景故事）
};

// 图鉴条目卡片组件
class CodexCard : public QWidget {
Q_OBJECT
public:
    explicit CodexCard(const CodexEntry &entry, QWidget *parent = nullptr);

signals:

    void clicked(const CodexEntry &entry);

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void enterEvent(QEnterEvent *event) override;

    void leaveEvent(QEvent *event) override;

private:
    CodexEntry m_entry;
    QLabel *m_imageLabel;
    QLabel *m_nameLabel;
};

// 详细信息弹窗
class CodexDetailDialog : public QDialog {
Q_OBJECT
public:
    explicit CodexDetailDialog(const CodexEntry &entry, QWidget *parent = nullptr);
};

// 主图鉴类
class Codex : public QWidget {
Q_OBJECT

public:
    explicit Codex(QWidget *parent = nullptr);

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

    QWidget *createCategoryPage(const QList<CodexEntry> &entries);

    void showEntryDetail(const CodexEntry &entry);

    QTabWidget *tabWidget;
    QPushButton *backButton;

    QList<CodexEntry> m_bossEntries;
    QList<CodexEntry> m_enemyEntries;
    QList<CodexEntry> m_playerEntries;
    QList<CodexEntry> m_usagiEntries;

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif  // CODEX_H
