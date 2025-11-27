#ifndef PLAYER_H
#define PLAYER_H

#include <QKeyEvent>
#include <QPointF>
#include <QTimer>
#include <QVector>
#include "../core/audiomanager.h"
#include "constants.h"
#include "entity.h"
#include "projectile.h"

const int max_red_contain = 9;
const int max_soul = 6;
const int bomb_r = 60;
const int bombHurt = 1;

class Player : public Entity {
Q_OBJECT
    int redContainers;
    double redHearts;
    int soulHearts;
    int blackHearts;                   // 是否短暂无敌，防止持续攻击
    QMap<int, bool> shootKeysPressed;  // 射击按键状态
    QTimer *keysTimer;
    QTimer *crashTimer;
    QTimer *shootTimer;  // 射击检测定时器（持续检测）
    int shootCooldown;   // 射击冷却时间（毫秒）
    int shootType;       // 0=普通, 1=激光
    QPixmap pic_bullet;
    qint64 lastShootTime;  // 上次射击的时间戳
    void shoot(int key);   // 射击方法
    void checkShoot();     // 检测并执行射击

    // 需要UI显式实现的
    int bombs;
    int keys;

    int bulletHurt;  // 玩家子弹伤害，可配置
    bool isDead;     // 玩家是否已死亡

public:
    friend class Item;

    QMap<int, bool> keysPressed;

    explicit Player(const QPixmap &pic_player, double scale = 1.0);

    void keyPressEvent(QKeyEvent *event) override;  // 控制移动
    void keyReleaseEvent(QKeyEvent *event) override;

    void move() override;

    void tryTeleport();  // Q键瞬移
    void activateUltimate(); // E键大招

    void setBulletPic(const QPixmap &pic) { pic_bullet = pic; };

    void setShootCooldown(int milliseconds) { shootCooldown = milliseconds; }  // 设置射击冷却时间
    [[nodiscard]] int getShootCooldown() const { return shootCooldown; };

    void takeDamage(int damage) override;  // 减血
    void forceTakeDamage(int damage);      // 强制伤害，无视无敌状态（用于特殊攻击如爆炸）
    [[nodiscard]] double getCurrentHealth() const { return redHearts; }

    [[nodiscard]] double getMaxHealth() const { return redContainers; }

    // 开发者模式：直接设置血量上限（无限制）
    void setMaxHealth(int maxHealth) {
        redContainers = maxHealth;
        redHearts = maxHealth;
    }

    void addRedContainers(int n) {
        if (redContainers + n <= max_red_contain)
            redContainers += n;
    };

    void addRedHearts(double n) {
        if (redHearts + n <= redContainers)
            redHearts += n;
    }

    void addSoulHearts(int n) {
        if (soulHearts + n <= max_soul)
            soulHearts += n;
    };

    [[nodiscard]] int getSoulHearts() const { return soulHearts; };

    void addBlackHearts(int n) { blackHearts += n; };

    [[nodiscard]] int getBlackHearts() const { return blackHearts; };

    void setShootType(int type) { shootType = type; };

    int getTeleportCooldownMs() const { return m_teleportCooldownMs; }

    int getTeleportRemainingMs() const;

    double getTeleportReadyRatio() const;

    bool isTeleportReady() const;

    bool isUltimateReady() const;

    bool isUltimateActive() const;

    int getUltimateRemainingMs() const;

    double getUltimateReadyRatio() const;

    int getUltimateActiveRemainingMs() const;

    double getUltimateActiveRatio() const;

    void crashEnemy();

    void die();

    void setInvincible();                          // 短暂无敌（1秒）
    void setPermanentInvincible(bool invincible);  // 持久无敌（手动取消）

    void addBombs(int n) { bombs += n; };

    void placeBomb();

    void addKeys(int n) { keys += n; };

    [[nodiscard]] int getKeys() const { return keys; };

    // 玩家子弹伤害相关接口
    void setBulletHurt(int hurt) { bulletHurt = hurt; }

    [[nodiscard]] int getBulletHurt() const { return bulletHurt; }

    bool isKeyPressed(int key) const {
        return keysPressed.value(key, false);
    }

    // 控制玩家是否可以移动（用于昏睡等状态效果）
    void setCanMove(bool canMove) { m_canMove = canMove; }

    bool canMove() const { return m_canMove; }

    // 控制玩家是否可以射击（用于吸纳动画等）
    void setCanShoot(bool canShoot) { m_canShoot = canShoot; }

    bool canShoot() const { return m_canShoot; }

    // 惊吓状态控制（移动速度增加但受伤提升150%）
    void setScared(bool scared) {
        if (scared && !m_isScared) {
            m_isScared = true;
            m_originalSpeed = speed;
            m_originalDamageScale = damageScale;
            speed = m_originalSpeed * 2.0;  // 移动速度翻倍
            damageScale = 1.5;              // 受伤提升150%
        } else if (!scared && m_isScared) {
            m_isScared = false;
            speed = m_originalSpeed;
            damageScale = m_originalDamageScale;
        }
    }

    bool isScared() const { return m_isScared; }

    // 效果冷却控制（触发后0.5秒内不可再次触发）
    void setEffectCooldown(bool onCooldown) { m_effectOnCooldown = onCooldown; }

    bool isEffectOnCooldown() const { return m_effectOnCooldown; }

    // 暂停控制
    void setPaused(bool paused) { m_isPaused = paused; }

    bool isPaused() const { return m_isPaused; }

signals:

    void playerDied();  // 玩家死亡信号
    void healthChanged(float current, float max);

    void playerDamaged();

private:
    bool m_canMove = true;               // 是否可以移动
    bool m_canShoot = true;              // 是否可以射击
    bool m_isPaused = false;             // 是否暂停
    bool m_isScared = false;             // 是否处于惊吓状态
    bool m_effectOnCooldown = false;     // 效果是否在冷却中
    double m_originalSpeed = 5.0;        // 惊吓前的原始速度
    double m_originalDamageScale = 1.0;  // 惊吓前的原始伤害倍率
    qint64 m_lastTeleportTime = 0;       // 上次瞬移时间
    int m_teleportCooldownMs = 5000;     // 瞬移冷却（毫秒）
    double m_teleportDistance = 120.0;   // 瞬移距离

    bool m_isUltimateActive = false;    // 大招是否进行中
    qint64 m_lastUltimateTime = 0;      // 上次大招释放时间
    int m_ultimateCooldownMs = 20000;   // 大招冷却
    int m_ultimateDurationMs = 5000;    // 大招持续时间
    double m_ultimateSpeedMultiplier = 1.5; // 大招移速倍率
    int m_ultimateOriginalBulletHurt = 0;   // 大招前的伤害
    double m_ultimateOriginalSpeed = 0.0;   // 大招前的速度
    QTimer *m_ultimateTimer = nullptr;      // 大招持续计时

    QPointF currentMoveDirection() const;

    QPointF clampPositionWithinRoom(const QPointF &candidate) const;

    void endUltimate();

protected:
    void focusOutEvent(QFocusEvent *event) override;
};

#endif  // PLAYER_H
