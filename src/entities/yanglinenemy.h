#ifndef YANGLINENEMY_H
#define YANGLINENEMY_H

#include "scalingenemy.h"

class Player;

/**
 * @brief 杨林 - 第三关精英小怪
 *
 * 特性：
 * - 继承自ScalingEnemy，拥有持续缩放效果（与optimization类似）
 * - 属性接近Boss一阶段（高血量、高伤害）
 * - 只有一个阶段，无阶段切换
 * - 拥有spinning技能：
 *   - 与PantsEnemy类似机制，但不形成可视的像素圆
 *   - 伤害判定仍然是圆形区域
 *   - 圆的大小随杨林的缩放而变化（基本上是能包住自己的最小圆）
 *   - 开局10秒后释放第一次，之后每30秒释放一次，每次持续5秒
 */
class YanglinEnemy : public ScalingEnemy {
Q_OBJECT

public:
    explicit YanglinEnemy(const QPixmap &pic, double scale = 1.0);

    ~YanglinEnemy() override;

    // 重写受伤方法
    void takeDamage(int damage) override;

    // 暂停/恢复定时器
    void pauseTimers() override;

    void resumeTimers() override;

protected:
    void attackPlayer() override;

private slots:

    void onSpinningTimer();  // 技能冷却定时器（每30秒）
    void onSpinningUpdate(); // 旋转动画更新
    void onSpinningEnd();    // 技能结束
    void onFirstSpinning();  // 开局10秒后第一次释放

private:
    void startSpinning();                    // 开始旋转技能
    void stopSpinning();                     // 停止旋转技能
    void checkSpinningDamage();              // 检测旋转圆是否碰到玩家
    double getCurrentSpinningRadius() const; // 获取当前缩放下的旋转伤害半径

    // 旋转技能相关
    bool m_isSpinning;               // 是否正在释放旋转技能
    QTimer *m_spinningCooldownTimer; // 技能冷却定时器（30秒）
    QTimer *m_spinningUpdateTimer;   // 旋转动画更新定时器
    QTimer *m_spinningDurationTimer; // 技能持续时间定时器（5秒）
    QTimer *m_firstSpinningTimer;    // 开局10秒定时器

    // 旋转动画相关
    double m_rotationAngle;     // 当前旋转角度
    double m_originalSpeed;     // 原始移速（用于恢复）
    bool m_isReturningToNormal; // 是否正在回正角度

    // 技能参数（杨林专属）- 与pants类似但有调整
    static constexpr int FIRST_SPINNING_DELAY = 10000;       // 开局10秒后第一次释放
    static constexpr int SPINNING_COOLDOWN = 30000;          // 技能冷却时间（30秒）
    static constexpr int SPINNING_DURATION = 5000;           // 技能持续时间（5秒）
    static constexpr int SPINNING_UPDATE_INTERVAL = 50;      // 旋转动画更新间隔（毫秒）
    static constexpr double SPINNING_SPEED_MULTIPLIER = 1.5; // 技能期间移速倍率（与pants相同）
    static constexpr double BASE_SPINNING_RADIUS = 35.0;     // 基础旋转圆半径（比pants的80小，刚好包住自己）
    static constexpr int SPINNING_DAMAGE = 3;                // 旋转圆伤害
    static constexpr double ROTATION_SPEED = 15.0;           // 旋转速度（度/帧）

    qint64 m_lastSpinningDamageTime; // 上次旋转伤害时间（用于伤害间隔）
};

#endif // YANGLINENEMY_H
