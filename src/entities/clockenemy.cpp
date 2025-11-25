#include "clockenemy.h"
#include <QDebug>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QRandomGenerator>
#include <QTimer>
#include <QPointer>
#include "player.h"

ClockEnemy::ClockEnemy(const QPixmap &pic, double scale)
    : Enemy(pic, scale)
{
    // 时钟怪物的基础属性（与普通敌人相同）
    setHealth(10);
    setContactDamage(1);
    setVisionRange(250.0);
    setAttackRange(40.0);
    setAttackCooldown(1000);
    setSpeed(2.0);

    // 使用Z字形移动模式，增加躲避难度
    setMovementPattern(MOVE_ZIGZAG);
    setZigzagAmplitude(60.0);
}

void ClockEnemy::attackPlayer()
{
    if (!player)
        return;

    // 近战攻击：检测碰撞
    QList<QGraphicsItem *> collisions = collidingItems();
    for (QGraphicsItem *item : collisions)
    {
        Player *p = dynamic_cast<Player *>(item);
        if (p)
        {
            p->takeDamage(contactDamage);

            // 50%概率触发昏睡效果
            if (QRandomGenerator::global()->bounded(2) == 0)
            {
                applySleepEffect();
            }

            break;
        }
    }
}

void ClockEnemy::applySleepEffect()
{
    if (!player || !scene())
        return;

    qDebug() << "时钟怪物触发昏睡效果！玩家无法移动1秒";

    // 显示"昏睡ZZZ"文字提示
    QGraphicsTextItem *sleepText = new QGraphicsTextItem("昏睡ZZZ");
    QFont font;
    font.setPointSize(16);
    font.setBold(true);
    sleepText->setFont(font);
    sleepText->setDefaultTextColor(QColor(128, 128, 128)); // 灰色
    sleepText->setPos(player->pos().x(), player->pos().y() - 40);
    sleepText->setZValue(200);
    scene()->addItem(sleepText);

    // 禁用玩家移动
    player->setCanMove(false);

    // 使用QPointer保护player指针，确保即使ClockEnemy被删除也能恢复玩家移动
    QPointer<Player> playerPtr = player;

    // 1秒后恢复移动并删除文字（不再依赖this指针）
    QTimer::singleShot(1000, [playerPtr, sleepText]()
                       {
        if (playerPtr) {
            playerPtr->setCanMove(true);
            qDebug() << "昏睡效果结束，玩家恢复移动";
        }
        if (sleepText) {
            if (sleepText->scene()) {
                sleepText->scene()->removeItem(sleepText);
            }
            delete sleepText;
        } });
}
