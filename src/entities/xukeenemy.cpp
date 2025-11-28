#include "xukeenemy.h"
#include <QDebug>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QtMath>
#include "../core/configmanager.h"
#include "player.h"

// ==================== XukeEnemy 实现 ====================

XukeEnemy::XukeEnemy(const QPixmap& pic, double scale)
    : Enemy(pic, scale),
      m_shootTimer(nullptr),
      m_shotCount(0),
      m_facingRight(true) {
    // 从配置文件读取徐轲属性
    ConfigManager& config = ConfigManager::instance();
    setHealth(config.getEnemyInt("xuke", "health", 15));
    setContactDamage(0);     // 纯远程敌人，无接触伤害！
    setVisionRange(9999.0);  // 全图视野！
    setAttackRange(9999.0);  // 全图攻击范围
    setAttackCooldown(config.getEnemyInt("xuke", "shoot_cooldown", SHOOT_COOLDOWN));
    setSpeed(config.getEnemyDouble("xuke", "speed", 1.0));

    // 设置碰撞半径
    setCrashR(25);

    // 使用保持距离移动模式（远程敌人专用）
    setMovementPattern(MOVE_KEEP_DISTANCE);
    setPreferredDistance(config.getEnemyDouble("xuke", "keep_distance", KEEP_DISTANCE));

    // 加载子弹图片
    loadBulletPixmaps();

    // 创建射击定时器
    m_shootTimer = new QTimer(this);
    connect(m_shootTimer, &QTimer::timeout, this, &XukeEnemy::shootBullet);
    m_shootTimer->start(SHOOT_COOLDOWN);

    qDebug() << "XukeEnemy 创建完成 - 射击间隔:" << SHOOT_COOLDOWN << "ms"
             << "接触伤害:" << contactDamage << "移动模式:MOVE_KEEP_DISTANCE";
}

XukeEnemy::~XukeEnemy() {
    if (m_shootTimer) {
        m_shootTimer->stop();
        delete m_shootTimer;
        m_shootTimer = nullptr;
    }
}

