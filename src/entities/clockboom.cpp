#include "clockboom.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include "../core/audiomanager.h"
#include "../ui/explosion.h"
#include "player.h"
#include "nightmareboss.h"

ClockBoom::ClockBoom(const QPixmap &normalPic, const QPixmap &redPic, double scale)
    : Enemy(normalPic, scale), m_triggered(false), m_exploded(false), m_normalPixmap(normalPic.scaled(normalPic.width() * scale, normalPic.height() * scale, Qt::KeepAspectRatio, Qt::SmoothTransformation)), m_redPixmap(redPic.scaled(redPic.width() * scale, redPic.height() * scale, Qt::KeepAspectRatio, Qt::SmoothTransformation)), m_isRed(false)
{
    // ClockBoom的特殊属性
    setHealth(6);        // 6点血，可以被攻击摧毁
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

void ClockBoom::takeDamage(int damage) {
    // ClockBoom被攻击时的特殊处理
    // 避免走父类Enemy::takeDamage的死亡流程（会重复创建爆炸效果）

    if (m_exploded)
        return;  // 已经爆炸了，不再处理

    flash();  // 显示受击闪烁
    health -= qMax(1, damage);

    if (health <= 0) {
        // ClockBoom被打死时，显示死亡特效但不造成范围伤害
        // dealDamage=false 表示只是被动死亡，不触发爆炸伤害
        explode(false);
    }
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

    // 暂停状态下不检测碰撞
    if (m_isPaused)
        return;

    // 检测与玩家的碰撞
    QList<QGraphicsItem *> collisions = collidingItems();
    for (QGraphicsItem *item : collisions)
    {
        Player *p = dynamic_cast<Player *>(item);
        if (p)
        {
            // 首次碰撞，触发倒计时
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

void ClockBoom::triggerCountdown()
{
    if (!m_triggered)
    {
        startCountdown();
    }
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
}

void ClockBoom::onExplodeTimeout()
{
    explode();
}

void ClockBoom::explode(bool dealDamage) {
    if (m_exploded)
        return;

    m_exploded = true;

    // 停止闪烁
    if (m_blinkTimer)
    {
        m_blinkTimer->stop();
    }

    // 只有主动爆炸（倒计时结束）才对范围内实体造成伤害
    // 被打死时不触发范围伤害，避免连锁反应导致卡顿
    if (dealDamage) {
        damageNearbyEntities();
    }

    // 播放爆炸音效（不阻塞）
    AudioManager::instance().playSound("enemy_death");

    // 创建爆炸动画 - 立即创建以确保显示
    if (scene()) {
        Explosion* explosion = new Explosion();
        explosion->setPos(this->pos());
        scene()->addItem(explosion);
        explosion->startAnimation();
    }

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

    const double explosionRadius = 150.0; // 爆炸范围
    QPointF bombPos = pos();
    const double radiusSquared = explosionRadius * explosionRadius;

    // 使用空间查询代替遍历所有物品
    QRectF searchRect(bombPos.x() - explosionRadius, bombPos.y() - explosionRadius,
                      explosionRadius * 2, explosionRadius * 2);
    QList<QGraphicsItem *> nearbyItems = scene()->items(searchRect);

    // 性能优化：先进行一次类型筛选，减少重复的dynamic_cast
    Player *targetPlayer = nullptr;
    QVector<Enemy *> targetEnemies;
    targetEnemies.reserve(nearbyItems.size() / 2); // 预分配避免频繁扩容

    for (QGraphicsItem *item : nearbyItems)
    {
        // 跳过自己
        if (item == this)
            continue;

        // 先尝试转换为Enemy（更常见），如果失败再尝试Player
        if (Enemy *enemy = dynamic_cast<Enemy *>(item))
        {
            targetEnemies.append(enemy);
        }
        else if (!targetPlayer)
        { // 只需要找到一次玩家
            targetPlayer = dynamic_cast<Player *>(item);
        }
    }

    // 对筛选后的目标进行距离检查和伤害
    if (targetPlayer)
    {
        QPointF itemPos = targetPlayer->pos();
        double dx = itemPos.x() - bombPos.x();
        double dy = itemPos.y() - bombPos.y();
        double distanceSquared = dx * dx + dy * dy;

        if (distanceSquared <= radiusSquared)
        {
            targetPlayer->forceTakeDamage(2);
        }
    }

    for (Enemy *enemy : targetEnemies)
    {
        QPointF itemPos = enemy->pos();
        double dx = itemPos.x() - bombPos.x();
        double dy = itemPos.y() - bombPos.y();
        double distanceSquared = dx * dx + dy * dy;

        if (distanceSquared <= radiusSquared)
        {
            // 对NightmareBoss造成20点伤害的特判
            if (dynamic_cast<NightmareBoss *>(enemy))
            {
                enemy->takeDamage(50);
                qDebug() << "ClockBoom对梦魇Boss造成50点爆炸伤害";
            }
            else
            {
                enemy->takeDamage(3);
            }
        }
    }
}

void ClockBoom::pauseTimers()
{
    // 调用父类的暂停方法
    Enemy::pauseTimers();

    // 暂停ClockBoom特有的定时器
    if (m_collisionTimer && m_collisionTimer->isActive())
    {
        m_collisionTimer->stop();
    }
    if (m_blinkTimer && m_blinkTimer->isActive())
    {
        m_blinkTimer->stop();
    }
    if (m_explodeTimer && m_explodeTimer->isActive())
    {
        m_explodeTimer->stop();
    }
}

void ClockBoom::resumeTimers()
{
    // 调用父类的恢复方法
    Enemy::resumeTimers();

    // 恢复ClockBoom特有的定时器
    if (!m_triggered && m_collisionTimer)
    {
        // 如果还没触发倒计时，恢复碰撞检测
        m_collisionTimer->start(50);
    }
    else if (m_triggered && !m_exploded)
    {
        // 如果已触发但还没爆炸，恢复闪烁和爆炸定时器
        if (m_blinkTimer)
        {
            m_blinkTimer->start(500);
        }
        // 爆炸定时器是一次性的，需要计算剩余时间
        // 这里简单处理：继续以原始2.5秒启动（这可能不完美但可以接受）
        if (m_explodeTimer && m_explodeTimer->remainingTime() > 0)
        {
            // 如果还有剩余时间，让它继续
            // Qt的定时器在stop后remainingTime会返回-1
        }
        else if (m_explodeTimer)
        {
            // 如果定时器停止了但还没爆炸，继续倒计时
            // 这是一个近似处理
            m_explodeTimer->start(500); // 给一个短时间爆炸
        }
    }
}
