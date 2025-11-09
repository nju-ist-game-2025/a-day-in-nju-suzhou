#ifndef MAINMENU_H
#define MAINMENU_H

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class MainMenu : public QWidget {
    Q_OBJECT

   private:
    QPushButton* startButton;
    QPushButton* codexButton;//新增图鉴功能
    QPushButton* exitButton;
    QLabel* titleLabel;

   public:
    explicit MainMenu(QWidget* parent = nullptr);

   signals:
    void startGameClicked();
    void codexClicked(); // new
    void exitGameClicked();
};

#endif  // MAINMENU_H
