#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "../ui/characterselector.h"
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
    void startGameAtLevel(int level);  // 开发者模式：跳转到指定关卡

    void showCodex();                                        // 新增：显示图鉴
    void showCharacterSelector();                            // 显示角色选择
    void onCharacterSelected(const QString& characterPath);  // 角色选择完成
    void exitGame();
    void onRequestRestart();

   private:
    QStackedWidget* stackedWidget;
    MainMenu* mainMenu;
    GameView* gameView;
    Codex* codex;
    CharacterSelector* characterSelector;
    QString m_selectedCharacter;  // 保存选择的角色路径
};

#endif  // GAMEWINDOW_H
