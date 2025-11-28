#include "zhuhaoenemy.h"
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QGraphicsScene>
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../ui/explosion.h"
#include "player.h"

// ==================== ZhuhaoEnemy 实现 ====================

ZhuhaoEnemy::ZhuhaoEnemy(const QPixmap& pic, double scale)
    : Enemy(pic, scale),
      m_moveTimer(nullptr),
      m_edgeSpeed(EDGE_MOVE_SPEED),
      m_movingClockwise(true),
      m_currentEdge(0),
      m_shootTimer(nullptr),
      m_bulletCount(BULLETS_PER_WAVE),
      m_bulletSpeed(DEFAULT_BULLET_SPEED) {
    // 从配置文件读取祝昊属性
    ConfigManager& config = ConfigManager::instance();
    health = config.getEnemyInt("zhuhao", "health", 150);
    maxHealth = health;
    contactDamage = config.getEnemyInt("zhuhao", "contact_damage", 3);
    visionRange = 9999.0;  // 全图视野
    attackRange = 9999.0;  // 全图攻击范围
    speed = 0;             // 不使用普通移动
    m_edgeSpeed = config.getEnemyDouble("zhuhao", "edge_move_speed", EDGE_MOVE_SPEED);
    m_bulletSpeed = config.getEnemyDouble("zhuhao", "bullet_speed", DEFAULT_BULLET_SPEED);
    m_bulletCount = config.getEnemyInt("zhuhao", "bullets_per_wave", BULLETS_PER_WAVE);

    // 设置移动模式为静止（我们自己控制移动）
    setMovementPattern(MOVE_NONE);

    // 注意：不在构造函数中调用initializeAtRandomEdge()
    // Level会在添加到场景后调用此方法

    // 创建移动定时器
    m_moveTimer = new QTimer(this);
    connect(m_moveTimer, &QTimer::timeout, this, &ZhuhaoEnemy::onMoveTimer);
    m_moveTimer->start(16);  // 约60fps

    // 创建射击定时器
    m_shootTimer = new QTimer(this);
    connect(m_shootTimer, &QTimer::timeout, this, &ZhuhaoEnemy::onShootTimer);
    m_shootTimer->start(SHOOT_COOLDOWN);

    // 随机决定顺时针或逆时针
    m_movingClockwise = QRandomGenerator::global()->bounded(2) == 0;

    qDebug() << "创建祝昊精英怪 - 血量:" << health << "边缘移动速度:" << m_edgeSpeed;
}

ZhuhaoEnemy::~ZhuhaoEnemy() {
    if (m_moveTimer) {
        m_moveTimer->stop();
        delete m_moveTimer;
        m_moveTimer = nullptr;
    }
    if (m_shootTimer) {
        m_shootTimer->stop();
        delete m_shootTimer;
        m_shootTimer = nullptr;
    }
}

void ZhuhaoEnemy::initializeAtRandomEdge() {
    // 随机选择一个边缘: 0=上 1=右 2=下 3=左
    int edge = QRandomGenerator::global()->bounded(4);

    // 获取精灵的宽高偏移量（因为setPos设置的是左上角，需要补偿让中心沿边界移动）
    double halfWidth = boundingRect().width() / 2.0;
    double halfHeight = boundingRect().height() / 2.0;

    // 计算中心坐标
    double centerX, centerY;

    switch (edge) {
        case 0:  // 上边缘
            centerX = QRandomGenerator::global()->bounded(static_cast<int>(MAP_LEFT + 50),
                                                          static_cast<int>(MAP_RIGHT - 50));
            centerY = MAP_TOP;
            m_currentEdge = 0;
            break;
        case 1:  // 右边缘
            centerX = MAP_RIGHT;
            centerY = QRandomGenerator::global()->bounded(static_cast<int>(MAP_TOP + 50),
                                                          static_cast<int>(MAP_BOTTOM - 50));
            m_currentEdge = 1;
            break;
        case 2:  // 下边缘
            centerX = QRandomGenerator::global()->bounded(static_cast<int>(MAP_LEFT + 50),
                                                          static_cast<int>(MAP_RIGHT - 50));
            centerY = MAP_BOTTOM;
            m_currentEdge = 2;
            break;
        case 3:  // 左边缘
        default:
            centerX = MAP_LEFT;
            centerY = QRandomGenerator::global()->bounded(static_cast<int>(MAP_TOP + 50),
                                                          static_cast<int>(MAP_BOTTOM - 50));
            m_currentEdge = 3;
            break;
    }

    // 将中心坐标转换回左上角坐标来设置位置
    setPos(centerX - halfWidth, centerY - halfHeight);
    qDebug() << "祝昊初始化位置(中心):" << centerX << centerY << "当前边:" << m_currentEdge;
}

