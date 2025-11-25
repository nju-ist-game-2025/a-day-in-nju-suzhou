#ifndef CODEX_H
#define CODEX_H

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

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

   protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif  // CODEX_H
