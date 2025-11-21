#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QList>
#include <QWidget>
#include "../entities/enemy.h"
#include "../entities/player.h"
#include "ui/hud.h"

class Level;

class GameView : public QWidget {
Q_OBJECT

private:
    QGraphicsView *view;
    QGraphicsScene *scene;
    Player *player;
    Level *level;  // 关卡管理器
    HUD *hud{};

public:
    explicit GameView(QWidget *parent = nullptr);

    ~GameView() override;

    void initGame();

protected:
    void keyPressEvent(QKeyEvent *event) override;

    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void initAudio();

private slots:

    void updateHUD();                      // 更新HUD
    void handlePlayerDeath();              // 处理玩家死亡
    void restartGame();                    // 重新开始游戏（槽）
    void quitGame();                       // 退出游戏（槽）
    void onEnemiesCleared(int roomIndex);  // 房间敌人清空提示

signals:

    void backToMenu();
};

#endif  // GAMEVIEW_H
