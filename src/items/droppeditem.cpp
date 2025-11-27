#include "droppeditem.h"
#include <QDebug>
#include <QFile>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QRandomGenerator>
#include <QtMath>
#include "../core/audiomanager.h"
#include "../entities/player.h"

DroppedItem::DroppedItem(DroppedItemType type, const QPointF& pos, Player* player, QObject* parent)
    : QObject(parent),
      m_type(type),
      m_player(player),
      m_canPickup(false),
      m_isPickingUp(false),
      m_isPaused(false),
      m_hasScatterTarget(false) {
    // 加载道具图片
    loadItemPixmap();

    // 设置初始位置
    setPos(pos);

    // 设置Z值，确保道具显示在合适的层级
    setZValue(50);

    // 创建碰撞检测定时器
    m_collisionTimer = new QTimer(this);
    connect(m_collisionTimer, &QTimer::timeout, this, &DroppedItem::checkPlayerCollision);
    m_collisionTimer->start(50);  // 每50ms检测一次

    // 创建拾取延迟定时器（1秒后才能拾取）
    m_pickupDelayTimer = new QTimer(this);
    m_pickupDelayTimer->setSingleShot(true);
    connect(m_pickupDelayTimer, &QTimer::timeout, this, &DroppedItem::enablePickup);
    m_pickupDelayTimer->start(1000);  // 1秒延迟
}

DroppedItem::~DroppedItem() {
    if (m_collisionTimer) {
        m_collisionTimer->stop();
    }
    if (m_pickupDelayTimer) {
        m_pickupDelayTimer->stop();
    }
}

QString DroppedItem::getItemImagePath(DroppedItemType type) {
    switch (type) {
        case DroppedItemType::RED_HEART:
            return "assets/props/red_heart.png";
        case DroppedItemType::BLACK_HEART:
            return "assets/props/black_heart.png";
        case DroppedItemType::BLOOD_BAG:
            return "assets/props/blood_bag.png";
        case DroppedItemType::DAMAGE_BOOST:
            return "assets/props/damage_boost.png";
        case DroppedItemType::FIRE_RATE_BOOST:
            return "assets/props/fire_rate_boost.png";
        case DroppedItemType::FROST_SLOWDOWN:
            return "assets/props/frost_slowdown.png";
        case DroppedItemType::MOVEMENT_SPEED:
            return "assets/props/movement_speed_boost.png";
        case DroppedItemType::SHIELD:
            return "assets/props/shield.png";
        case DroppedItemType::KEY:
            return "assets/props/key.png";
        default:
            return "";
    }
}

QString DroppedItem::getItemName() const {
    switch (m_type) {
        case DroppedItemType::RED_HEART:
            return "红心";
        case DroppedItemType::BLACK_HEART:
            return "黑心";
        case DroppedItemType::BLOOD_BAG:
            return "血袋";
        case DroppedItemType::DAMAGE_BOOST:
            return "伤害提升";
        case DroppedItemType::FIRE_RATE_BOOST:
            return "射速提升";
        case DroppedItemType::FROST_SLOWDOWN:
            return "冰冻减速";
        case DroppedItemType::MOVEMENT_SPEED:
            return "移动速度提升";
        case DroppedItemType::SHIELD:
            return "护盾";
        case DroppedItemType::KEY:
            return "钥匙";
        default:
            return "未知道具";
    }
}

