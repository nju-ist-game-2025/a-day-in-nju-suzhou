#ifndef PANTSENEMY_H
#define PANTSENEMY_H

#include <QGraphicsEllipseItem>
#include "enemy.h"

class Player;

/**
 * @brief 裤子怪物 - 拥有旋转技能（spinning）
 * 特性：
 * - 巡敌方式为 MOVE_ZIGZAG
 * - 开局及每15秒释放一次 spinning 技能，持续5秒
 * - 技能期间：免疫移速下降、以自身为圆心快速旋转、形成伤害圆、移速大增
 * - 旋转圆接触玩家造成更高伤害
 */
class PantsEnemy : public Enemy
{
    Q_OBJECT

public:
    explicit PantsEnemy(const QPixmap &pic, double scale = 1.0);
    ~PantsEnemy() override;

    // 接触玩家时的处理
    void onContactWithPlayer(Player *p) override;

    // 暂停/恢复定时器
    void pauseTimers() override;
    void resumeTimers() override;

protected:
    void attackPlayer() override;

private slots:
    void onSpinningTimer();  // 技能冷却定时器
    void onSpinningUpdate(); // 旋转动画更新
    void onSpinningEnd();    // 技能结束

private:
    void startSpinning();        // 开始旋转技能
    void stopSpinning();         // 停止旋转技能
    void updateSpinningCircle(); // 更新旋转圆的位置
    void createRotationFrames(); // 创建旋转动画帧
    void checkSpinningDamage();  // 检测旋转圆是否碰到玩家

    // 旋转技能相关
    bool m_isSpinning;                      // 是否正在释放旋转技能
    QTimer *m_spinningCooldownTimer;        // 技能冷却定时器（15秒）
    QTimer *m_spinningUpdateTimer;          // 旋转动画更新定时器
    QTimer *m_spinningDurationTimer;        // 技能持续时间定时器（5秒）
    QGraphicsEllipseItem *m_spinningCircle; // 旋转形成的伤害圆

    // 旋转动画相关
    QVector<QPixmap> m_rotationFrames; // 旋转动画帧（不同角度）
    int m_currentFrameIndex;           // 当前动画帧索引
    QPixmap m_originalPixmap;          // 原始图片（用于恢复）
    double m_originalSpeed;            // 原始移速（用于恢复）

    // 技能参数
    static constexpr int SPINNING_COOLDOWN = 20000;          // 技能冷却时间（毫秒）
    static constexpr int SPINNING_DURATION = 5000;           // 技能持续时间（毫秒）
    static constexpr int SPINNING_UPDATE_INTERVAL = 50;      // 旋转动画更新间隔（毫秒）
    static constexpr double SPINNING_SPEED_MULTIPLIER = 2.5; // 技能期间移速倍率
    static constexpr double SPINNING_CIRCLE_RADIUS = 80.0;   // 旋转圆半径
    static constexpr int SPINNING_DAMAGE = 4;                // 旋转圆伤害
    static constexpr int ROTATION_FRAME_COUNT = 8;           // 旋转动画帧数

    qint64 m_lastSpinningDamageTime; // 上次旋转伤害时间（用于伤害间隔）
};

#endif // PANTSENEMY_H
