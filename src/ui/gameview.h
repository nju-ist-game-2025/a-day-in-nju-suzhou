#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QList>
#include <QWidget>
#include "../entities/enemy.h"
#include "../entities/player.h"

class GameView : public QWidget {
    Q_OBJECT

   private:
    QGraphicsView* view;
    QGraphicsScene* scene;
    Player* player;
    QList<Enemy*> enemies;  // 敌人列表

    void spawnEnemies();  // 生成敌人

   public:
    explicit GameView(QWidget* parent = nullptr);
    ~GameView();

    void initGame();

   protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

   private slots:
    void handlePlayerDeath();  // 处理玩家死亡
    void restartGame();        // 重新开始游戏（槽）
    void quitGame();           // 退出游戏（槽）

   signals:
    void backToMenu();
};

#endif  // GAMEVIEW_H
