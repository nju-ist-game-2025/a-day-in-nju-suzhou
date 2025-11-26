#include "sockshooter.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QtMath>
#include "../core/configmanager.h"
#include "player.h"
#include "projectile.h"

SockShooter::SockShooter(const QPixmap &pic, double scale)
    : Enemy(pic, scale),
      m_shootTimer(nullptr),
      m_facingRight(true),
      m_bulletDamage(DEFAULT_BULLET_DAMAGE),
      m_shootCooldown(DEFAULT_SHOOT_COOLDOWN),
      m_bulletSpeed(DEFAULT_BULLET_SPEED),
      m_bulletScale(DEFAULT_BULLET_SCALE)
{
    // 设置基础属性
    setHealth(10);                         // 血量
    setContactDamage(0);                  // 无接触伤害！纯远程敌人
    setVisionRange(DEFAULT_VISION_RANGE); // 较大的视野范围
    setAttackRange(DEFAULT_VISION_RANGE); // 攻击范围等于视野范围（可以远距离射击）
    setAttackCooldown(m_shootCooldown);   // 攻击冷却
    setSpeed(1.0);                        // 移动速度慢，减少抖动

    // 设置碰撞半径（确保可以被玩家子弹击中）
    setCrashR(25);

    // 使用保持距离移动模式（远程敌人专用）
    setMovementPattern(MOVE_KEEP_DISTANCE);
    setPreferredDistance(DEFAULT_KEEP_DISTANCE);

    // 加载子弹图片
    loadBulletPixmap();

    // 创建射击定时器
    m_shootTimer = new QTimer(this);
    connect(m_shootTimer, &QTimer::timeout, this, &SockShooter::shootBullet);
    m_shootTimer->start(m_shootCooldown);

    qDebug() << "SockShooter 创建完成 - 子弹伤害:" << m_bulletDamage
             << " 射击间隔:" << m_shootCooldown << "ms"
             << " 接触伤害:" << contactDamage;
}

SockShooter::~SockShooter()
{
    if (m_shootTimer)
    {
        m_shootTimer->stop();
        delete m_shootTimer;
        m_shootTimer = nullptr;
    }
}

void SockShooter::loadBulletPixmap()
{
    // 从配置文件获取子弹大小
    int bulletSize = ConfigManager::instance().getBulletSize("sock_shooter");
    if (bulletSize <= 0)
        bulletSize = 25; // 默认值

    // 加载子弹图片
    QPixmap originalBullet("assets/items/bullet_sock_shooter.png");

    if (originalBullet.isNull())
    {
        qWarning() << "无法加载 sock_shooter 子弹图片，使用默认黄色子弹";
        // 创建一个默认的黄色子弹
        m_bulletPixmap = QPixmap(bulletSize, bulletSize / 2);
        m_bulletPixmap.fill(Qt::yellow);
    }
    else
    {
        // 缩放子弹图片到配置的大小
        m_bulletPixmap = originalBullet.scaled(bulletSize, bulletSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        qDebug() << "SockShooter 子弹图片加载成功，大小:" << m_bulletPixmap.size();
    }
}

void SockShooter::pauseTimers()
{
    Enemy::pauseTimers();

    if (m_shootTimer && m_shootTimer->isActive())
    {
        m_shootTimer->stop();
    }
}

void SockShooter::resumeTimers()
{
    Enemy::resumeTimers();

    if (m_shootTimer && !m_shootTimer->isActive())
    {
        m_shootTimer->start(m_shootCooldown);
    }
}

void SockShooter::updateFacingDirection()
{
    if (!player)
        return;

    // 根据玩家位置更新面朝方向
    double dx = player->pos().x() - pos().x();
    bool newFacingRight = (dx >= 0);

    // 更新面朝方向
    m_facingRight = newFacingRight;

    // 设置 xdir 让 Entity 基类的 updateFacing() 处理图片翻转
    // 玩家在右边时 xdir > 0，在左边时 xdir < 0
    if (dx > 0)
    {
        xdir = 1; // 触发向右朝向
    }
    else if (dx < 0)
    {
        xdir = -1; // 触发向左朝向
    }
}

void SockShooter::attackPlayer()
{
    // SockShooter 是纯远程敌人，不进行任何近战攻击
    // 完全重写基类方法，不调用基类实现
    // 射击由独立的 shootBullet() 定时器处理
}

void SockShooter::shootBullet()
{
    // 暂停状态下不射击
    if (m_isPaused)
        return;

    // 检查是否在场景中
    if (!scene())
    {
        qDebug() << "SockShooter::shootBullet - 不在场景中";
        return;
    }

    // 检查是否能看到玩家（在视野范围内）
    if (!player)
    {
        qDebug() << "SockShooter::shootBullet - 没有玩家引用";
        return;
    }

    double dist = distanceToPlayer();
    if (dist > visionRange)
        return;

    // 更新面朝方向（会同时更新 xdir 触发图片翻转）
    updateFacingDirection();

    // 计算子弹发射位置（从敌人中心位置发射）
    QRectF rect = boundingRect();
    QPointF center = pos() + QPointF(rect.width() / 2.0, rect.height() / 2.0);

    // 创建子弹（mode=1 表示敌人子弹，会伤害玩家）
    // 不再额外缩放，因为 loadBulletPixmap 已经缩放好了
    Projectile *bullet = new Projectile(1, m_bulletDamage, center,
                                        m_bulletPixmap, 1.0);

    // 设置子弹方向（只向水平方向发射）
    // 面朝方向决定子弹方向：右=正X，左=负X
    int bulletDirX = m_facingRight ? static_cast<int>(m_bulletSpeed) : -static_cast<int>(m_bulletSpeed);
    int bulletDirY = 0; // Y方向始终为0，只水平发射

    bullet->setDir(bulletDirX, bulletDirY);

    // 设置子弹 Z 值确保可见
    bullet->setZValue(100);

    // 添加到场景
    scene()->addItem(bullet);

    qDebug() << "SockShooter 发射子弹 - 方向:" << (m_facingRight ? "右" : "左")
             << "位置:" << center << "玩家距离:" << dist
             << "子弹dirX:" << bulletDirX;
}
