#ifndef PLAYER_H
#define PLAYER_H

#include <QGraphicsPixmapItem>
#include <QKeyEvent>
#include <QPointF>
#include <QTimer>
#include <QVector>
#include "../core/audiomanager.h"
#include "constants.h"
#include "entity.h"
#include "projectile.h"

const int max_red_contain = 12;
const int max_soul = 6;
const int bomb_r = 100;
const int bombHurt = 5;

class Player : public Entity {
    Q_OBJECT
    int redContainers;
    double redHearts;
    int blackHearts;                   // 黑心数量（用于复活）
    QMap<int, bool> shootKeysPressed;  // 射击按键状态
    QTimer* keysTimer;
    QTimer* crashTimer;
    QTimer* shootTimer;  // 射击检测定时器（持续检测）
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

    // 新道具系统
    int m_frostChance;                    // 寒冰子弹概率 (0-60)
    int m_shieldCount;                    // 护盾数量
    QGraphicsPixmapItem* m_shieldSprite;  // 护盾图案
    QPixmap m_frostBulletPic;             // 寒冰子弹图片

   public:
    friend class Item;

    QMap<int, bool> keysPressed;

    explicit Player(const QPixmap& pic_player, double scale = 1.0);

    void keyPressEvent(QKeyEvent* event) override;  // 控制移动
    void keyReleaseEvent(QKeyEvent* event) override;

    void move() override;

    void tryTeleport();       // Q键瞬移
    void activateUltimate();  // E键大招

    void setBulletPic(const QPixmap& pic) { pic_bullet = pic; };

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
        redHearts = qMin(redHearts + n, static_cast<double>(redContainers));
        emit healthChanged(redHearts, getMaxHealth());
    }

    // 强制设置当前血量（用于特殊效果）
    void setCurrentHealth(double health) {
        redHearts = qBound(0.0, health, static_cast<double>(redContainers));
        emit healthChanged(redHearts, getMaxHealth());
    }

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

    int getBombs() { return bombs; };

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

    // 寒冰子弹系统
    void addFrostChance(int amount);
    [[nodiscard]] int getFrostChance() const { return m_frostChance; }
    void setFrostBulletPic(const QPixmap& pic) { m_frostBulletPic = pic; }
    [[nodiscard]] const QPixmap& getFrostBulletPic() const { return m_frostBulletPic; }

    // 护盾系统
    void addShield(int count);
    void removeShield(int count = 1);
    [[nodiscard]] int getShieldCount() const { return m_shieldCount; }
    void updateShieldDisplay();

    // 黑心复活系统
    bool tryBlackHeartRevive();  // 尝试使用黑心复活，返回是否成功

   signals:
    void blackHeartReviveStarted();   // 黑心复活动画开始信号
    void blackHeartReviveFinished();  // 黑心复活动画结束信号
    void playerDied();                // 玩家死亡信号
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

    bool m_isUltimateActive = false;       // 增伤技能是否进行中
    qint64 m_lastUltimateTime = 0;         // 上次技能释放时间
    int m_ultimateCooldownMs = 60000;      // 技能冷却60秒
    int m_ultimateDurationMs = 10000;      // 技能持续10秒
    int m_ultimateOriginalBulletHurt = 0;  // 技能前的伤害
    double m_bulletScaleMultiplier = 2.0;  // 技能期间子弹缩放倍率
    QPixmap m_originalBulletPic;           // 原始子弹图片
    QTimer* m_ultimateTimer = nullptr;     // 技能持续计时

    QPointF currentMoveDirection() const;

    QPointF clampPositionWithinRoom(const QPointF& candidate) const;

    void endUltimate();

   protected:
    void focusOutEvent(QFocusEvent* event) override;
};

#endif  // PLAYER_H
