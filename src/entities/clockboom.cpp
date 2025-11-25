#include "clockboom.h"
#include "player.h"
#include "../ui/explosion.h"
#include "../core/audiomanager.h"
#include <QGraphicsScene>
#include <QPainter>
#include <QDebug>
#include <QtMath>

ClockBoom::ClockBoom(const QPixmap &normalPic, const QPixmap &redPic, double scale)
    : Enemy(normalPic, scale), m_triggered(false), m_exploded(false), m_normalPixmap(normalPic.scaled(normalPic.width() * scale, normalPic.height() * scale,
                                                                                                      Qt::KeepAspectRatio, Qt::SmoothTransformation)),
      m_redPixmap(redPic.scaled(redPic.width() * scale, redPic.height() * scale,
                                Qt::KeepAspectRatio, Qt::SmoothTransformation)),
      m_isRed(false)
{
    // ClockBoom的特殊属性
    setHealth(5);        // 5点血，可以被攻击摧毁
    setContactDamage(0); // 碰撞不造成伤害
    setVisionRange(0);   // 无视野
    setAttackRange(0);   // 无攻击范围
    setSpeed(0);         // 不移动

    // 停止所有继承的定时器（因为不需要移动、AI和攻击检测）
    if (aiTimer)
    {
        aiTimer->stop();
    }
    if (moveTimer)
    {
        moveTimer->stop();
    }
    if (attackTimer)
    {
        attackTimer->stop();
    }

    // 创建独立的碰撞检测定时器
    m_collisionTimer = new QTimer(this);
    connect(m_collisionTimer, &QTimer::timeout, this, &ClockBoom::onCollisionCheck);
    m_collisionTimer->start(50); // 每50ms检测一次碰撞

    // 创建闪烁定时器（倒计时闪烁）
    m_blinkTimer = new QTimer(this);
    connect(m_blinkTimer, &QTimer::timeout, this, &ClockBoom::onBlinkTimeout);

    // 创建爆炸定时器
    m_explodeTimer = new QTimer(this);
    m_explodeTimer->setSingleShot(true);
    connect(m_explodeTimer, &QTimer::timeout, this, &ClockBoom::onExplodeTimeout);

    // 预创建红色闪烁效果图（类似Entity的flash效果）
    m_redPixmap = m_normalPixmap;
    QPainter painter(&m_redPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(m_redPixmap.rect(), QColor(255, 0, 0, 180));
    painter.end();

    // 禁用与其他物体的碰撞检测，完全避免基类碰撞机制
    setFlag(QGraphicsItem::ItemIsSelectable, false);

    qDebug() << "ClockBoom创建完成";
}

ClockBoom::~ClockBoom()
{
    if (m_collisionTimer)
    {
        m_collisionTimer->stop();
        delete m_collisionTimer;
        m_collisionTimer = nullptr;
    }
    if (m_blinkTimer)
    {
        m_blinkTimer->stop();
        delete m_blinkTimer;
        m_blinkTimer = nullptr;
    }
    if (m_explodeTimer)
    {
        m_explodeTimer->stop();
        delete m_explodeTimer;
        m_explodeTimer = nullptr;
    }
}

void ClockBoom::move()
{
    // ClockBoom不移动
    return;
}

void ClockBoom::onCollisionCheck()
{
    checkCollisionWithPlayer();
}

void ClockBoom::checkCollisionWithPlayer()
{
    if (m_triggered || m_exploded)
        return;

    if (!player)
        return;

    // 检测与玩家的碰撞
    QList<QGraphicsItem *> collisions = collidingItems();
    for (QGraphicsItem *item : collisions)
    {
        Player *p = dynamic_cast<Player *>(item);
        if (p)
        {
            // 首次碰撞，触发倒计时
            qDebug() << "ClockBoom被玩家触发！开始倒计时2.5秒";
            startCountdown();
            break;
        }
    }
}

void ClockBoom::attackPlayer()
{
    // ClockBoom不使用常规攻击系统，使用独立的碰撞检测
    // 此方法为空实现，避免基类调用
    return;
}

void ClockBoom::startCountdown()
{
    m_triggered = true;

    // 停止碰撞检测定时器（已触发，不需要再检测）
    if (m_collisionTimer)
    {
        m_collisionTimer->stop();
    }

    // 开始闪烁（每0.5秒切换一次）
    m_blinkTimer->start(500);

    // 2.5秒后爆炸
    m_explodeTimer->start(2500);
}

void ClockBoom::onBlinkTimeout()
{
    toggleBlink();
}

void ClockBoom::toggleBlink()
{
    m_isRed = !m_isRed;
    if (m_isRed)
    {
        // 显示红色闪烁版本（类似受击效果）
        QGraphicsPixmapItem::setPixmap(m_redPixmap);
    }
    else
    {
        // 显示普通版本
        QGraphicsPixmapItem::setPixmap(m_normalPixmap);
    }
    qDebug() << "ClockBoom闪烁切换，当前状态:" << (m_isRed ? "红色" : "普通");
}

void ClockBoom::onExplodeTimeout()
{
    explode();
}

void ClockBoom::explode()
{
    if (m_exploded)
        return;

    m_exploded = true;

    // 停止闪烁
    if (m_blinkTimer)
    {
        m_blinkTimer->stop();
    }

    qDebug() << "ClockBoom爆炸！";

    // 播放爆炸音效
    AudioManager::instance().playSound("enemy_death");

    // 创建爆炸动画
    if (scene())
    {
        Explosion *explosion = new Explosion();
        explosion->setPos(this->pos());
        scene()->addItem(explosion);
        explosion->startAnimation();
    }

    // 对范围内的实体造成伤害
    damageNearbyEntities();

    // 发出dying信号并删除自己
    emit dying(this);

    if (scene())
    {
        scene()->removeItem(this);
    }

    deleteLater();
}

void ClockBoom::damageNearbyEntities()
{
    if (!scene())
        return;

    const double explosionRadius = 500.0; // 爆炸范围（增加到两倍）
    QPointF bombPos = pos();

    // 获取场景中的所有物品
    QList<QGraphicsItem *> allItems = scene()->items();

    for (QGraphicsItem *item : allItems)
    {
        // 检查玩家
        Player *p = dynamic_cast<Player *>(item);
        if (p)
        {
            double distance = qSqrt(qPow(p->pos().x() - bombPos.x(), 2) +
                                    qPow(p->pos().y() - bombPos.y(), 2));
            if (distance <= explosionRadius)
            {
                p->forceTakeDamage(2); // 爆炸伤害无视无敌状态
            }
            continue;
        }

        // 检查其他敌人
        Enemy *enemy = dynamic_cast<Enemy *>(item);
        if (enemy && enemy != this)
        {
            double distance = qSqrt(qPow(enemy->pos().x() - bombPos.x(), 2) +
                                    qPow(enemy->pos().y() - bombPos.y(), 2));
            if (distance <= explosionRadius)
            {
                qDebug() << "爆炸对敌人造成3点伤害";
                enemy->takeDamage(3);
            }
        }
    }
}
