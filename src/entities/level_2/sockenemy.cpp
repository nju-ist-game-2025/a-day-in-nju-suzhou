#include "sockenemy.h"
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include <QTimer>
#include "../../core/configmanager.h"
#include "../../items/statuseffect.h"
#include "../player.h"

// 静态成员初始化
QMap<Player*, qint64> SockEnemy::s_playerPoisonCooldowns;

// 袜子怪物基类
SockEnemy::SockEnemy(const QPixmap& pic, double scale)
    : Enemy(pic, scale) {
    // 基础袜子怪物属性（从config读取）
}

bool SockEnemy::canApplyPoisonTo(Player* player) {
    if (!player)
        return false;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (s_playerPoisonCooldowns.contains(player)) {
        qint64 cooldownEndTime = s_playerPoisonCooldowns[player];
        if (currentTime < cooldownEndTime) {
            return false;  // 仍在冷却中
        }
    }
    return true;
}

void SockEnemy::markPoisonCooldownStart(Player* player) {
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

    // 近战攻击：使用像素级碰撞检测
    if (Entity::pixelCollision(this, player)) {
        player->takeDamage(contactDamage);

        // 袜子怪物的特殊效果：50%概率触发中毒
        applyPoisonEffect();
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
        PoisonEffect* effect = new PoisonEffect(player, duration, 1);
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
SockNormal::SockNormal(const QPixmap& pic, double scale)
    : SockEnemy(pic, scale) {
    // 从配置文件读取普通袜子属性
    ConfigManager& config = ConfigManager::instance();
    setHealth(config.getEnemyInt("sock_normal", "health", 10));
    setContactDamage(config.getEnemyInt("sock_normal", "contact_damage", 1));
    setVisionRange(config.getEnemyDouble("sock_normal", "vision_range", 250.0));
    setAttackRange(config.getEnemyDouble("sock_normal", "attack_range", 40.0));
    setAttackCooldown(config.getEnemyInt("sock_normal", "attack_cooldown", 1000));
    setSpeed(config.getEnemyDouble("sock_normal", "speed", 2.0));

    // 使用斜向移动模式，斜着接近玩家以躲避直线子弹
    setMovementPattern(MOVE_DIAGONAL);
}

// 愤怒袜子怪物 - 移速和伤害提升150%，使用冲刺攻击
SockAngrily::SockAngrily(const QPixmap& pic, double scale)
    : SockEnemy(pic, scale) {
    // 从配置文件读取愤怒袜子属性
    ConfigManager& config = ConfigManager::instance();
    setHealth(config.getEnemyInt("sock_angrily", "health", 18));
    setContactDamage(config.getEnemyInt("sock_angrily", "contact_damage", 2));
    setVisionRange(config.getEnemyDouble("sock_angrily", "vision_range", 250.0));
    setAttackRange(config.getEnemyDouble("sock_angrily", "attack_range", 40.0));
    setAttackCooldown(config.getEnemyInt("sock_angrily", "attack_cooldown", 1000));
    setSpeed(config.getEnemyDouble("sock_angrily", "speed", 3.0));

    // 使用冲刺模式，蓄力后快速冲向玩家
    setMovementPattern(MOVE_DASH);
    setDashChargeTime(config.getEnemyInt("sock_angrily", "dash_charge_time", 1200));
    setDashSpeed(config.getEnemyDouble("sock_angrily", "dash_speed", 5.0));
}