void ZhuhaoEnemy::onMoveTimer() {
    if (m_isPaused || !scene())
        return;

    moveAlongEdge();
}

void ZhuhaoEnemy::moveAlongEdge() {
    // 获取精灵的宽高偏移量（因为setPos设置的是左上角，需要补偿让中心沿边界移动）
    double halfWidth = boundingRect().width() / 2.0;
    double halfHeight = boundingRect().height() / 2.0;

    // 将左上角坐标转换为中心坐标
    QPointF currentPos = pos();
    double centerX = currentPos.x() + halfWidth;
    double centerY = currentPos.y() + halfHeight;

    double newCenterX = centerX;
    double newCenterY = centerY;

    // 状态机方式：根据当前边移动，到达端点时切换到下一边
    // m_currentEdge: 0=上边(向右) 1=右边(向下) 2=下边(向左) 3=左边(向上) 顺时针
    // 逆时针时方向相反

    if (m_movingClockwise) {
        switch (m_currentEdge) {
            case 0:  // 上边，向右移动
                newCenterX += m_edgeSpeed;
                newCenterY = MAP_TOP;
                if (newCenterX >= MAP_RIGHT) {
                    newCenterX = MAP_RIGHT;
                    m_currentEdge = 1;  // 切换到右边
                }
                break;
            case 1:  // 右边，向下移动
                newCenterY += m_edgeSpeed;
                newCenterX = MAP_RIGHT;
                if (newCenterY >= MAP_BOTTOM) {
                    newCenterY = MAP_BOTTOM;
                    m_currentEdge = 2;  // 切换到下边
                }
                break;
            case 2:  // 下边，向左移动
                newCenterX -= m_edgeSpeed;
                newCenterY = MAP_BOTTOM;
                if (newCenterX <= MAP_LEFT) {
                    newCenterX = MAP_LEFT;
                    m_currentEdge = 3;  // 切换到左边
                }
                break;
            case 3:  // 左边，向上移动
                newCenterY -= m_edgeSpeed;
                newCenterX = MAP_LEFT;
                if (newCenterY <= MAP_TOP) {
                    newCenterY = MAP_TOP;
                    m_currentEdge = 0;  // 切换到上边
                }
                break;
        }
    } else {
        // 逆时针
        switch (m_currentEdge) {
            case 0:  // 上边，向左移动
                newCenterX -= m_edgeSpeed;
                newCenterY = MAP_TOP;
                if (newCenterX <= MAP_LEFT) {
                    newCenterX = MAP_LEFT;
                    m_currentEdge = 3;  // 切换到左边
                }
                break;
            case 3:  // 左边，向下移动
                newCenterY += m_edgeSpeed;
                newCenterX = MAP_LEFT;
                if (newCenterY >= MAP_BOTTOM) {
                    newCenterY = MAP_BOTTOM;
                    m_currentEdge = 2;  // 切换到下边
                }
                break;
            case 2:  // 下边，向右移动
                newCenterX += m_edgeSpeed;
                newCenterY = MAP_BOTTOM;
                if (newCenterX >= MAP_RIGHT) {
                    newCenterX = MAP_RIGHT;
                    m_currentEdge = 1;  // 切换到右边
                }
                break;
            case 1:  // 右边，向上移动
                newCenterY -= m_edgeSpeed;
                newCenterX = MAP_RIGHT;
                if (newCenterY <= MAP_TOP) {
                    newCenterY = MAP_TOP;
                    m_currentEdge = 0;  // 切换到上边
                }
                break;
        }
    }

    // 将中心坐标转换回左上角坐标来设置位置
    setPos(newCenterX - halfWidth, newCenterY - halfHeight);

    // 更新朝向
    updateZhuhaoFacing();
}

