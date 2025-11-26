#include "sockenemy.h"
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include <QTimer>
#include "player.h"
#include "statuseffect.h"

// 静态成员初始化
QMap<Player *, qint64> SockEnemy::s_playerPoisonCooldowns;

// 袜子怪物基类
SockEnemy::SockEnemy(const QPixmap &pic, double scale)
        : Enemy(pic, scale) {
    // 基础袜子怪物属性
}

bool SockEnemy::canApplyPoisonTo(Player *player) {
    if (!player)
        return false;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (s_playerPoisonCooldowns.contains(player)) {
        qint64 cooldownEndTime = s_playerPoisonCooldowns[player];
        if (currentTime < cooldownEndTime) {
            return false; // 仍在冷却中
        }
    }
    return true;
}

void SockEnemy::markPoisonCooldownStart(Player *player) {
    // 中毒结束后开始3秒冷却
    s_playerPoisonCooldowns[player] = QDateTime::currentMSecsSinceEpoch() + POISON_COOLDOWN_MS;
}

void SockEnemy::clearAllCooldowns() {
    s_playerPoisonCooldowns.clear();
}

void SockEnemy::attackPlayer() {
    if (!player)
        return;

    // 暂停状态下不攻击
    if (m_isPaused)
        return;

    // 近战攻击：检测碰撞
    QList<QGraphicsItem *> collisions = collidingItems();
    for (QGraphicsItem *item: collisions) {
        Player *p = dynamic_cast<Player *>(item);
        if (p) {
            p->takeDamage(contactDamage);

            // 袜子怪物的特殊效果：50%概率触发中毒
            applyPoisonEffect();

            break;
        }
    }
}

void SockEnemy::applyPoisonEffect() {
    if (!player || player->getCurrentHealth() < 1)
        return;

    // 检查冷却时间
    if (!canApplyPoisonTo(player)) {
        qDebug() << "袜子中毒冷却中，无法再次中毒";
        return;
    }

    // 50%概率触发中毒效果
    if (QRandomGenerator::global()->bounded(2) == 0) {
        // 中毒效果：每秒扣0.5颗心，持续3秒共扣1.5心
        int duration = qMin(3, static_cast<int>(player->getCurrentHealth()));
        PoisonEffect *effect = new PoisonEffect(player, duration, 1);
        effect->applyTo(player);

        // 中毒结束后开始冷却（duration秒后 + 3秒冷却）
        // 使用QTimer延迟设置冷却开始时间
        QTimer::singleShot(duration * 1000, [this]() {
            if (player) {
                markPoisonCooldownStart(player);
                qDebug() << "袜子中毒结束，开始3秒冷却";
            }
        });

        qDebug() << "袜子怪物触发中毒效果！持续" << duration << "秒，每秒扣1点血";
        StatusEffect::showFloatText(player->scene(), QString("中毒！"), player->pos(), QColor(100, 50, 0));
    }
}

// 普通袜子怪物 - 使用斜向移动躲避子弹
SockNormal::SockNormal(const QPixmap &pic, double scale)
        : SockEnemy(pic, scale) {
    // 普通袜子的属性（与基础敌人相同）
    setHealth(10);
    setContactDamage(1);
    setVisionRange(250.0);
    setAttackRange(40.0);
    setAttackCooldown(1000);
    setSpeed(2.0);

    // 使用斜向移动模式，斜着接近玩家以躲避直线子弹
    setMovementPattern(MOVE_DIAGONAL);
}

// 愤怒袜子怪物 - 移速和伤害提升150%，使用冲刺攻击
SockAngrily::SockAngrily(const QPixmap &pic, double scale)
        : SockEnemy(pic, scale) {
    // 愤怒袜子的属性（伤害和移速提升150%）
    setHealth(18);
    setContactDamage(2);
    setVisionRange(250.0);
    setAttackRange(40.0);
    setAttackCooldown(1000);
    setSpeed(2.0 + 2.0 * 0.5);

    // 使用冲刺模式，蓄力后快速冲向玩家
    setMovementPattern(MOVE_DASH);
    setDashChargeTime(1200); // 1.2秒蓄力
    setDashSpeed(5.0);       // 高速冲刺
}
