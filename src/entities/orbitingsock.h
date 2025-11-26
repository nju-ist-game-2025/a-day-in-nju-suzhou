#ifndef ORBITINGSOCK_H
#define ORBITINGSOCK_H

#include <QTimer>
#include "sockenemy.h"

class WashMachineBoss;

/**
 * @brief OrbitingSock - 围绕洗衣机Boss旋转的臭袜子
 * 特性：
 * - 围绕洗衣机公转，可为Boss挡子弹
 * - 可以正常攻击玩家（接触伤害）
 * - 当Boss死亡或袜子被击杀时消失
 */
class OrbitingSock : public SockEnemy {
    Q_OBJECT

   public:
    explicit OrbitingSock(const QPixmap& pic, WashMachineBoss* master, double scale = 1.0);
    ~OrbitingSock() override;

    // 设置轨道参数
    void setOrbitAngle(double angle) { m_orbitAngle = angle; }
    void setOrbitRadius(double radius) { m_orbitRadius = radius; }
    void setOrbitSpeed(double speed) { m_orbitSpeed = speed; }

    // 更新围绕Boss的位置
    void updateOrbit();

    // 重写移动方法，使用轨道运动而非追击玩家
    void move() override;

    // 暂停/恢复
    void pauseTimers() override;
    void resumeTimers() override;

    // 获取主人（洗衣机Boss）
    WashMachineBoss* getMaster() const { return m_master; }

   private:
    WashMachineBoss* m_master;  // 洗衣机Boss
    double m_orbitAngle;        // 当前轨道角度（弧度）
    double m_orbitRadius;       // 轨道半径
    double m_orbitSpeed;        // 旋转速度（弧度/帧）
    QTimer* m_orbitTimer;       // 轨道更新定时器
};

#endif  // ORBITINGSOCK_H