void ZhuhaoEnemy::updateZhuhaoFacing() {
    // 朝向逻辑：
    // - 在上/下边缘(y轴极点)：朝向移动方向
    // - 在左/右边缘(x轴极点)：朝向地图内部

    bool shouldFaceRight = true;

    switch (m_currentEdge) {
        case 0:  // 上边
            // 顺时针向右移动，逆时针向左移动
            shouldFaceRight = m_movingClockwise;
            break;
        case 2:  // 下边
            // 顺时针向左移动，逆时针向右移动
            shouldFaceRight = !m_movingClockwise;
            break;
        case 1:  // 右边 - 朝向地图内部(左)
            shouldFaceRight = false;
            break;
        case 3:  // 左边 - 朝向地图内部(右)
            shouldFaceRight = true;
            break;
    }

    // 更新图片朝向
    if (shouldFaceRight != facingRight) {
        QPixmap cur = pixmap();
        if (!cur.isNull()) {
            QPixmap flipped = cur.transformed(QTransform().scale(-1, 1));
            QGraphicsPixmapItem::setPixmap(flipped);
            facingRight = shouldFaceRight;
        }
    }
}

ZhuhaoEnemy::EdgePosition ZhuhaoEnemy::getCurrentEdgePosition() {
    double x = pos().x();
    double y = pos().y();

    bool atLeft = qAbs(x - MAP_LEFT) < CORNER_THRESHOLD;
    bool atRight = qAbs(x - MAP_RIGHT) < CORNER_THRESHOLD;
    bool atTop = qAbs(y - MAP_TOP) < CORNER_THRESHOLD;
    bool atBottom = qAbs(y - MAP_BOTTOM) < CORNER_THRESHOLD;

    // 角落检测
    if (atLeft && atTop)
        return CORNER_TOP_LEFT;
    if (atRight && atTop)
        return CORNER_TOP_RIGHT;
    if (atLeft && atBottom)
        return CORNER_BOTTOM_LEFT;
    if (atRight && atBottom)
        return CORNER_BOTTOM_RIGHT;

    // 边缘检测
    if (atTop)
        return EDGE_TOP;
    if (atBottom)
        return EDGE_BOTTOM;
    if (atLeft)
        return EDGE_LEFT;
    if (atRight)
        return EDGE_RIGHT;

    // 默认返回最近的边缘
    double distToLeft = qAbs(x - MAP_LEFT);
    double distToRight = qAbs(x - MAP_RIGHT);
    double distToTop = qAbs(y - MAP_TOP);
    double distToBottom = qAbs(y - MAP_BOTTOM);

    double minDist = qMin(qMin(distToLeft, distToRight), qMin(distToTop, distToBottom));

    if (minDist == distToTop)
        return EDGE_TOP;
    if (minDist == distToBottom)
        return EDGE_BOTTOM;
    if (minDist == distToLeft)
        return EDGE_LEFT;
    return EDGE_RIGHT;
}

bool ZhuhaoEnemy::isAtCorner() {
    EdgePosition pos = getCurrentEdgePosition();
    return pos == CORNER_TOP_LEFT || pos == CORNER_TOP_RIGHT ||
           pos == CORNER_BOTTOM_LEFT || pos == CORNER_BOTTOM_RIGHT;
}

double ZhuhaoEnemy::getInwardAngle() {
    // 获取向内的中心角度（用于弹幕发射）
    EdgePosition edgePos = getCurrentEdgePosition();

    switch (edgePos) {
        case EDGE_TOP:
        case CORNER_TOP_LEFT:
        case CORNER_TOP_RIGHT:
            return M_PI / 2;  // 向下 (90度)
        case EDGE_BOTTOM:
        case CORNER_BOTTOM_LEFT:
        case CORNER_BOTTOM_RIGHT:
            return -M_PI / 2;  // 向上 (-90度 或 270度)
        case EDGE_LEFT:
            return 0;  // 向右 (0度)
        case EDGE_RIGHT:
            return M_PI;  // 向左 (180度)
    }
    return 0;
}

void ZhuhaoEnemy::onShootTimer() {
    if (m_isPaused || !scene())
        return;

    shootBarrage();
}