void DroppedItem::loadItemPixmap() {
    QString path = getItemImagePath(m_type);
    QPixmap pix(path);

    if (pix.isNull()) {
        qWarning() << "DroppedItem: 无法加载道具图片:" << path;
        // 创建一个默认的彩色方块作为占位符
        pix = QPixmap(32, 32);
        pix.fill(Qt::yellow);
    } else {
        // 缩放到合适大小（32x32）
        pix = pix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    setPixmap(pix);

    // 设置变换原点为中心（用于缩放动画）
    setTransformOriginPoint(boundingRect().center());
}

void DroppedItem::setScatterTarget(const QPointF& targetPos) {
    m_scatterTarget = targetPos;
    m_hasScatterTarget = true;
    startScatterAnimation();
}

void DroppedItem::startScatterAnimation() {
    if (!m_hasScatterTarget)
        return;

    // 创建散落动画
    QPropertyAnimation* moveAnim = new QPropertyAnimation(this, "pos", this);
    moveAnim->setDuration(300);
    moveAnim->setStartValue(pos());
    moveAnim->setEndValue(m_scatterTarget);
    moveAnim->setEasingCurve(QEasingCurve::OutQuad);
    moveAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void DroppedItem::setPaused(bool paused) {
    m_isPaused = paused;

    if (paused) {
        if (m_collisionTimer)
            m_collisionTimer->stop();
        if (m_pickupDelayTimer && m_pickupDelayTimer->isActive()) {
            m_pickupDelayTimer->stop();
        }
    } else {
        if (m_collisionTimer && !m_isPickingUp)
            m_collisionTimer->start(50);
        // 注意：延迟定时器如果已经完成就不需要重新启动
    }
}

void DroppedItem::enablePickup() {
    m_canPickup = true;
    qDebug() << "DroppedItem:" << getItemName() << "现在可以拾取了";
}

void DroppedItem::checkPlayerCollision() {
    if (!m_canPickup || m_isPickingUp || m_isPaused || !m_player || !scene()) {
        return;
    }

    // 计算与玩家的距离
    QPointF playerCenter = m_player->pos() + QPointF(m_player->pixmap().width() / 2,
                                                     m_player->pixmap().height() / 2);
    QPointF itemCenter = pos() + QPointF(pixmap().width() / 2, pixmap().height() / 2);

    double dx = playerCenter.x() - itemCenter.x();
    double dy = playerCenter.y() - itemCenter.y();
    double distance = qSqrt(dx * dx + dy * dy);

    // 拾取距离（玩家半径 + 道具半径）
    double pickupRange = 40;

    if (distance < pickupRange) {
        // 触发拾取
        m_isPickingUp = true;
        m_collisionTimer->stop();
        startPickupAnimation();
    }
}

void DroppedItem::startPickupAnimation() {
    // 拾取动画：先放大到1.3倍，然后缩小到0并消失
    QPropertyAnimation* scaleAnim = new QPropertyAnimation(this, "scale", this);
    scaleAnim->setDuration(250);
    scaleAnim->setKeyValueAt(0, 1.0);
    scaleAnim->setKeyValueAt(0.4, 1.3);  // 放大
    scaleAnim->setKeyValueAt(1.0, 0.0);  // 缩小消失
    scaleAnim->setEasingCurve(QEasingCurve::InOutQuad);

    connect(scaleAnim, &QPropertyAnimation::finished, this, &DroppedItem::onPickupAnimationFinished);
    scaleAnim->start(QAbstractAnimation::DeleteWhenStopped);

    // 播放拾取音效
    AudioManager::instance().playSound("chest_open");  // 复用宝箱音效，或者可以添加专门的拾取音效
}

void DroppedItem::onPickupAnimationFinished() {
    // 应用道具效果
    applyEffect();

    // 从场景中移除
    if (scene()) {
        scene()->removeItem(this);
    }

    // 延迟删除
    deleteLater();
}

void DroppedItem::applyEffect() {
    if (!m_player) {
        qWarning() << "DroppedItem::applyEffect: 玩家引用无效";
        return;
    }

    QString pickupText;
    QColor textColor = Qt::white;

    switch (m_type) {
        case DroppedItemType::RED_HEART: {
            // 红心：增加1点血量（若已满则不增加）
            double currentHealth = m_player->getCurrentHealth();
            double maxHealth = m_player->getMaxHealth();
            if (currentHealth < maxHealth) {
                m_player->addRedHearts(1);
                pickupText = "❤️ +1 血量";
                textColor = Qt::red;
            } else {
                pickupText = "❤️ 血量已满";
                textColor = QColor(255, 150, 150);
            }
            break;
        }

        case DroppedItemType::BLACK_HEART: {
            // 黑心：增加一颗黑心（用于复活）
            m_player->addBlackHearts(1);
            pickupText = "🖤 +1 黑心";
            textColor = QColor(80, 80, 80);
            break;
        }

        case DroppedItemType::BLOOD_BAG: {
            // 血袋：增加2点血量上限和2点当前血量
            m_player->addRedContainers(2);
            m_player->addRedHearts(2);
            pickupText = "💉 +2 血量上限 & +2 血量";
            textColor = QColor(200, 50, 50);
            break;
        }

        case DroppedItemType::DAMAGE_BOOST: {
            // 伤害提升：子弹伤害+1
            int currentDamage = m_player->getBulletHurt();
            m_player->setBulletHurt(currentDamage + 1);
            pickupText = QString("⚔️ 伤害 +1 (当前: %1)").arg(currentDamage + 1);
            textColor = QColor(255, 100, 100);
            break;
        }

        case DroppedItemType::FIRE_RATE_BOOST: {
            // 射速提升：射速x1.5（上限6倍，即冷却时间最低为初始的1/6）
            int currentCooldown = m_player->getShootCooldown();
            int baseCooldown = 150;              // 基础冷却时间
            int minCooldown = baseCooldown / 6;  // 最低冷却时间（6倍射速）

            int newCooldown = static_cast<int>(currentCooldown / 1.5);
            if (newCooldown < minCooldown) {
                newCooldown = minCooldown;
                pickupText = "🔫 射速已达最高！";
                textColor = QColor(255, 200, 100);
            } else {
                m_player->setShootCooldown(newCooldown);
                pickupText = "🔫 射速提升!";
                textColor = QColor(255, 200, 100);
            }
            break;
        }

        case DroppedItemType::FROST_SLOWDOWN: {
            // 冰冻减速：增加20%寒冰子弹概率，最多60%
            int currentFrostChance = m_player->getFrostChance();
            if (currentFrostChance >= 60) {
                pickupText = "❄️ 寒冰子弹概率已达最高";
                textColor = QColor(150, 200, 255);
            } else {
                m_player->addFrostChance(20);
                pickupText = QString("❄️ 寒冰概率 +20%% (当前: %1%%)").arg(currentFrostChance + 20);
                textColor = QColor(100, 200, 255);
            }
            break;
        }

        case DroppedItemType::MOVEMENT_SPEED: {
            // 移动速度：+20%（上限2.5倍）
            double currentSpeed = m_player->getSpeed();
            double baseSpeed = 5.0;             // 基础速度
            double maxSpeed = baseSpeed * 2.5;  // 最大速度（250%）

            double newSpeed = currentSpeed * 1.2;
            if (newSpeed > maxSpeed) {
                newSpeed = maxSpeed;
                pickupText = "⚡ 移速已达最高！";
                textColor = QColor(100, 200, 255);
            } else {
                m_player->setSpeed(newSpeed);
                pickupText = "⚡ 移速提升!";
                textColor = QColor(100, 200, 255);
            }
            break;
        }

        case DroppedItemType::SHIELD: {
            // 护盾：增加一个护盾
            m_player->addShield(1);
            pickupText = "🛡️ +1 护盾";
            textColor = QColor(100, 255, 150);
            break;
        }

        case DroppedItemType::KEY: {
            // 钥匙
            m_player->addKeys(1);
            pickupText = "🔑 获得一把钥匙";
            textColor = QColor(255, 215, 0);
            break;
        }

        default:
            pickupText = "获得道具";
            break;
    }

    // 显示拾取提示
    showPickupText(pickupText, textColor);

    qDebug() << "DroppedItem: 玩家拾取了" << getItemName();
}

void DroppedItem::showPickupText(const QString& text, const QColor& color) {
    if (!scene() || !m_player)
        return;

    QGraphicsTextItem* textItem = new QGraphicsTextItem(text);
    textItem->setDefaultTextColor(color);
    textItem->setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    textItem->setZValue(1000);

    // 显示在玩家上方
    QPointF textPos = m_player->pos() + QPointF(
                                            m_player->pixmap().width() / 2 - textItem->boundingRect().width() / 2,
                                            -30);
    textItem->setPos(textPos);

    scene()->addItem(textItem);

    // 上浮并淡出动画
    QPointer<QGraphicsTextItem> textPtr(textItem);
    QPointer<QGraphicsScene> scenePtr(scene());

    // 使用QTimer实现简单的上浮动画
    QTimer* moveTimer = new QTimer;
    QTimer* fadeTimer = new QTimer;

    auto stepPtr = std::make_shared<int>(0);

    QObject::connect(moveTimer, &QTimer::timeout, [textPtr, moveTimer, stepPtr]() {
        if (!textPtr) {
            moveTimer->stop();
            moveTimer->deleteLater();
            return;
        }
        textPtr->setPos(textPtr->pos() + QPointF(0, -1.5));
        (*stepPtr)++;
        if (*stepPtr >= 30) {
            moveTimer->stop();
            moveTimer->deleteLater();
        }
    });

    QObject::connect(fadeTimer, &QTimer::timeout, [textPtr, scenePtr, fadeTimer]() {
        if (!textPtr) {
            fadeTimer->stop();
            fadeTimer->deleteLater();
            return;
        }

        qreal opacity = textPtr->opacity() - 0.04;
        if (opacity <= 0) {
            if (scenePtr && textPtr->scene() == scenePtr) {
                scenePtr->removeItem(textPtr.data());
            }
            delete textPtr.data();
            fadeTimer->stop();
            fadeTimer->deleteLater();
        } else {
            textPtr->setOpacity(opacity);
        }
    });

    moveTimer->start(30);
    fadeTimer->start(50);
}
