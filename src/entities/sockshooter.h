#ifndef SOCKSHOOTER_H
#define SOCKSHOOTER_H

#include <QGraphicsPixmapItem>
#include "enemy.h"

class Player;
class Projectile;

/**
 * @brief 射击袜子怪物 - 远程攻击敌人
 * 特性：
 * - 只能远程攻击，不进行近战
 * - 使用 MOVE_KEEP_DISTANCE 移动方式与玩家保持距离
 * - 只向水平方向（面朝方向，即图片右侧）发射子弹
 * - 攻击参数可自由调整
 */
class SockShooter : public Enemy
{
    Q_OBJECT

public:
    explicit SockShooter(const QPixmap &pic, double scale = 1.0);
    ~SockShooter() override;

    // 暂停/恢复定时器
    void pauseTimers() override;
    void resumeTimers() override;

    // ========== 攻击参数设置接口（方便调整） ==========

    // 设置子弹伤害
    void setBulletDamage(int damage) { m_bulletDamage = damage; }
    int getBulletDamage() const { return m_bulletDamage; }

    // 设置射击冷却时间（毫秒）
    void setShootCooldown(int ms) { m_shootCooldown = ms; }
    int getShootCooldown() const { return m_shootCooldown; }

    // 设置子弹速度
    void setBulletSpeed(double speed) { m_bulletSpeed = speed; }
    double getBulletSpeed() const { return m_bulletSpeed; }

    // 设置子弹缩放
    void setBulletScale(double scale) { m_bulletScale = scale; }
    double getBulletScale() const { return m_bulletScale; }

    // 设置与玩家保持的理想距离
    void setKeepDistance(double dist) { setPreferredDistance(dist); }

protected:
    void attackPlayer() override;
    // 使用基类 Enemy::moveKeepDistance() 移动逻辑（已重构支持Y轴对齐）

private slots:
    void shootBullet();

private:
    void loadBulletPixmap();
    void updateFacingDirection();

    // 射击相关
    QTimer *m_shootTimer;   // 射击定时器
    QPixmap m_bulletPixmap; // 子弹图片
    bool m_facingRight;     // 面朝方向（true=右，false=左）

    // ========== 可调整的攻击参数 ==========
    int m_bulletDamage;   // 子弹伤害（默认：2）
    int m_shootCooldown;  // 射击冷却（默认：1500ms）
    double m_bulletSpeed; // 子弹速度（默认：5.0）
    double m_bulletScale; // 子弹缩放（默认：1.0）

    // ========== 默认参数常量 ==========
    static constexpr int DEFAULT_BULLET_DAMAGE = 2;
    static constexpr int DEFAULT_SHOOT_COOLDOWN = 1500; // 1.5秒
    static constexpr double DEFAULT_BULLET_SPEED = 8.0;
    static constexpr double DEFAULT_BULLET_SCALE = 1.0;
    static constexpr double DEFAULT_KEEP_DISTANCE = 200.0; // 与玩家保持200像素距离
    static constexpr double DEFAULT_VISION_RANGE = 400.0;  // 视野范围
};

#endif // SOCKSHOOTER_H
