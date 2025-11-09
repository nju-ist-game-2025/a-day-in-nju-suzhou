#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "../ui/gameview.h"
#include "../ui/mainmenu.h"
#include "../ui/codex.h"  // 新增包含

QT_BEGIN_NAMESPACE
namespace Ui {
class GameWindow;
}
QT_END_NAMESPACE

class GameWindow : public QMainWindow {
    Q_OBJECT

   public:
    GameWindow(QWidget* parent = nullptr);
    ~GameWindow();

   private slots:
    void showMainMenu();
    void startGame();
    void showCodex();           // 新增：显示图鉴
    void exitGame();

   private:
    Ui::GameWindow* ui;
    QStackedWidget* stackedWidget;
    MainMenu* mainMenu;
    GameView* gameView;
    Codex* codex;
};
#endif  // GAMEWINDOW_H
