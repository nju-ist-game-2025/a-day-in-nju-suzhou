#ifndef BOSSFIGHT_H
#define BOSSFIGHT_H

#include <QColor>
#include <QGraphicsItem>
#include <QMap>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>

class Boss;
class NightmareBoss;
class WashMachineBoss;
class TeacherBoss;
class Enemy;
class Player;
class QGraphicsScene;
class QGraphicsPixmapItem;
class QPropertyAnimation;

/**
 * @brief Boss战斗管理类 - 封装Boss战斗相关的所有逻辑
 *
 * 职责：
 *   - Boss信号连接和事件处理
 *   - Boss阶段转换和对话管理
 *   - Boss特殊机制（吸纳动画、敌人召唤等）
 *   - 背景切换和特效
 */
class BossFight : public QObject {
    Q_OBJECT

   public:
    explicit BossFight(Player* player, QGraphicsScene* scene, QObject* parent = nullptr);
    ~BossFight() override;

    /**
     * @brief 清理所有资源
     */
    void cleanup();

    /**
     * @brief 设置背景图片项（用于背景切换）
     */
    void setBackgroundItem(QGraphicsPixmapItem* item) { m_backgroundItem = item; }

    /**
     * @brief 设置背景覆盖项（用于渐变效果）
     */
    void setBackgroundOverlay(QGraphicsPixmapItem* overlay) { m_backgroundOverlay = overlay; }

    /**
     * @brief 初始化Nightmare Boss
     */
    void initNightmareBoss(NightmareBoss* boss);

    /**
     * @brief 初始化WashMachine Boss
     */
    void initWashMachineBoss(WashMachineBoss* boss);

    /**
     * @brief 初始化Teacher Boss
     */
    void initTeacherBoss(TeacherBoss* boss);

    /**
     * @brief 获取当前WashMachineBoss
     */
    WashMachineBoss* currentWashMachineBoss() const { return m_currentWashMachineBoss.data(); }

    /**
     * @brief 获取当前TeacherBoss
     */
    TeacherBoss* currentTeacherBoss() const { return m_currentTeacherBoss.data(); }

    /**
     * @brief 检查Boss是否已被击败
     */
    bool isBossDefeated() const { return m_bossDefeated; }

    /**
     * @brief 标记Boss已被击败
     */
    void setBossDefeated(bool defeated) { m_bossDefeated = defeated; }

    /**
     * @brief 检查奖励流程是否激活
     */
    bool isRewardSequenceActive() const { return m_rewardSequenceActive; }

    /**
     * @brief 设置奖励流程状态
     */
    void setRewardSequenceActive(bool active) { m_rewardSequenceActive = active; }

    /**
     * @brief 执行吸纳动画（WashMachineBoss Phase 3）
     */
    void performAbsorbAnimation(WashMachineBoss* boss, const QVector<QPointer<Enemy>>& enemies);

    /**
     * @brief 检查吸纳动画是否激活
     */
    bool isAbsorbAnimationActive() const { return m_isAbsorbAnimationActive; }

    /**
     * @brief 切换背景
     */
    void changeBackground(const QString& backgroundPath);

    /**
     * @brief 渐变背景
     */
    void fadeBackgroundTo(const QString& imagePath, int duration);

    /**
     * @brief 渐变对话背景
     */
    void fadeDialogBackgroundTo(const QString& imagePath, int duration);

    /**
     * @brief 显示阶段转换文字
     */
    void showPhaseTransitionText(const QString& text, const QColor& color = QColor(75, 0, 130));

    /**
     * @brief 暂停所有敌人定时器
     */
    void pauseAllEnemyTimers(const QVector<QPointer<Enemy>>& enemies);

    /**
     * @brief 恢复所有敌人定时器
     */
    void resumeAllEnemyTimers(const QVector<QPointer<Enemy>>& enemies);

   signals:
    /**
     * @brief 请求显示对话
     */
    void requestShowDialog(const QStringList& dialogs, bool isBossDialog, const QString& background);

    /**
     * @brief 请求召唤敌人
     */
    void requestSpawnEnemies(const QVector<QPair<QString, int>>& enemies);

    /**
     * @brief 请求渐变背景
     */
    void requestFadeBackground(const QString& path, int duration);

    /**
     * @brief 吸纳动画完成
     */
    void absorbAnimationCompleted();

    /**
     * @brief Boss请求对话背景切换（TeacherBoss特殊需求）
     */
    void dialogBackgroundChangeRequested(int dialogIndex, const QString& backgroundName);

   private slots:
    void onWashMachineBossRequestDialog(const QStringList& dialogs, const QString& background);
    void onWashMachineBossRequestChangeBackground(const QString& backgroundPath);
    void onWashMachineBossRequestAbsorb();

    void onTeacherBossRequestDialog(const QStringList& dialogs, const QString& background);
    void onTeacherBossRequestChangeBackground(const QString& backgroundPath);
    void onTeacherBossRequestTransitionText(const QString& text);
    void onTeacherBossRequestFadeBackground(const QString& backgroundPath, int duration);
    void onTeacherBossRequestFadeDialogBackground(const QString& backgroundPath, int duration);
    void onTeacherBossRequestDialogBackgroundChange(int dialogIndex, const QString& backgroundName);

    void onAbsorbAnimationStep();

   private:
    /**
     * @brief 连接WashMachineBoss信号
     */
    void connectWashMachineBossSignals(WashMachineBoss* boss);

    /**
     * @brief 连接TeacherBoss信号
     */
    void connectTeacherBossSignals(TeacherBoss* boss);

    Player* m_player = nullptr;
    QGraphicsScene* m_scene = nullptr;

    // Boss引用
    QPointer<WashMachineBoss> m_currentWashMachineBoss;
    QPointer<TeacherBoss> m_currentTeacherBoss;

    // 战斗状态
    bool m_bossDefeated = false;
    bool m_rewardSequenceActive = false;

    // 背景管理
    QGraphicsPixmapItem* m_backgroundItem = nullptr;
    QGraphicsPixmapItem* m_backgroundOverlay = nullptr;
    QString m_currentBackgroundPath;
    QString m_originalBackgroundPath;

    // 吸纳动画相关
    bool m_isAbsorbAnimationActive = false;
    QTimer* m_absorbAnimationTimer = nullptr;
    QVector<QGraphicsItem*> m_absorbingItems;
    QVector<QPointF> m_absorbStartPositions;
    QVector<double> m_absorbAngles;
    QPointF m_absorbCenter;
    int m_absorbAnimationStep = 0;

    // 对话中背景切换相关（TeacherBoss特殊需求）
    QMap<int, QString> m_pendingDialogBackgrounds;
    QString m_pendingFadeDialogBackground;
    int m_pendingFadeDialogDuration = 0;
};

#endif  // BOSSFIGHT_H
