#ifndef SCALINGENEMY_H
#define SCALINGENEMY_H

#include "enemy.h"

/**
 * @brief 缩放敌人基类 - 用于第三关的optimization和digital_system敌人
 *
 * 特点：
 * 1. 使用绕圈寻敌模式（MOVE_CIRCLE）
 * 2. 模型大小在一定范围内来回变化（伸缩变化）
 * 3. 碰撞判定范围随大小变化
 * 4. 攻击玩家后有50%概率触发昏迷效果（与PillowEnemy相同）
 */
class ScalingEnemy : public Enemy
{
    Q_OBJECT

public:
    explicit ScalingEnemy(const QPixmap &pic, double scale = 1.0);
    ~ScalingEnemy() override;

    // 接触玩家时50%概率触发昏睡效果
    void onContactWithPlayer(Player *p) override;

    // 暂停/恢复定时器
    void pauseTimers() override;
    void resumeTimers() override;

protected:
    void attackPlayer() override;

private slots:
    void updateScaling(); // 更新缩放大小

private:
    void applySleepEffect(); // 50%概率触发昏睡效果

    QTimer *m_scalingTimer;   // 缩放动画定时器
    double m_baseScale;       // 基础缩放比例
    double m_minScale;        // 最小缩放比例
    double m_maxScale;        // 最大缩放比例
    double m_currentScale;    // 当前缩放比例
    double m_scaleSpeed;      // 缩放速度
    bool m_scalingUp;         // 是否正在放大
    QPixmap m_originalPixmap; // 原始图片（用于缩放计算）
};

#endif // SCALINGENEMY_H
