#ifndef CODEX_H
#define CODEX_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QTextBrowser>
#include <QTabWidget>

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
    void loadMonsterData();

    QTabWidget* tabWidget;
    QListWidget* monsterList;
    QTextBrowser* detailBrowser;
    QPushButton* backButton;

    QMap<QString, QString> monsterData;
};

#endif // CODEX_H