void XukeEnemy::loadBulletPixmaps() {
    // 从配置文件读取子弹尺寸
    int normalBulletSize = ConfigManager::instance().getBulletSize("xuke");
    int specialBulletSize = ConfigManager::instance().getBulletSize("xuke_special");

    if (normalBulletSize <= 0)
        normalBulletSize = 15;  // 默认值
    if (specialBulletSize <= 0)
        specialBulletSize = 20;  // 默认值

    // 加载普通子弹图片
    QPixmap bullet1("assets/items/bullet_xuke1.png");
    if (bullet1.isNull()) {
        qWarning() << "无法加载 bullet_xuke1.png，使用默认子弹";
        m_bulletPixmap1 = QPixmap(normalBulletSize, normalBulletSize);
        m_bulletPixmap1.fill(Qt::yellow);
    } else {
        m_bulletPixmap1 = bullet1.scaled(normalBulletSize, normalBulletSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        qDebug() << "XukeEnemy 普通子弹图片加载成功，尺寸:" << normalBulletSize;
    }

    // 加载强化子弹图片
    QPixmap bullet2("assets/items/bullet_xuke2.png");
    if (bullet2.isNull()) {
        qWarning() << "无法加载 bullet_xuke2.png，使用默认强化子弹";
        m_bulletPixmap2 = QPixmap(specialBulletSize, specialBulletSize);
        m_bulletPixmap2.fill(Qt::red);
    } else {
        m_bulletPixmap2 = bullet2.scaled(specialBulletSize, specialBulletSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        qDebug() << "XukeEnemy 强化子弹图片加载成功，尺寸:" << specialBulletSize;
    }
}

void XukeEnemy::pauseTimers() {
    Enemy::pauseTimers();

    if (m_shootTimer && m_shootTimer->isActive()) {
        m_shootTimer->stop();
    }
}

void XukeEnemy::resumeTimers() {
    Enemy::resumeTimers();

    if (m_shootTimer && !m_shootTimer->isActive()) {
        m_shootTimer->start(SHOOT_COOLDOWN);
    }
}

void XukeEnemy::updateFacingDirection() {
    if (!player)
        return;

    // 根据玩家位置更新面朝方向
    double dx = player->pos().x() - pos().x();
    m_facingRight = (dx >= 0);

    // 设置 xdir 让 Entity 基类的 updateFacing() 处理图片翻转
    if (dx > 0) {
        xdir = 1;
    } else if (dx < 0) {
        xdir = -1;
    }
}

void XukeEnemy::attackPlayer() {
    // 徐科是纯远程敌人，不进行任何近战攻击
    // 完全重写基类方法，不调用基类实现
    // 射击由独立的 shootBullet() 定时器处理
}

void XukeEnemy::shootBullet() {
    // 暂停状态下不射击
    if (m_isPaused)
        return;

    // 检查是否在场景中
    if (!scene()) {
        qDebug() << "XukeEnemy::shootBullet - 不在场景中";
        return;
    }

    // 检查玩家引用
    if (!player) {
        qDebug() << "XukeEnemy::shootBullet - 没有玩家引用";
        return;
    }

    double dist = distanceToPlayer();
    if (dist > visionRange)
        return;

    // 更新面朝方向
    updateFacingDirection();

    // 增加射击计数
    m_shotCount++;

    // 判断是否是第6发（强化子弹）
    bool isSpecialShot = (m_shotCount % 6 == 0);

    // 计算子弹发射位置（从敌人中心发射）
    QRectF rect = boundingRect();
    QPointF center = pos() + QPointF(rect.width() / 2.0, rect.height() / 2.0);

    // 锁定玩家当前位置
    QRectF playerRect = player->boundingRect();
    QPointF playerCenter = player->pos() + QPointF(playerRect.width() / 2.0, playerRect.height() / 2.0);

    // 计算指向玩家的方向向量
    double dx = playerCenter.x() - center.x();
    double dy = playerCenter.y() - center.y();
    double distance = qSqrt(dx * dx + dy * dy);

    if (distance < 1.0)
        return;  // 距离太近，不发射

    // 归一化方向并乘以速度
    int dirX = static_cast<int>((dx / distance) * BULLET_SPEED);
    int dirY = static_cast<int>((dy / distance) * BULLET_SPEED);

    // 选择子弹类型和图片
    XukeProjectile::BulletType bulletType = isSpecialShot ? XukeProjectile::SPECIAL : XukeProjectile::NORMAL;
    int baseDamage = isSpecialShot ? SPECIAL_DAMAGE : NORMAL_DAMAGE;
    const QPixmap& bulletPic = isSpecialShot ? m_bulletPixmap2 : m_bulletPixmap1;

    // 创建子弹（继承自Projectile，mode=1表示敌人子弹）
    XukeProjectile* bullet = new XukeProjectile(bulletType, baseDamage, center, bulletPic, 1.0, player);
    bullet->setDir(dirX, dirY);
    bullet->setZValue(100);

    scene()->addItem(bullet);

    qDebug() << "XukeEnemy 发射子弹 - 类型:" << (isSpecialShot ? "强化" : "普通")
             << "计数:" << m_shotCount << "方向:(" << dirX << "," << dirY << ")"
             << "玩家距离:" << dist;
}

// ==================== XukeProjectile 实现 ====================

XukeProjectile::XukeProjectile(BulletType type, int baseDamage, QPointF pos, const QPixmap& pic, double scale, Player* targetPlayer)
    : Projectile(1, baseDamage, pos, pic, scale),  // mode=1 表示敌人子弹
      m_bulletType(type),
      m_targetPlayer(targetPlayer),
      m_hasHit(false) {
    qDebug() << "XukeProjectile 创建 - 类型:" << (type == SPECIAL ? "强化" : "普通")
             << "位置:" << pos << "伤害:" << baseDamage;
}

XukeProjectile::~XukeProjectile() {
    // 父类Projectile的析构函数会处理定时器的清理
}

void XukeProjectile::move() {
    // 如果已经命中，不再处理
    if (m_hasHit)
        return;

    // 检查场景
    if (!scene())
        return;

    // 先检测碰撞（在父类move之前，这样可以应用爆头逻辑）
    if (m_targetPlayer && !m_hasHit) {
        QList<QGraphicsItem*> collisions = collidingItems();
        for (QGraphicsItem* item : collisions) {
            Player* player = dynamic_cast<Player*>(item);
            if (player && player == m_targetPlayer) {
                // 使用像素级碰撞检测
                if (Entity::pixelCollision(this, player)) {
                    // 应用爆头伤害逻辑
                    checkHeadshotAndApplyDamage(player);
                    // 销毁子弹
                    destroy();
                    return;
                }
            }
        }
    }

    // 调用父类的move方法处理移动和边界检测
    Projectile::move();
}

void XukeProjectile::checkHeadshotAndApplyDamage(Player* player) {
    if (!player || m_hasHit)
        return;

    m_hasHit = true;

    // 获取玩家的碰撞区域
    QRectF playerRect = player->boundingRect();
    double playerTop = player->pos().y();
    double playerHeight = playerRect.height();

    // 获取子弹命中位置（子弹中心）
    QRectF bulletRect = boundingRect();
    double bulletCenterY = pos().y() + bulletRect.height() / 2.0;

    // 判断是否爆头（命中玩家y轴上20%区域）
    double headshotThreshold = playerTop + playerHeight * 0.2;
    bool isHeadshot = (bulletCenterY <= headshotThreshold);

    int damage = 0;
    QString headshotText;

    if (m_bulletType == SPECIAL) {
        // 强化子弹
        if (isHeadshot) {
            damage = XukeEnemy::SPECIAL_HEADSHOT_DAMAGE;  // 8点伤害
            headshotText = "颗秒！！！";
        } else {
            damage = XukeEnemy::SPECIAL_DAMAGE;  // 2点伤害
        }
    } else {
        // 普通子弹
        if (isHeadshot) {
            damage = XukeEnemy::NORMAL_HEADSHOT_DAMAGE;  // 3点伤害
            headshotText = "颗秒！";
        } else {
            damage = XukeEnemy::NORMAL_DAMAGE;  // 1点伤害
        }
    }

    // 造成伤害
    player->takeDamage(damage);

    // 如果爆头，显示文字
    if (isHeadshot && !headshotText.isEmpty()) {
        showHeadshotText(headshotText);
    }

    qDebug() << "XukeProjectile 命中玩家 - 类型:" << (m_bulletType == SPECIAL ? "强化" : "普通")
             << "爆头:" << isHeadshot << "伤害:" << damage;
}

void XukeProjectile::showHeadshotText(const QString& text) {
    if (!scene() || !m_targetPlayer)
        return;

    QGraphicsTextItem* textItem = new QGraphicsTextItem(text);
    QFont font;
    font.setPointSize(18);
    font.setBold(true);
    textItem->setFont(font);
    textItem->setDefaultTextColor(QColor(139, 69, 19));  // 棕色
    textItem->setPos(m_targetPlayer->pos().x(), m_targetPlayer->pos().y() - 50);
    textItem->setZValue(300);
    scene()->addItem(textItem);

    // 1秒后删除文字
    QTimer::singleShot(1000, textItem, [textItem]() {
        if (textItem && textItem->scene())
        {
            textItem->scene()->removeItem(textItem);
        }
        delete textItem; });
}
