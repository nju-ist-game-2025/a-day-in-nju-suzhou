#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QList>
#include <QPushButton>
#include <QWidget>
#include "../entities/enemy.h"
#include "../entities/player.h"
#include "ui/hud.h"

class Level;

class PauseMenu;

class GameView : public QWidget {
    Q_OBJECT

   private:
    QGraphicsView* view;
    QGraphicsScene* scene;
    Player* player;
    Level* level;  // 关卡管理器
    HUD* hud{};
    PauseMenu* m_pauseMenu;    // 暂停菜单
    int currentLevel{};        // 添加当前关卡变量
    bool isLevelTransition{};  // 防止重复触发
    bool m_isInStoryMode{};
    bool m_isPaused;                // 游戏是否暂停
    QString m_playerCharacterPath;  // 玩家角色图片路径
    int m_startLevel = 1;           // 起始关卡（开发者模式）
    bool m_isDevMode = false;       // 是否为开发者模式
    int m_devMaxHealth = 3;         // 开发者模式血量上限
    int m_devBulletDamage = 1;      // 开发者模式子弹伤害
    bool m_devSkipToBoss = false;   // 开发者模式直接进入Boss房

   public:
    explicit GameView(QWidget* parent = nullptr);

    ~GameView() override;

    void initGame();
    void cleanupGame();  // 彻底清理游戏状态

    void setPlayerCharacter(const QString& characterPath);                       // 设置玩家角色
    void setStartLevel(int level) { m_startLevel = level; }                      // 设置起始关卡（开发者模式）
    void setDevModeSettings(int maxHealth, int bulletDamage, bool skipToBoss) {  // 设置开发者模式参数
        m_devMaxHealth = maxHealth;
        m_devBulletDamage = bulletDamage;
        m_devSkipToBoss = skipToBoss;
        m_isDevMode = true;
    }

    [[nodiscard]] HUD* getHUD() const { return hud; }

   protected:
    void showEvent(QShowEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void keyReleaseEvent(QKeyEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;  // 处理窗口大小变化

   private:
    void adjustViewToWindow();

    void initAudio();

    void togglePause();  // 切换暂停状态
    void resumeGame();   // 继续游戏
    void pauseGame();    // 暂停游戏
    void applyCharacterAbility(Player* player, const QString& characterPath);

    [[nodiscard]] QString resolveCharacterKey(const QString& characterPath) const;

    // 死亡界面相关（scene->clear()后会被自动删除，需要重置指针）
    QGraphicsRectItem* m_deathOverlay = nullptr;
    QPushButton* m_retryButton = nullptr;
    QPushButton* m_menuButton2 = nullptr;
    QPushButton* m_quitButton2 = nullptr;
    QGraphicsProxyWidget* m_retryProxy = nullptr;
    QGraphicsProxyWidget* m_menuProxy2 = nullptr;
    QGraphicsProxyWidget* m_quitProxy2 = nullptr;

    // 胜利界面相关（scene->clear()后会被自动删除，需要重置指针）
    QGraphicsRectItem* m_victoryOverlay = nullptr;
    QPushButton* m_victoryMenuButton = nullptr;
    QPushButton* m_victoryAgainButton = nullptr;
    QPushButton* m_victoryQuitButton = nullptr;

   private slots:

    void
    updateHUD();  // 更新HUD
    void
    handlePlayerDeath();  // 处理玩家死亡
    // 重新开始游戏（槽）
    // 退出游戏（槽）
    void onEnemiesCleared(int roomIndex, bool up = false, bool down = false, bool left = false,
                          bool right = false);  // 房间敌人清空提示
    void
    onBossDoorsOpened();  // boss门开启提示

    void advanceToNextLevel();

    void onLevelCompleted();  // 关卡完成时的处理

    void onStoryFinished();

   signals:

    void backToMenu();

    void requestRestart();

    void showVictoryUI();
};

#endif  // GAMEVIEW_H