void ZhuhaoEnemy::shootBarrage() {
    if (!scene())
        return;

    // 360°均匀发射
    double angleStep = 2 * M_PI / m_bulletCount;  // 360° / 12 = 30°每发

    QPointF myPos = pos();
    QPointF bulletStart(myPos.x() + boundingRect().width() / 2,
                        myPos.y() + boundingRect().height() / 2);

    qDebug() << "祝昊发射360°弹幕 - 位置:" << bulletStart << "子弹数:" << m_bulletCount;

    for (int i = 0; i < m_bulletCount; ++i) {
        double angle = i * angleStep;

        // 随机选择子弹类型
        int bulletTypeRand = QRandomGenerator::global()->bounded(3);
        ZhuhaoProjectile::BulletType bulletType;
        switch (bulletTypeRand) {
            case 0:
                bulletType = ZhuhaoProjectile::SLEEP_ZZZ;
                break;
            case 1:
                bulletType = ZhuhaoProjectile::CONFUSED;
                break;
            case 2:
            default:
                bulletType = ZhuhaoProjectile::CPU;
                break;
        }

        ZhuhaoProjectile* projectile = new ZhuhaoProjectile(
            bulletType, bulletStart, angle, m_bulletSpeed, scene());

        scene()->addItem(projectile);
    }

    AudioManager::instance().playSound("enemy_shoot");
}

void ZhuhaoEnemy::attackPlayer() {
    // 朱昊不进行近战攻击，只发射弹幕
}

void ZhuhaoEnemy::move() {
    // 朱昊不使用基类的移动逻辑，不跟随玩家
    // 移动由m_moveTimer和moveAlongEdge()处理
}

void ZhuhaoEnemy::takeDamage(int damage) {
    Enemy::takeDamage(damage);
}

void ZhuhaoEnemy::pauseTimers() {
    Enemy::pauseTimers();
    if (m_moveTimer)
        m_moveTimer->stop();
    if (m_shootTimer)
        m_shootTimer->stop();
}

void ZhuhaoEnemy::resumeTimers() {
    Enemy::resumeTimers();
    if (m_moveTimer)
        m_moveTimer->start(16);
    if (m_shootTimer)
        m_shootTimer->start(SHOOT_COOLDOWN);
}

// ==================== ZhuhaoProjectile 实现 ====================

ZhuhaoProjectile::ZhuhaoProjectile(BulletType type, QPointF startPos, double angle, double speed, QGraphicsScene* scene)
    : QObject(nullptr),
      QGraphicsPixmapItem(),
      m_type(type),
      m_angle(angle),
      m_speed(speed),
      m_moveTimer(nullptr),
      m_collisionTimer(nullptr),
      m_isPaused(false),
      m_isDestroying(false) {
    Q_UNUSED(scene);

    // 计算移动方向
    m_dx = qCos(angle) * speed;
    m_dy = qSin(angle) * speed;

    setPos(startPos);
    setZValue(50);

    // 创建视觉效果
    createVisual();

    // 创建移动定时器
    m_moveTimer = new QTimer(this);
    connect(m_moveTimer, &QTimer::timeout, this, &ZhuhaoProjectile::onMoveTimer);
    m_moveTimer->start(16);

    // 创建碰撞检测定时器
    m_collisionTimer = new QTimer(this);
    connect(m_collisionTimer, &QTimer::timeout, this, &ZhuhaoProjectile::checkCollision);
    m_collisionTimer->start(50);
}

ZhuhaoProjectile::~ZhuhaoProjectile() {
    if (m_moveTimer) {
        m_moveTimer->stop();
        delete m_moveTimer;
        m_moveTimer = nullptr;
    }
    if (m_collisionTimer) {
        m_collisionTimer->stop();
        delete m_collisionTimer;
        m_collisionTimer = nullptr;
    }
}

