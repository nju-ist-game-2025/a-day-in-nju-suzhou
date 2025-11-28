#include "projectile.h"
#include <QDebug>
#include <QPointer>
#include <QRandomGenerator>
#include "../items/itemeffectconfig.h"
#include "enemy.h"
#include "player.h"
#include "probabilityenemy.h"
#include "statuseffect.h"

Projectile::Projectile(int _mode, double _hurt, QPointF pos, const QPixmap& pic_bullet, double scale)
    : mode(_mode), isDestroying(false), m_isPaused(false), m_isFrostBullet(false) {
    setTransformationMode(Qt::SmoothTransformation);

    // 禁用缓存以避免留下轨迹
    setCacheMode(QGraphicsItem::NoCache);

    xdir = 0;
    ydir = 0;
    speed = 1.0;
    hurt = _hurt;

    // 检查图片是否有效
    if (pic_bullet.isNull()) {
        qWarning() << "Projectile: pic_bullet is null, using default";
        QPixmap defaultBullet(10, 10);
        defaultBullet.fill(Qt::yellow);
        this->setPixmap(defaultBullet);
    } else if (scale == 1.0) {
        this->setPixmap(pic_bullet);
    } else {
        // 按比例缩放（保持宽高比）
        this->setPixmap(pic_bullet.scaled(
            pic_bullet.width() * scale,
            pic_bullet.height() * scale,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    }

    this->setPos(pos);

    // 预加载碰撞掩码（子弹图片通常很小，生成开销低）
    preloadCollisionMask();

    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, &Projectile::move);
    moveTimer->start(16);

    crashTimer = new QTimer(this);
    connect(crashTimer, &QTimer::timeout, this, &Projectile::checkCrash);
    crashTimer->start(50);
}

Projectile::~Projectile() {
    // 析构函数只负责清理资源
    // 定时器作为子对象会自动删除，但为了确保立即停止，手动处理
    if (moveTimer) {
        moveTimer->stop();
    }
    if (crashTimer) {
        crashTimer->stop();
    }
}

void Projectile::move() {
    // 如果正在销毁或暂停，不再执行任何操作
    if (isDestroying || m_isPaused) {
        return;
    }

    // 检查场景是否有效
    if (!scene()) {
        destroy();
        return;
    }

    // 检查是否超出边界
    double newX = pos().x() + xdir;
    double newY = pos().y() + ydir;

    if (newX < 0 || newX > scene_bound_x || newY < 0 || newY > scene_bound_y) {
        destroy();
        return;
    } else {
        QPointF dir(xdir, ydir);
        this->setPos(pos() + speed * dir);
    }
}

// 冰霜子弹减速效果：从配置文件读取参数
void applyFrostEffect(Enemy* enemy) {
    if (!enemy || !enemy->scene())
        return;

    // 从配置文件读取寒冰效果参数
    ItemEffectData frostConfig = ItemEffectConfig::instance().getItemEffect("frost_slowdown");
    int maxStacks = frostConfig.getMaxSlowStacks();
    double slowDuration = frostConfig.getSlowDuration();
    double slowFactor = frostConfig.getSlowFactor();

    // 检查是否已达到最大叠加层数
    if (!enemy->canApplySlowStack(maxStacks)) {
        qDebug() << "Frost effect: max slow stacks reached, skipping";
        return;
    }

    // 增加减速层数
    enemy->addSlowStack();

    SpeedEffect* frostSlow = new SpeedEffect(slowDuration, slowFactor);

    // 连接效果结束信号，减少叠加层数
    QPointer<Enemy> enemyPtr = enemy;
    QObject::connect(frostSlow, &QObject::destroyed, [enemyPtr]() {
        if (enemyPtr) {
            enemyPtr->removeSlowStack();
        }
    });

    frostSlow->applyTo(enemy);
}

void Projectile::checkCrash() {
    // 如果正在销毁或暂停，不再执行任何操作
    if (isDestroying || m_isPaused) {
        return;
    }

    // 确保场景和定时器有效
    if (!scene() || !moveTimer || !crashTimer)
        return;

    // 使用collidingItems代替遍历整个scene，大幅提升性能
    QList<QGraphicsItem*> collisions = collidingItems();

    // 如果没有碰撞，直接返回
    if (collisions.isEmpty())
        return;

    if (mode) {
        // 敌人子弹，检测玩家碰撞
        for (QGraphicsItem* item : collisions) {
            if (auto player = dynamic_cast<Player*>(item)) {
                // 使用像素级碰撞检测
                if (Entity::pixelCollision(this, player)) {
                    player->takeDamage(hurt);
                    destroy();
                    return;
                }
            }
        }
    } else {
        // 玩家子弹，检测敌人碰撞
        for (QGraphicsItem* item : collisions) {
            if (auto enemy = dynamic_cast<Enemy*>(item)) {
                // 特殊处理：如果是ProbabilityEnemy且有其他敌人与之接触，则跳过
                if (auto probEnemy = dynamic_cast<ProbabilityEnemy*>(enemy)) {
                    if (probEnemy->hasContactingEnemies()) {
                        continue;  // 跳过概率论，让子弹继续检测其他敌人
                    }
                }

                // 使用像素级碰撞检测
                if (Entity::pixelCollision(this, enemy)) {
                    // 如果是冰霜子弹，先应用减速效果
                    if (m_isFrostBullet && enemy && enemy->scene()) {
                        applyFrostEffect(enemy);
                    }

                    // 造成伤害
                    if (enemy && enemy->scene()) {
                        enemy->takeDamage(static_cast<int>(hurt));
                    }
                    destroy();
                    return;
                }
            }
        }
    }
}

void Projectile::destroy() {
    // 防止重复调用
    if (isDestroying) {
        return;
    }

    isDestroying = true;

    // 停止所有定时器
    if (moveTimer) {
        moveTimer->stop();
    }
    if (crashTimer) {
        crashTimer->stop();
    }

    // 从场景中移除
    if (scene()) {
        scene()->removeItem(this);
    }

    // 这是唯一调用deleteLater的地方
    deleteLater();
}

void Projectile::setPaused(bool paused) {
    m_isPaused = paused;
}
