#ifndef WASHMACHINEBOSS_H
#define WASHMACHINEBOSS_H

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include "boss.h"

class OrbitingSock;

class ToxicGas;

class Player;

/**
 * @brief WashMachine Boss - 洗衣机Boss（第二关）
 *
 * 三个阶段：
 * 1. 普通阶段 (100%~70%血量): 保持距离 + 四方向水柱攻击
 * 2. 愤怒阶段 (70%~40%血量): 冲刺攻击 + 召唤旋转臭袜子
 * 3. 变异阶段 (40%~0%血量): 吸纳所有实体 + 毒气团攻击
 */
class WashMachineBoss : public Boss {
Q_OBJECT

public:
    explicit WashMachineBoss(const QPixmap &pic, double scale = 1.5);

    ~WashMachineBoss() override;

    void takeDamage(int damage) override;

    void move() override;

    void attackPlayer() override;  // 重写攻击方法，吸纳期间不攻击

    // 暂停/恢复定时器
    void pauseTimers() override;

    void resumeTimers() override;

    // 获取当前阶段
    int getPhase() const { return m_phase; }

    // 设置场景（用于生成投射物）
    void setScene(QGraphicsScene *scene) { m_scene = scene; }

signals:

    // 请求Level执行操作
    void requestSpawnEnemies(const QVector<QPair<QString, int>> &enemies);

    void requestShowDialog(const QStringList &dialogs, const QString &background);

    void requestChangeBackground(const QString &backgroundPath);

    void requestAbsorbAllEntities();  // 请求吸纳所有实体

    // 阶段变化通知
    void phaseChanged(int newPhase);

    // 对话结束后继续战斗
    void dialogFinished();

public slots:

    // 对话结束后调用，继续战斗
    void onDialogFinished();

    // 吸纳动画完成后调用
    void onAbsorbComplete();

private:
    // 阶段管理
    int m_phase;              // 1=普通, 2=愤怒, 3=变异
    bool m_isTransitioning;   // 阶段转换中（无敌）
    bool m_isAbsorbing;       // 正在执行吸纳动画
    bool m_waitingForDialog;  // 等待对话结束
    bool m_isDefeated;        // Boss已被击败，等待对话结束后死亡

    // 图片资源
    QPixmap m_normalPixmap;
    QPixmap m_chargePixmap;
    QPixmap m_angryPixmap;
    QPixmap m_mutatedPixmap;
    QPixmap m_toxicGasPixmap;

    // 场景引用
    QGraphicsScene *m_scene;

    // ========== 普通阶段 - 水柱攻击 ==========
    QTimer *m_waterAttackTimer;  // 水柱攻击定时器
    bool m_isCharging;           // 是否正在蓄力
    QTimer *m_chargeTimer;       // 蓄力定时器

    void startWaterAttackCycle();

    void startCharging();

    void performWaterAttack();

    void createWaterWave(int direction);  // 0=上, 1=下, 2=左, 3=右

    // ========== 愤怒阶段 - 召唤臭袜子 ==========
    QTimer *m_summonTimer;                            // 召唤定时器
    QVector<QPointer<OrbitingSock>> m_orbitingSocks;  // 围绕的臭袜子

    void startSummonCycle();

    void summonInitialSocks();  // 进入愤怒阶段时召唤初始袜子
    void summonOrbitingSock();

    void cleanupOrbitingSocks();

    // ========== 变异阶段 - 毒气攻击 ==========
    QTimer *m_toxicGasTimer;  // 扩散毒气定时器
    QTimer *m_fastGasTimer;   // 追踪毒气定时器
    double m_spiralAngle;     // 螺旋发射角度（度）

    void startToxicGasCycle();

    void shootSpreadGas();  // 扩散模式：16个向四周
    void shootFastGas();    // 追踪模式：向玩家发射快速毒气

    // ========== 阶段转换 ==========
    void checkPhaseTransition();

    void enterPhase2();           // 进入愤怒阶段
    void enterPhase3();           // 进入变异阶段
    void startAbsorbAnimation();  // 开始吸纳动画
    void onBossDefeated();        // Boss被击败

    // ========== 对话内容 ==========
    QStringList getMutationDialog();

    QStringList getDefeatedDialog();
};

#endif  // WASHMACHINEBOSS_H
