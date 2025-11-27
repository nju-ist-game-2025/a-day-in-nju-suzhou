#include "clockenemy.h"
#include <QDebug>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPointer>
#include <QTimer>
#include "player.h"

ClockEnemy::ClockEnemy(const QPixmap &pic, double scale)
        : Enemy(pic, scale) {
    // 时钟怪物的基础属性（与普通敌人相同）
    setHealth(10);
    setContactDamage(2);
    setVisionRange(250.0);
    setAttackRange(40.0);
    setAttackCooldown(1000);
    setSpeed(2.0);

    // 使用Z字形移动模式，增加躲避难度
    setMovementPattern(MOVE_ZIGZAG);
    setZigzagAmplitude(60.0);
}

void ClockEnemy::onContactWithPlayer(Player *p) {
    Q_UNUSED(p);
    // 接触时触发惊吓效果
    applyScareEffect();
}

void ClockEnemy::attackPlayer() {
    if (!player)
        return;

    // 暂停状态下不攻击
    if (m_isPaused)
        return;

    // 近战攻击：使用像素级碰撞检测
    if (Entity::pixelCollision(this, player)) {
        player->takeDamage(contactDamage);

        // 触发惊吓效果（不可叠加）
        applyScareEffect();
    }
}

void ClockEnemy::applyScareEffect() {
    if (!player || !scene())
        return;

    // 检查玩家是否已经处于惊吓状态（不可叠加）
    if (player->isScared())
        return;

    // 检查效果冷却（触发后0.5秒内不可再次触发）
    if (player->isEffectOnCooldown())
        return;

    qDebug() << "时钟怪物触发惊吓效果！玩家移动速度增加但受伤提升150%，持续3秒";

    // 立即设置效果冷却，防止重复触发（将在效果结束后解除）
    player->setEffectCooldown(true);

    // 显示"惊吓！！"文字提示
    QGraphicsTextItem *scareText = new QGraphicsTextItem("惊吓！！");
    QFont font;
    font.setPointSize(16);
    font.setBold(true);
    scareText->setFont(font);
    scareText->setDefaultTextColor(QColor(128, 128, 128));  // 灰色
    scareText->setPos(player->pos().x(), player->pos().y() - 40);
    scareText->setZValue(200);
    scene()->addItem(scareText);

    // 应用惊吓效果：移动速度增加、受伤提升150%
    player->setScared(true);

    // 使用QPointer保护player指针
    QPointer<Player> playerPtr = player;

    // 创建文字跟随定时器，让文字跟随玩家移动
    QTimer *followTimer = new QTimer();
    QObject::connect(followTimer, &QTimer::timeout, [playerPtr, scareText]() {
        if (playerPtr && scareText && scareText->scene()) {
            scareText->setPos(playerPtr->pos().x(), playerPtr->pos().y() - 40);
        }
    });
    followTimer->start(16);  // 每16ms更新位置

    // 3秒后恢复正常并删除文字
    QTimer::singleShot(3000, [playerPtr, scareText, followTimer]() {
        // 停止跟随定时器
        if (followTimer) {
            followTimer->stop();
            delete followTimer;
        }

        if (playerPtr) {
            playerPtr->setScared(false);
            qDebug() << "惊吓效果结束，玩家恢复正常，3秒后可再次触发";
            // 效果结束后3秒再解除冷却
            QTimer::singleShot(3000, [playerPtr]() {
                if (playerPtr) {
                    playerPtr->setEffectCooldown(false);
                }
            });
        }
        if (scareText) {
            if (scareText->scene()) {
                scareText->scene()->removeItem(scareText);
            }
            delete scareText;
        }
    });
}
