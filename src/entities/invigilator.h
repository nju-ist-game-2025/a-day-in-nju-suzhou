#ifndef INVIGILATOR_H
#define INVIGILATOR_H

#include <QTimer>
#include "enemy.h"

class TeacherBoss;

/**
 * @brief Invigilator - 监考员小怪
 * 特性：
 * - 默认状态：围绕奶牛张Boss巡逻
 * - 发现玩家：切换到愤怒状态，冲向玩家
 * - 接触伤害：1点
 */
class Invigilator : public Enemy {
Q_OBJECT

public:
    explicit Invigilator(const QPixmap &normalPic, const QPixmap &angryPic, TeacherBoss *master, double scale = 1.0);

    ~Invigilator() override;

    // 重写移动方法
    void move() override;

    // 暂停/恢复
    void pauseTimers() override;

    void resumeTimers() override;

    // 获取主人
    TeacherBoss *getMaster() const { return m_master; }

    // 设置巡逻参数
    void setPatrolRadius(double radius) { m_patrolRadius = radius; }

    void setPatrolSpeed(double speed) { m_patrolSpeed = speed; }

private:
    // 状态
    enum InvigilatorState {
        PATROL,  // 巡逻状态
        CHASE    // 追击状态
    };

    TeacherBoss *m_master;     // 奶牛张Boss
    InvigilatorState m_state;  // 当前状态
    QPixmap m_normalPixmap;    // 普通图片
    QPixmap m_angryPixmap;     // 愤怒图片

    // 巡逻参数
    double m_patrolAngle;   // 当前巡逻角度（弧度）
    double m_patrolRadius;  // 巡逻半径
    double m_patrolSpeed;   // 巡逻速度（弧度/帧）
    QTimer *m_patrolTimer;  // 巡逻更新定时器

    // 视野参数
    double m_detectionRange;  // 发现玩家的距离

    // 私有方法
    void updatePatrol();          // 更新巡逻位置
    void checkPlayerDetection();  // 检测玩家
    void switchToChase();         // 切换到追击状态

private slots:

    void onPatrolTimer();
};

#endif  // INVIGILATOR_H
