#ifndef XUKEENEMY_H
#define XUKEENEMY_H

#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QPointer>
#include "enemy.h"
#include "projectile.h"

class Player;

/**
 * @brief 徐科 - 第三关远程敌人（模仿SockShooter实现）
 *
 * 特性：
 * - 远程敌人，使用 MOVE_KEEP_DISTANCE 保持与玩家的距离
 * - 可以向360°发射子弹，锁定玩家当前位置
 * - 拥有技能"沙漠之鹰"：
 *   - 普通子弹(bullet_xuke1)：1点伤害，爆头3点伤害
 *   - 每第6发子弹(bullet_xuke2)：2点伤害，爆头8点伤害
 *   - 爆头判定：命中玩家y轴上20%区域
 *   - 爆头时显示棕色文字"颗秒！"或"颗秒！！！"
 */
class XukeEnemy : public Enemy
{
    Q_OBJECT

public:
    explicit XukeEnemy(const QPixmap &pic, double scale = 1.0);
    ~XukeEnemy() override;

    // 暂停/恢复定时器
    void pauseTimers() override;
    void resumeTimers() override;

    // 伤害参数（public 供 XukeProjectile 访问）
    static constexpr int NORMAL_DAMAGE = 1;           // 普通子弹伤害
    static constexpr int NORMAL_HEADSHOT_DAMAGE = 3;  // 普通子弹爆头伤害
    static constexpr int SPECIAL_DAMAGE = 2;          // 强化子弹伤害
    static constexpr int SPECIAL_HEADSHOT_DAMAGE = 8; // 强化子弹爆头伤害

protected:
    void attackPlayer() override;

private slots:
    void shootBullet();

private:
    void loadBulletPixmaps();
    void updateFacingDirection();

    // 射击相关
    QTimer *m_shootTimer;    // 射击定时器
    QPixmap m_bulletPixmap1; // 普通子弹图片
    QPixmap m_bulletPixmap2; // 强化子弹图片
    int m_shotCount;         // 已发射子弹计数
    bool m_facingRight;      // 面朝方向

    // ========== 参数常量 ==========
    static constexpr int SHOOT_COOLDOWN = 1200;    // 射击间隔 1.2秒
    static constexpr double BULLET_SPEED = 6.0;    // 子弹速度
    static constexpr double KEEP_DISTANCE = 220.0; // 与玩家保持的距离
    static constexpr double VISION_RANGE = 450.0;  // 视野范围
};

/**
 * @brief 徐科专属子弹 - 继承自Projectile，带爆头判定
 */
class XukeProjectile : public Projectile
{
    Q_OBJECT

public:
    enum BulletType
    {
        NORMAL, // 普通子弹
        SPECIAL // 强化子弹（每6发）
    };

    XukeProjectile(BulletType type, int baseDamage, QPointF pos, const QPixmap &pic,
                   double scale, Player *targetPlayer);
    ~XukeProjectile() override;

    void move() override;

private:
    void checkHeadshotAndApplyDamage(Player *player);
    void showHeadshotText(const QString &text);

    BulletType m_bulletType;
    QPointer<Player> m_targetPlayer;
    bool m_hasHit; // 是否已经命中（防止重复判定）
};

#endif // XUKEENEMY_H
