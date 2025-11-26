#ifndef LEVEL_H
#define LEVEL_H

#include <QObject>
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVector>
#include "door.h"
#include "levelconfig.h"
#include "room.h"

class Player;
class Enemy;
class Boss;
class WashMachineBoss;
class Chest;
class QGraphicsScene;

class Level : public QObject {
    Q_OBJECT

   public:
    explicit Level(Player* player, QGraphicsScene* scene, QObject* parent = nullptr);
    ~Level() override;

    void init(int levelNumber);
    int currentLevel() const { return m_levelNumber; }
    bool isAllRoomsCompleted() const;
    bool enterNextRoom();
    Room* currentRoom() const;
    void loadRoom(int roomIndex);
    void onPlayerDied();
    void bonusEffects();
    void clearCurrentRoomEntities();
    void clearSceneEntities();

    void showLevelStartText(LevelConfig& config);
    void openDoors(Room* cur);

    void showCredits(const QStringList& desc);

    void showStoryDialog(const QStringList& dialogs, bool isBossDialog = false, const QString& customBackground = QString());

    bool canOpenBossDoor() const;  // 检查是否所有非boss房间都已访问且怪物清空
    void openBossDoors();          // 打开所有通往boss房间的门

    // 暂停控制
    void setPaused(bool paused);
    bool isPaused() const { return m_isPaused; }

    // 背景切换
    void changeBackground(const QString& backgroundPath);

    // 吸纳动画（洗衣机Boss变异阶段）
    void performAbsorbAnimation(WashMachineBoss* boss);

   protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

   signals:
    void levelCompleted(int levelNumber);
    void roomEntered(int roomIndex);
    void enemiesCleared(int roomIndex, bool up = false, bool down = false, bool left = false, bool right = false);
    void bossDoorsOpened();  // boss房间的门被打开

    void storyFinished();
    void dialogStarted();   // 对话开始（包括关卡开始和boss对话）
    void dialogFinished();  // 对话结束
    void levelInitialized();

   private:
    void initCurrentRoom(Room* room);
    void spawnEnemiesInRoom(int roomIndex);
    void spawnChestsInRoom(int roomIndex);
    void spawnDoors(const RoomConfig& roomCfg);
    void buildMinimapData();  // New method

    // 敌人工厂方法：根据类型创建具体的敌人实例
    Enemy* createEnemyByType(int levelNumber, const QString& enemyType, const QPixmap& pic, double scale);

    // Boss工厂方法：根据关卡号创建对应的Boss实例
    Boss* createBossByLevel(int levelNumber, const QPixmap& pic, double scale);

    // Boss召唤敌人（通用方法）
    void spawnEnemiesForBoss(const QVector<QPair<QString, int>>& enemies);

    // WashMachineBoss相关
    void connectWashMachineBossSignals(WashMachineBoss* boss);
    void onWashMachineBossRequestDialog(const QStringList& dialogs, const QString& background);
    void onWashMachineBossRequestChangeBackground(const QString& backgroundPath);
    void onWashMachineBossRequestAbsorb();

    int m_levelNumber;
    QVector<Room*> m_rooms;
    int m_currentRoomIndex;
    Player* m_player;
    QGraphicsScene* m_scene;
    QVector<QPointer<Enemy>> m_currentEnemies;
    QVector<QPointer<Chest>> m_currentChests;
    QVector<QGraphicsItem*> m_doorItems;
    QVector<Door*> m_currentDoors;          // 当前房间的门对象
    QMap<int, QVector<Door*>> m_roomDoors;  // 每个房间的门（roomIndex -> doors）
    QVector<bool> visited;
    QTimer* checkChange;
    int visited_count;
    QTimer* m_levelTextTimer;               // 追踪关卡文字显示的定时器
    QGraphicsPixmapItem* m_textBackground;  // 新增：渐变文字背景

    // boss门相关
    bool m_hasEncounteredBossDoor;  // 是否已经遇到过boss门
    bool m_bossDoorsAlreadyOpened;  // boss门是否已经打开过

    // galgame相关
    QGraphicsPixmapItem* m_dialogBox;
    QGraphicsTextItem* m_dialogText;
    QGraphicsTextItem* m_skipHint;  // “跳过”提示
    QGraphicsTextItem* m_continueHint;
    QStringList m_currentDialogs;
    int m_currentDialogIndex;
    bool m_skipRequested = false;  // 防止重复调用 finishStory
    bool m_isStoryFinished;
    bool m_isBossDialog;  // 标记当前是否为boss对话

    // 背景图片项
    QGraphicsPixmapItem* m_backgroundItem = nullptr;

    // 暂停状态
    bool m_isPaused = false;

    // WashMachineBoss引用（用于吸纳动画回调）
    QPointer<WashMachineBoss> m_currentWashMachineBoss;

    // 吸纳动画相关
    bool m_isAbsorbAnimationActive = false;
    QTimer* m_absorbAnimationTimer = nullptr;
    QVector<QGraphicsItem*> m_absorbingItems;
    QVector<QPointF> m_absorbStartPositions;
    QVector<double> m_absorbAngles;
    QPointF m_absorbCenter;
    int m_absorbAnimationStep = 0;

   public slots:
    void onDialogClicked();
    void nextDialog();

   private slots:
    void onEnemyDying(Enemy* enemy);

    void finishStory();
    void initializeLevelAfterStory(const LevelConfig& config);

    // 吸纳动画步进
    void onAbsorbAnimationStep();
};

#endif  // LEVEL_H
