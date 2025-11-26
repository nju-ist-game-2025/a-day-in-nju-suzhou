#ifndef SOCKENEMY_H
#define SOCKENEMY_H

#include <QMap>
#include "enemy.h"

class Player;

/**
 * @brief 袜子怪物基类
 * 特性：攻击有50%概率触发中毒效果
 * 中毒效果：每秒扣1点血，持续3秒（最多扣至玩家当前血量）
 * 冷却机制：中毒结束后3秒内无法再次被袜子中毒
 */
class SockEnemy : public Enemy {
Q_OBJECT

public:
    explicit SockEnemy(const QPixmap &pic, double scale = 1.0);

    ~SockEnemy() override = default;

    // 静态方法：检查玩家是否可以被中毒（冷却检测）
    static bool canApplyPoisonTo(Player *player);

    // 静态方法：标记玩家中毒冷却开始（中毒结束时调用）
    static void markPoisonCooldownStart(Player *player);

    // 静态方法：清除所有冷却记录
    static void clearAllCooldowns();

protected:
    void attackPlayer() override;

    void applyPoisonEffect();

private:
    // 静态冷却管理：记录玩家中毒冷却结束时间
    static QMap<Player *, qint64> s_playerPoisonCooldowns;
    // 冷却时间常量（毫秒）
    static constexpr int POISON_COOLDOWN_MS = 3000; // 3秒冷却
};

/**
 * @brief 普通袜子怪物 - 使用默认敌人机制
 */
class SockNormal : public SockEnemy {
Q_OBJECT

public:
    explicit SockNormal(const QPixmap &pic, double scale = 1.0);

    ~SockNormal() override = default;
};

/**
 * @brief 愤怒袜子怪物 - 移速和伤害提升150%
 */
class SockAngrily : public SockEnemy {
Q_OBJECT

public:
    explicit SockAngrily(const QPixmap &pic, double scale = 1.0);

    ~SockAngrily() override = default;
};

#endif // SOCKENEMY_H
