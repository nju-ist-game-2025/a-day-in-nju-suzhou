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
    QGraphicsView* view;
    QGraphicsScene* scene;
    Player* player;
    Level* level;  // 关卡管理器
    HUD* hud{};
    int currentLevel;        // 添加当前关卡变量
    bool isLevelTransition;  // 防止重复触发
    bool m_isInStoryMode;
    QString m_playerCharacterPath;  // 玩家角色图片路径

   public:
    explicit GameView(QWidget* parent = nullptr);

    ~GameView() override;

    void initGame();
    void setPlayerCharacter(const QString& characterPath);  // 设置玩家角色
    HUD* getHUD() const { return hud; }

   protected:
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void keyReleaseEvent(QKeyEvent* event) override;

   private:
    void initAudio();

   private slots:

    void updateHUD();                                                                                                 // 更新HUD
    void handlePlayerDeath();                                                                                         // 处理玩家死亡
    void restartGame();                                                                                               // 重新开始游戏（槽）
    void quitGame();                                                                                                  // 退出游戏（槽）
    void onEnemiesCleared(int roomIndex, bool up = false, bool down = false, bool left = false, bool right = false);  // 房间敌人清空提示
    void onBossDoorsOpened();                                                                                         // boss门开启提示

    void advanceToNextLevel();
    void onLevelCompleted();  // 关卡完成时的处理

    void onStoryFinished();

   signals:

    void backToMenu();
    void requestRestart();
};

#endif  // GAMEVIEW_H
