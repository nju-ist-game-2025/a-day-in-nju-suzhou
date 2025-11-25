#include "sockenemy.h"
#include <QDebug>
#include <QRandomGenerator>
#include "player.h"
#include "statuseffect.h"

// 袜子怪物基类
SockEnemy::SockEnemy(const QPixmap& pic, double scale)
    : Enemy(pic, scale) {
    // 基础袜子怪物属性
}

void SockEnemy::attackPlayer() {
    if (!player)
        return;

    // 近战攻击：检测碰撞
    QList<QGraphicsItem*> collisions = collidingItems();
    for (QGraphicsItem* item : collisions) {
        Player* p = dynamic_cast<Player*>(item);
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

    // 50%概率触发中毒效果
    if (QRandomGenerator::global()->bounded(2) == 0) {
        int duration = (3 > player->getCurrentHealth() * 2 ? (int)player->getCurrentHealth() : 3);
        PoisonEffect* effect = new PoisonEffect(player, duration, 1);
        effect->applyTo(player);
        qDebug() << "袜子怪物触发中毒效果！持续" << duration << "秒";
    }
}

// 普通袜子怪物 - 使用斜向移动躲避子弹
SockNormal::SockNormal(const QPixmap& pic, double scale)
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
SockAngrily::SockAngrily(const QPixmap& pic, double scale)
    : SockEnemy(pic, scale) {
    // 愤怒袜子的属性（伤害和移速提升150%）
    setHealth(10);
    setContactDamage(1 + static_cast<int>(1 * 0.5));
    setVisionRange(250.0);
    setAttackRange(40.0);
    setAttackCooldown(1000);
    setSpeed(2.0 + 2.0 * 0.5);

    // 使用冲刺模式，蓄力后快速冲向玩家
    setMovementPattern(MOVE_DASH);
    setDashChargeTime(1200);  // 1.2秒蓄力
    setDashSpeed(5.0);        // 高速冲刺
}
