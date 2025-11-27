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

class TeacherBoss;

class Chest;

class Usagi;

class ZhuhaoEnemy;

class QGraphicsScene;

class Level : public QObject {
    Q_OBJECT

   public:
    explicit Level(Player* player, QGraphicsScene* scene, QObject* parent = nullptr);

    ~Level() override;

    void init(int levelNumber);

    void setSkipToBoss(bool skip) { m_skipToBoss = skip; }  // 设置是否跳过到Boss房

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

    void
    showStoryDialog(const QStringList& dialogs, bool isBossDialog = false, const QString& customBackground = QString());

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

    void levelCompleted(int
                            levelNumber);

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

    // TeacherBoss相关
    void connectTeacherBossSignals(TeacherBoss* boss);

    void onTeacherBossRequestDialog(const QStringList& dialogs, const QString& background);

    void onTeacherBossRequestChangeBackground(const QString& backgroundPath);

    void onTeacherBossRequestTransitionText(const QString &text);
    
    void onTeacherBossRequestFadeBackground(const QString &backgroundPath, int duration);
    
    void onTeacherBossRequestFadeDialogBackground(const QString &backgroundPath, int duration);
    
    void onTeacherBossRequestDialogBackgroundChange(int dialogIndex, const QString &backgroundName);

    // Boss奖励机制（乌萨奇）
    void startBossRewardSequence();

    void onUsagiRequestShowDialog(const QStringList& dialog);

    void onUsagiRewardCompleted();

    // 精英房间相关
    void checkEliteRoomPhase2();  // 检查是否触发精英房间第二阶段
    void startElitePhase2();      // 启动精英房间第二阶段（生成朱昊）
    void spawnZhuhaoEnemy();      // 生成朱昊精英怪

    // 辅助方法：批量暂停/恢复敌人定时器
    void pauseAllEnemyTimers();

    void resumeAllEnemyTimers();

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
    QTimer* m_levelTextTimer;                         // 追踪关卡文字显示的定时器
    QGraphicsPixmapItem* m_textBackground = nullptr;  // 渐变文字背景

    // boss门相关
    bool m_hasEncounteredBossDoor;  // 是否已经遇到过boss门
    bool m_bossDoorsAlreadyOpened;  // boss门是否已经打开过
    bool m_skipToBoss = false;      // 开发者模式：直接跳过到Boss房

    // galgame相关
    QGraphicsPixmapItem* m_dialogBox;
    QGraphicsTextItem* m_dialogText;
    QGraphicsTextItem* m_skipHint = nullptr;  // "跳过"提示
    QGraphicsTextItem* m_continueHint;
    QStringList m_currentDialogs;
    int m_currentDialogIndex;
    bool m_skipRequested = false;  // 防止重复调用 finishStory
    bool m_isStoryFinished;
    bool m_isBossDialog;  // 标记当前是否为boss对话

    // 对话期间的Boss角色图片（入场动画）
    QGraphicsPixmapItem *m_dialogBossSprite = nullptr;
    QPropertyAnimation *m_dialogBossFlyAnimation = nullptr;
    bool m_isTeacherBossInitialDialog = false; // 标记是否为TeacherBoss初始对话
    
    // 对话中背景切换相关（TeacherBoss特殊需求）
    QMap<int, QString> m_pendingDialogBackgrounds;  // 待切换的对话背景 <对话索引, 背景名称>
    
    // 待执行的对话背景渐变（在对话框创建后执行）
    QString m_pendingFadeDialogBackground;  // 待渐变到的背景路径
    int m_pendingFadeDialogDuration = 0;    // 渐变持续时间

    // 背景图片项
    QGraphicsPixmapItem* m_backgroundItem = nullptr;
    // 当前背景路径（相对 assets/）
    QString m_currentBackgroundPath;
    // 原始背景路径（用于恢复）
    QString m_originalBackgroundPath;
    // 临时背景覆盖项（用于渐变过渡）
    QGraphicsPixmapItem* m_backgroundOverlay = nullptr;

    // 暂停状态
    bool m_isPaused = false;

    // WashMachineBoss引用（用于吸纳动画回调）
    QPointer<WashMachineBoss> m_currentWashMachineBoss;

    // TeacherBoss引用
    QPointer<TeacherBoss> m_currentTeacherBoss;

    // 吸纳动画相关
    bool m_isAbsorbAnimationActive = false;
    QTimer* m_absorbAnimationTimer = nullptr;
    QVector<QGraphicsItem*> m_absorbingItems;
    QVector<QPointF> m_absorbStartPositions;
    QVector<double> m_absorbAngles;
    QPointF m_absorbCenter;
    int m_absorbAnimationStep = 0;

    // Boss奖励机制相关
    QPointer<Usagi> m_usagi;
    bool m_bossDefeated = false;          // Boss是否已被击败
    bool m_rewardSequenceActive = false;  // 奖励流程是否激活

    // 精英房间相关
    bool m_isEliteRoom = false;           // 当前是否在精英房间
    bool m_elitePhase2Triggered = false;  // 精英房间第二阶段是否已触发
    int m_eliteYanglinDeathCount = 0;     // 精英房间中杨林死亡计数
    bool m_isEliteDialog = false;         // 是否正在显示精英房间对话
    QPointer<ZhuhaoEnemy> m_zhuhaoEnemy;  // 朱昊精英怪引用

   public slots:

    void onDialogClicked();

    void nextDialog();

    void showPhaseTransitionText(const QString& text, const QColor& color = QColor(75, 0, 130));  // 显示阶段转换文字提示（可被信号连接或直接调用）

    // 背景渐变接口：将背景渐变到给定路径（绝对路径或相对 assets/），duration 毫秒
    void fadeBackgroundTo(const QString &imagePath, int duration);
    
    // 对话框背景渐变接口：将对话框背景渐变到给定路径，duration 毫秒
    void fadeDialogBackgroundTo(const QString &imagePath, int duration);

   private slots:

    void onEnemyDying(Enemy* enemy);

    void finishStory();

    void initializeLevelAfterStory(const LevelConfig& config);

    // 吸纳动画步进
    void onAbsorbAnimationStep();
};

#endif  // LEVEL_H
