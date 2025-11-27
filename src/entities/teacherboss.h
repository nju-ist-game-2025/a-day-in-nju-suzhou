#ifndef TEACHERBOSS_H
#define TEACHERBOSS_H

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include "boss.h"

class Invigilator;

class Player;

/**
 * @brief TeacherBoss - 奶牛张Boss（第三关）
 *
 * 三个阶段：
 * 1. 授课阶段 (100%~60%血量): 保持距离 + 正态分布弹幕 + 随机点名
 * 2. 期中考试阶段 (60%~30%血量): 冲刺模式 + 考卷攻击 + 陷阱 + 召唤监考员
 * 3. 调离阶段 (30%~0%血量): 高速移动 + 挂科警告 + 环形弹幕 + 分裂弹
 */
class TeacherBoss : public Boss {
Q_OBJECT

public:
    explicit TeacherBoss(const QPixmap &pic, double scale = 1.5);

    ~TeacherBoss() override;

    void takeDamage(int damage) override;

    void move() override;

    void attackPlayer() override;

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
    
    // 请求渐变背景（用于boss击败后的渐变效果）
    void requestFadeBackground(const QString &backgroundPath, int duration);
    
    // 请求渐变对话背景（用于boss击败后对话框背景的渐变效果）
    void requestFadeDialogBackground(const QString &backgroundPath, int duration);
    
    // 请求对话过程中切换对话背景（在指定对话索引时瞬间切换）
    void requestDialogBackgroundChange(int dialogIndex, const QString &newBackground);

    void requestShowTransitionText(const QString &text);

    // 阶段变化通知
    void phaseChanged(int newPhase);

    // 对话结束后继续战斗
    void dialogFinished();

public slots:

    // 对话结束后调用，继续战斗
    void onDialogFinished();

    // 飞出动画完成后调用
    void onFlyOutComplete();

private:
    // ========== 阶段管理 ==========
    int m_phase;              // 1=授课, 2=期中考试, 3=调离
    bool m_isTransitioning;   // 阶段转换中（无敌）
    bool m_waitingForDialog;  // 等待对话结束
    bool m_isDefeated;        // Boss已被击败，等待对话结束后死亡
    bool m_firstDialogShown;  // 是否已显示过初始对话
    bool m_isFlyingOut;       // 正在执行飞出动画
    bool m_isFlyingIn;        // 正在执行飞入动画
    QPointF m_flyStartPos;    // 飞行起始位置
    QPointF m_flyTargetPos;   // 飞行目标位置
    QTimer *m_flyTimer;       // 飞行动画定时器

    // ========== 图片资源 ==========
    QPixmap m_normalPixmap;         // cow.png - 授课阶段
    QPixmap m_angryPixmap;          // cowAngry.png - 期中考试阶段
    QPixmap m_finalPixmap;          // cowFinal.png - 调离阶段
    QPixmap m_formulaBulletPixmap;  // 公式弹幕图片
    QPixmap m_chalkBeamPixmap;      // 粉笔光束图片
    QPixmap m_examPaperPixmap;      // 考卷图片
    QPixmap m_finalBulletPixmap;    // 分裂弹图片

    // 场景引用
    QGraphicsScene *m_scene;

    // ========== 第一阶段：授课阶段 ==========
    QTimer *m_normalBarrageTimer;  // 正态分布弹幕定时器
    QTimer *m_rollCallTimer;       // 随机点名定时器

    void startPhase1Skills();

    void stopPhase1Skills();

    void fireNormalDistributionBarrage();  // 发射正态分布弹幕
    void performRollCall();                // 随机点名（红圈+粉笔光束）

    // ========== 第二阶段：期中考试阶段 ==========
    QTimer *m_examPaperTimer;                       // 考卷定时器
    QTimer *m_mleTrapTimer;                         // 极大似然估计陷阱定时器
    QTimer *m_summonInvigilatorTimer;               // 召唤监考员定时器
    QVector<QPointer<Invigilator>> m_invigilators;  // 监考员列表

    void startPhase2Skills();

    void stopPhase2Skills();

    void throwExamPaper();       // 抛出考卷
    void placeMleTrap();         // 放置预判陷阱
    void summonInvigilator();    // 召唤监考员
    void cleanupInvigilators();  // 清理监考员

    // ========== 第三阶段：调离阶段 ==========
    QTimer *m_failWarningTimer;  // 挂科警告定时器
    QTimer *m_formulaBombTimer;  // 公式轰炸定时器
    QTimer *m_splitBulletTimer;  // 喜忧参半分裂弹定时器

    void startPhase3Skills();

    void stopPhase3Skills();

    void performFailWarning();  // 挂科警告（追踪红圈）
    void fireFormulaBomb();     // 环形弹幕
    void fireSplitBullet();     // 分裂弹

    // ========== 阶段转换 ==========
    void checkPhaseTransition();

    void enterPhase2();           // 进入期中考试阶段
    void enterPhase3();           // 进入调离阶段
    void startFlyOutAnimation();  // 飞出画面动画
    void startFlyInAnimation();   // 飞入画面动画
    void onFlyAnimationStep();    // 飞行动画每帧
    void onBossDefeated();        // Boss被击败

    // ========== 对话内容 ==========
    QStringList getInitialDialog();   // 初始对话
    QStringList getPhase2Dialog();    // 期中考试阶段对话
    QStringList getPhase3Dialog();    // 调离阶段对话
    QStringList getDefeatedDialog();  // 被击败对话

    // ========== 辅助方法 ==========
    void loadTextures();                              // 加载图片资源
    double randomNormal(double mean, double stddev);  // 正态分布随机数
};

#endif  // TEACHERBOSS_H
