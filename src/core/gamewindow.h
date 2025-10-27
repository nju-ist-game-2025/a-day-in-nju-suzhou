#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "../ui/gameview.h"
#include "../ui/mainmenu.h"

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
    void exitGame();

   private:
    Ui::GameWindow* ui;
    QStackedWidget* stackedWidget;
    MainMenu* mainMenu;
    GameView* gameView;
};
#endif  // GAMEWINDOW_H
