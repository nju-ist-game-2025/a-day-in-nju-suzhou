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
    QPushButton* exitButton;
    QLabel* titleLabel;

   public:
    explicit MainMenu(QWidget* parent = nullptr);

   signals:
    void startGameClicked();
    void exitGameClicked();
};

#endif  // MAINMENU_H