void ZhuhaoProjectile::createVisual() {
    QPixmap bulletPix;

    switch (m_type) {
        case SLEEP_ZZZ: {
            // 创建 "zzz" 文字图片 - 带背景的圆形气泡
            bulletPix = QPixmap(50, 30);
            bulletPix.fill(Qt::transparent);
            QPainter painter(&bulletPix);
            painter.setRenderHint(QPainter::Antialiasing);
            // 绘制蓝色气泡背景
            painter.setBrush(QColor(100, 100, 255, 180));
            painter.setPen(QPen(QColor(50, 50, 200), 2));
            painter.drawRoundedRect(2, 2, 46, 26, 10, 10);
            // 绘制文字
            QFont font;
            font.setPointSize(16);
            font.setBold(true);
            painter.setFont(font);
            painter.setPen(Qt::white);
            painter.drawText(bulletPix.rect(), Qt::AlignCenter, "zzz");
            painter.end();
            break;
        }
        case CONFUSED: {
            // 创建 "叽里咕噜" 文字图片 - 带背景的椭圆气泡
            bulletPix = QPixmap(90, 30);
            bulletPix.fill(Qt::transparent);
            QPainter painter(&bulletPix);
            painter.setRenderHint(QPainter::Antialiasing);
            // 绘制橙色气泡背景
            painter.setBrush(QColor(255, 165, 0, 180));
            painter.setPen(QPen(QColor(200, 100, 0), 2));
            painter.drawRoundedRect(2, 2, 86, 26, 12, 12);
            // 绘制文字
            QFont font;
            font.setPointSize(12);
            font.setBold(true);
            painter.setFont(font);
            painter.setPen(Qt::white);
            painter.drawText(bulletPix.rect(), Qt::AlignCenter, "叽里咕噜");
            painter.end();
            break;
        }
        case CPU: {
            // 加载 CPU 子弹图片
            bulletPix = QPixmap("assets/items/bullet_cpu.png");
            if (bulletPix.isNull()) {
                // 如果加载失败，创建占位符
                bulletPix = QPixmap(30, 30);
                bulletPix.fill(Qt::cyan);
            } else {
                // 缩放到合适大小
                bulletPix = bulletPix.scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            break;
        }
    }

    setPixmap(bulletPix);

    // 设置变换原点为中心
    setTransformOriginPoint(boundingRect().center());
}

void ZhuhaoProjectile::onMoveTimer() {
    if (m_isPaused || m_isDestroying)
        return;

    QPointF newPos = pos() + QPointF(m_dx, m_dy);
    setPos(newPos);

    // 检查是否超出屏幕边界
    if (newPos.x() < -50 || newPos.x() > 850 ||
        newPos.y() < -50 || newPos.y() > 650) {
        m_isDestroying = true;
        if (m_moveTimer)
            m_moveTimer->stop();
        if (m_collisionTimer)
            m_collisionTimer->stop();
        if (scene())
            scene()->removeItem(this);
        deleteLater();
    }
}

void ZhuhaoProjectile::checkCollision() {
    if (m_isPaused || m_isDestroying || !scene())
        return;

    QList<QGraphicsItem*> collisions = collidingItems();
    for (QGraphicsItem* item : collisions) {
        Player* player = dynamic_cast<Player*>(item);
        if (player) {
            applyEffect(player);

            m_isDestroying = true;
            if (m_moveTimer)
                m_moveTimer->stop();
            if (m_collisionTimer)
                m_collisionTimer->stop();
            if (scene())
                scene()->removeItem(this);
            deleteLater();
            return;
        }
    }
}

void ZhuhaoProjectile::applyEffect(Player* player) {
    if (!player || !scene())
        return;

    switch (m_type) {
        case SLEEP_ZZZ:
            // 无伤害，100%昏迷效果（与枕头一致）
            qDebug() << "朱昊昏睡子弹命中 - 100%昏迷";
            if (player->canMove() && !player->isEffectOnCooldown()) {
                player->setEffectCooldown(true);
                player->setCanMove(false);

                // 显示"昏睡ZZZ"文字提示（与枕头一致）
                QGraphicsTextItem* sleepText = new QGraphicsTextItem("昏睡ZZZ");
                QFont font;
                font.setPointSize(16);
                font.setBold(true);
                sleepText->setFont(font);
                sleepText->setDefaultTextColor(QColor(128, 128, 128));  // 灰色，与枕头一致
                sleepText->setPos(player->pos().x(), player->pos().y() - 40);
                sleepText->setZValue(200);
                scene()->addItem(sleepText);

                QPointer<Player> playerPtr = player;

                // 1.5秒后恢复移动（与枕头一致，不跟随移动）
                QTimer::singleShot(1500, [playerPtr, sleepText]() {
                    if (playerPtr) {
                        playerPtr->setCanMove(true);
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
            break;

        case CONFUSED: {
            // 2点伤害，50%昏迷或50%惊吓
            qDebug() << "朱昊叽里咕噜子弹命中 - 2点伤害，50%昏迷或50%惊吓";
            player->takeDamage(2);
            if (!player->isEffectOnCooldown()) {
                player->setEffectCooldown(true);
                QPointer<Player> playerPtr = player;
                QGraphicsScene* currentScene = scene();

                if (QRandomGenerator::global()->bounded(100) < 50) {
                    // 50%昏迷效果（与枕头一致）
                    if (player->canMove()) {
                        player->setCanMove(false);

                        // 显示"昏睡ZZZ"文字提示（与枕头一致）
                        QGraphicsTextItem* sleepText = new QGraphicsTextItem("昏睡ZZZ");
                        QFont font;
                        font.setPointSize(16);
                        font.setBold(true);
                        sleepText->setFont(font);
                        sleepText->setDefaultTextColor(QColor(128, 128, 128));  // 灰色，与枕头一致
                        sleepText->setPos(player->pos().x(), player->pos().y() - 40);
                        sleepText->setZValue(200);
                        currentScene->addItem(sleepText);

                        // 1.5秒后恢复移动（与枕头一致，不跟随移动）
                        QTimer::singleShot(1500, [playerPtr, sleepText]() {
                            if (playerPtr) {
                                playerPtr->setCanMove(true);
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
                } else {
                    // 50%惊吓效果（与闹钟一致：移动速度增加但受伤提升150%）
                    if (!player->isScared()) {
                        // 显示"惊吓！！"文字提示
                        QGraphicsTextItem* scareText = new QGraphicsTextItem("惊吓！！");
                        QFont font;
                        font.setPointSize(16);
                        font.setBold(true);
                        scareText->setFont(font);
                        scareText->setDefaultTextColor(QColor(128, 128, 128));  // 灰色，与闹钟一致
                        scareText->setPos(player->pos().x(), player->pos().y() - 40);
                        scareText->setZValue(200);
                        currentScene->addItem(scareText);

                        // 应用惊吓效果
                        player->setScared(true);

                        QTimer* followTimer = new QTimer();
                        QObject::connect(followTimer, &QTimer::timeout, [playerPtr, scareText]() {
                            if (playerPtr && scareText && scareText->scene()) {
                                scareText->setPos(playerPtr->pos().x(), playerPtr->pos().y() - 40);
                            }
                        });
                        followTimer->start(16);

                        // 3秒后恢复
                        QTimer::singleShot(3000, player, [playerPtr, scareText, followTimer]() {
                            if (followTimer) {
                                followTimer->stop();
                                delete followTimer;
                            }
                            if (playerPtr) {
                                playerPtr->setScared(false);
                                QTimer::singleShot(3000, playerPtr, [playerPtr]() {
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
                    } else {
                        // 已经惊吓中，解除冷却
                        player->setEffectCooldown(false);
                    }
                }
            }
            break;
        }

        case CPU: {
            // 2点伤害，100%惊吓效果
            qDebug() << "朱昊CPU子弹命中 - 2点伤害，100%惊吓";
            player->takeDamage(2);
            if (!player->isEffectOnCooldown() && !player->isScared()) {
                player->setEffectCooldown(true);
                QPointer<Player> playerPtr = player;
                QGraphicsScene* currentScene = scene();

                // 显示"惊吓！！"文字提示
                QGraphicsTextItem* scareText = new QGraphicsTextItem("惊吓！！");
                QFont font;
                font.setPointSize(16);
                font.setBold(true);
                scareText->setFont(font);
                scareText->setDefaultTextColor(QColor(128, 128, 128));  // 灰色，与闹钟一致
                scareText->setPos(player->pos().x(), player->pos().y() - 40);
                scareText->setZValue(200);
                currentScene->addItem(scareText);

                // 应用惊吓效果（与闹钟一致：移动速度增加但受伤提升150%）
                player->setScared(true);

                QTimer* followTimer = new QTimer();
                QObject::connect(followTimer, &QTimer::timeout, [playerPtr, scareText]() {
                    if (playerPtr && scareText && scareText->scene()) {
                        scareText->setPos(playerPtr->pos().x(), playerPtr->pos().y() - 40);
                    }
                });
                followTimer->start(16);

                // 3秒后恢复
                QTimer::singleShot(3000, player, [playerPtr, scareText, followTimer]() {
                    if (followTimer) {
                        followTimer->stop();
                        delete followTimer;
                    }
                    if (playerPtr) {
                        playerPtr->setScared(false);
                        QTimer::singleShot(3000, playerPtr, [playerPtr]() {
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
            break;
        }
    }
}

void ZhuhaoProjectile::setPaused(bool paused) {
    m_isPaused = paused;
    if (paused) {
        if (m_moveTimer)
            m_moveTimer->stop();
        if (m_collisionTimer)
            m_collisionTimer->stop();
    } else {
        if (m_moveTimer)
            m_moveTimer->start(16);
        if (m_collisionTimer)
            m_collisionTimer->start(50);
    }
}
