#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "../ui/codex.h"
#include "../ui/gameview.h"
#include "../ui/mainmenu.h"

class GameWindow : public QMainWindow {
    Q_OBJECT

   public:
    GameWindow(QWidget* parent = nullptr);
    ~GameWindow();

   private slots:
    void showMainMenu();
    void startGame();
    void showCodex();  // 新增：显示图鉴
    void exitGame();

   private:
    QStackedWidget* stackedWidget;
    MainMenu* mainMenu;
    GameView* gameView;
    Codex* codex;
};
#endif  // GAMEWINDOW_H
