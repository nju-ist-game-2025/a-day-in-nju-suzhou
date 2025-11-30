#ifndef SCALINGENEMY_H
#define SCALINGENEMY_H

#include "../enemy.h"

/**
 * @brief 缩放敌人基类 - 用于第三关的optimization和digital_system敌人
 *
 * 特点：
 * 1. 使用绕圈寻敌模式（MOVE_CIRCLE）
 * 2. 模型大小在一定范围内来回变化（伸缩变化）
 * 3. 碰撞判定范围随大小变化
 * 4. 攻击玩家后有50%概率触发昏迷效果（与PillowEnemy相同）
 */
class ScalingEnemy : public Enemy {
Q_OBJECT

public:
    explicit ScalingEnemy(const QPixmap &pic, double scale = 1.0);

    ~ScalingEnemy() override;

    // 接触玩家时50%概率触发昏睡效果
    void onContactWithPlayer(Player *p) override;

    // 暂停/恢复定时器
    void pauseTimers() override;

    void resumeTimers() override;

    // 覆写受伤方法，确保闪烁效果正确
    void takeDamage(int damage) override;

    // 获取碰撞边界（考虑缩放）
    QRectF boundingRect() const override;

    QPainterPath shape() const override;

    // 获取缩放参数（供子类使用）
    double getBaseScale() const { return m_baseScale; }

    double getCurrentScale() const { return m_currentScale; }

    double getTotalScale() const { return m_baseScale * m_currentScale; }

protected:
    void attackPlayer() override;

private slots:

    void updateScaling();  // 更新缩放大小
    void endFlashEffect(); // 结束闪烁效果

private:
    void applySleepEffect();    // 50%概率触发昏睡效果
    void applyFlashEffect();    // 应用闪烁效果
    void updateScaledPixmaps(); // 更新缩放后的图片

    QTimer *m_scalingTimer;   // 缩放动画定时器
    double m_baseScale;       // 基础缩放比例
    double m_minScale;        // 最小缩放比例
    double m_maxScale;        // 最大缩放比例
    double m_currentScale;    // 当前缩放比例
    double m_scaleSpeed;      // 缩放速度
    bool m_scalingUp;         // 是否正在放大
    QPixmap m_originalPixmap; // 原始未缩放图片
    QPixmap m_normalPixmap;   // 正常显示用的缩放图片
    QPixmap m_flashPixmap;    // 闪烁用的红色图片
    bool m_isFlashing;        // 是否正在闪烁
    QTimer *m_flashTimer;     // 闪烁定时器
};

#endif // SCALINGENEMY_H
