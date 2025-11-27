#include "pillowenemy.h"
#include <QDebug>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPointer>
#include <QTimer>
#include "player.h"

PillowEnemy::PillowEnemy(const QPixmap &pic, double scale)
        : Enemy(pic, scale) {
    // 设置移动模式为绕圈移动
    setMovementPattern(MOVE_CIRCLE);

    // 设置绕圈半径和速度等参数
    setCircleRadius(200.0);  // 绕圈半径
    setSpeed(3.0);           // 移动速度
    setHealth(20);           // 生命值
    setContactDamage(2);     // 接触伤害
}

void PillowEnemy::onContactWithPlayer(Player *p) {
    Q_UNUSED(p);
    // 接触时100%触发昏睡效果
    applySleepEffect();
}

void PillowEnemy::attackPlayer() {
    if (!player)
        return;

    // 暂停状态下不攻击
    if (m_isPaused)
        return;

    // 近战攻击：使用像素级碰撞检测
    if (Entity::pixelCollision(this, player)) {
        player->takeDamage(contactDamage);

        // 100%概率触发昏睡效果（枕头必定让人昏睡）
        applySleepEffect();
    }
}

void PillowEnemy::applySleepEffect() {
    if (!player || !scene())
        return;

    // 检查玩家是否已经处于昏睡状态（不可叠加）
    if (!player->canMove())
        return;

    // 检查效果冷却（触发后0.5秒内不可再次触发）
    if (player->isEffectOnCooldown())
        return;

    qDebug() << "枕头怪物触发昏睡效果！玩家无法移动1.5秒";

    // 立即设置效果冷却，防止重复触发（将在效果结束后解除）
    player->setEffectCooldown(true);

    // 显示"昏睡ZZZ"文字提示
    QGraphicsTextItem *sleepText = new QGraphicsTextItem("昏睡ZZZ");
    QFont font;
    font.setPointSize(16);
    font.setBold(true);
    sleepText->setFont(font);
    sleepText->setDefaultTextColor(QColor(128, 128, 128));  // 灰色
    sleepText->setPos(player->pos().x(), player->pos().y() - 40);
    sleepText->setZValue(200);
    scene()->addItem(sleepText);

    // 禁用玩家移动
    player->setCanMove(false);

    // 使用QPointer保护player指针，确保即使PillowEnemy被删除也能恢复玩家移动
    QPointer<Player> playerPtr = player;

    // 1.5秒后恢复移动并删除文字（不再依赖this指针）
    QTimer::singleShot(1500, [playerPtr, sleepText]() {
        if (playerPtr) {
            playerPtr->setCanMove(true);
            qDebug() << "昏睡效果结束，玩家恢复移动，3秒后可再次触发";
            // 效果结束后3秒再解除冷却
            QTimer::singleShot(3000, [playerPtr]() {
                if (playerPtr) {
                    playerPtr->setEffectCooldown(false);
                }
            });
        }
        if (sleepText) {
            if (sleepText->scene()) {
                sleepText->scene()->removeItem(sleepText);
            }
            delete sleepText;
        }
    });
}
