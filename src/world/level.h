#ifndef LEVEL_H
#define LEVEL_H

#include <QObject>
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVector>
#include "../items/droppeditem.h"
#include "../ui/dialogsystem.h"
#include "door.h"
#include "levelconfig.h"
#include "rewardsystem.h"
#include "room.h"
#include "roommanager.h"

class Player;
class Enemy;
class Boss;
class WashMachineBoss;
class TeacherBoss;
class Chest;
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

    // 物品掉落系统
    void dropRandomItem(QPointF position);                                         // 在指定位置掉落随机物品
    void dropItemsFromPosition(QPointF position, int count, bool scatter = true);  // 从指定位置掉落多个物品（散开效果）

    void clearCurrentRoomEntities();
    void clearSceneEntities();

    void showLevelStartText(LevelConfig& config);

    void openDoors(Room* cur);
    void showCredits(const QStringList& desc);

    void showStoryDialog(const QStringList& dialogs, bool isBossDialog = false, const QString& customBackground = QString());

    // 获取对话系统（供外部访问）
    DialogSystem* dialogSystem() const { return m_dialogSystem; }

    bool canOpenBossDoor() const;  // 检查是否所有非boss房间都已访问且怪物清空
    void openBossDoors();          // 打开所有通往boss房间的门

    // 暂停控制
    void setPaused(bool paused);
    bool isPaused() const { return m_isPaused; }

    // G键进入下一关（代理到RewardSystem）
    bool isGKeyEnabled() const;
    void triggerNextLevelByGKey();  // 按G键触发进入下一关

    // 吸纳动画（洗衣机Boss变异阶段）
    void performAbsorbAnimation(WashMachineBoss* boss);

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

    void ticketPickedUp();  // 车票拾取信号（通关）

   private:
    void initCurrentRoom(Room* room);
    void spawnEnemiesInRoom(int roomIndex);
    void spawnChestsInRoom(int roomIndex);
    void spawnDoors(const RoomConfig& roomCfg);
    void buildMinimapData();  // New method

    // Boss工厂方法：根据关卡号创建对应的Boss实例（使用BossFactory并连接信号）
    Boss* createBossByLevel(int levelNumber, const QPixmap& pic, double scale);

    // Boss信号连接设置（供RoomManager创建Boss后调用）
    void setupBossConnections(Boss* boss);

    // Boss召唤敌人（通用方法）
    void spawnEnemiesForBoss(const QVector<QPair<QString, int>>& enemies);

    // Boss奖励机制（代理到RewardSystem）
    void startBossRewardSequence();
    void onRewardShowDialogRequested(const QStringList& dialog);
    void onRewardSequenceCompleted();

    // 精英房间相关
    void checkEliteRoomPhase2();  // 检查是否触发精英房间第二阶段
    void startElitePhase2();      // 启动精英房间第二阶段
    void spawnZhuhaoEnemy();      // 生成zhuhao精英怪

    // 辅助方法：批量暂停/恢复敌人定时器
    void pauseAllEnemyTimers();

    void resumeAllEnemyTimers();

    // G键提示相关
    void showGKeyHint();  // 显示G键提示
    void hideGKeyHint();  // 隐藏G键提示

    // ========== 房间管理访问器（委托给RoomManager）==========
    int currentRoomIndex() const;
    QVector<Room*>& rooms();
    const QVector<Room*>& rooms() const;
    QVector<QPointer<Enemy>>& currentEnemies();
    QVector<QPointer<Chest>>& currentChests();
    QVector<Door*>& currentDoors();
    QMap<int, QVector<Door*>>& roomDoors();
    QVector<bool>& visitedRooms();
    const QVector<bool>& visitedRooms() const;
    int& visitedCount();
    bool hasEncounteredBossDoor() const;
    void setHasEncounteredBossDoor(bool value);
    bool bossDoorsAlreadyOpened() const;
    void setBossDoorsAlreadyOpened(bool value);
    void setCurrentRoomIndex(int index);

    // 房间管理器（完全管理房间数据）
    RoomManager* m_roomManager = nullptr;

    // 对话系统（管理所有对话相关UI和逻辑）
    DialogSystem* m_dialogSystem = nullptr;

    int m_levelNumber;
    Player* m_player;
    QGraphicsScene* m_scene;
    QTimer* checkChange;

    bool m_skipToBoss = false;  // 开发者模式：直接跳过到Boss房

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

    // Boss奖励机制相关 - 使用RewardSystem管理
    RewardSystem* m_rewardSystem = nullptr;
    bool m_bossDefeated = false;  // Boss是否已被击败

    // G键提示文字（UI由Level管理，状态由RewardSystem管理）
    QGraphicsTextItem* m_gKeyHintText = nullptr;  // G键提示文字

    // 精英房间相关
    bool m_isEliteRoom = false;           // 当前是否在精英房间
    bool m_elitePhase2Triggered = false;  // 精英房间第二阶段是否已触发
    int m_eliteYanglinDeathCount = 0;     // 精英房间yanglin死亡计数
    bool m_isEliteDialog = false;         // 是否正在显示精英房间对话
    QPointer<ZhuhaoEnemy> m_zhuhaoEnemy;  // zhuhao精英怪引用

   public slots:

    void showPhaseTransitionText(const QString& text, const QColor& color = QColor(75, 0, 130));  // 显示阶段转换文字提示（可被信号连接或直接调用）

    // 背景切换（供Boss信号连接使用）
    void changeBackground(const QString& backgroundPath);

    // 背景渐变接口：将背景渐变到给定路径（绝对路径或相对 assets/），duration 毫秒
    void fadeBackgroundTo(const QString& imagePath, int duration);

    // 对话框背景渐变接口（委托给DialogSystem）
    void fadeDialogBackgroundTo(const QString& imagePath, int duration);

    // ========== Boss通用槽函数（供Boss类的setupLevelConnections使用）==========
    void onBossRequestDialog(const QStringList& dialogs, const QString& background);
    void onBossRequestAbsorb();  // WashMachineBoss吸纳请求
    void onBossRequestFadeDialogBackground(const QString& backgroundPath, int duration);
    void onBossRequestDialogBackgroundChange(int dialogIndex, const QString& backgroundName);
    void onBossEnemySpawned(Enemy* enemy);  // TeacherBoss直接生成的敌人追踪

   private slots:

    void onEnemyDying(Enemy* enemy);

    // 对话系统回调槽函数
    void onDialogSystemStoryFinished();
    void onDialogSystemBossDialogFinished();
    void onDialogSystemEliteDialogFinished();

    void initializeLevelAfterStory(const LevelConfig& config);

    // 吸纳动画步进
    void onAbsorbAnimationStep();
};

#endif  // LEVEL_H
