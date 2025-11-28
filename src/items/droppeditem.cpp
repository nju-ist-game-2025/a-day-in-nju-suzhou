#include "droppeditem.h"
#include <QDebug>
#include <QFile>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QRandomGenerator>
#include <QtMath>
#include "../core/audiomanager.h"
#include "../core/configmanager.h"
#include "../entities/player.h"
#include "itemeffectconfig.h"

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
        case DroppedItemType::TICKET:
            return "assets/items/ticket.png";
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
        case DroppedItemType::TICKET:
            return "车票";
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

    // 获取道具配置key
    QString itemKey = getItemConfigKey();
    ItemEffectData config = ItemEffectConfig::instance().getItemEffect(itemKey);

    QString pickupText;
    QColor textColor = config.color;
    QMap<QString, QString> textParams;

    // 根据效果类型应用效果
    if (config.effectType == "heal") {
        // 红心：增加血量
        double currentHealth = m_player->getCurrentHealth();
        double maxHealth = m_player->getMaxHealth();
        int healValue = config.getValue();

        if (currentHealth < maxHealth) {
            m_player->addRedHearts(healValue);
            textParams["value"] = QString::number(healValue);
            pickupText = ItemEffectConfig::formatText(config.pickupText, textParams);
        } else {
            pickupText = config.pickupTextFull;
            textColor = QColor(255, 150, 150);
        }
    } else if (config.effectType == "black_heart") {
        // 黑心：复活用
        int value = config.getValue();
        m_player->addBlackHearts(value);
        textParams["value"] = QString::number(value);
        pickupText = ItemEffectConfig::formatText(config.pickupText, textParams);
    } else if (config.effectType == "blood_bag") {
        // 血袋：增加血量上限和当前血量
        int maxBonus = config.getMaxHealthBonus();
        int currentBonus = config.getCurrentHealthBonus();
        m_player->addRedContainers(maxBonus);
        m_player->addRedHearts(currentBonus);
        textParams["maxHealthBonus"] = QString::number(maxBonus);
        textParams["currentHealthBonus"] = QString::number(currentBonus);
        pickupText = ItemEffectConfig::formatText(config.pickupText, textParams);
    } else if (config.effectType == "damage") {
        // 伤害提升
        int value = config.getValue();
        int currentDamage = m_player->getBulletHurt();
        m_player->setBulletHurt(currentDamage + value);
        textParams["value"] = QString::number(value);
        pickupText = ItemEffectConfig::formatText(config.pickupText, textParams) +
                     QString(" (当前: %1)").arg(currentDamage + value);
    } else if (config.effectType == "fire_rate") {
        // 射速提升
        int currentCooldown = m_player->getShootCooldown();
        int baseCooldown = config.getBaseCooldown();
        double multiplier = config.getMultiplier();
        double maxMultiplier = config.getMaxMultiplier();
        int minCooldown = static_cast<int>(baseCooldown / maxMultiplier);

        int newCooldown = static_cast<int>(currentCooldown / multiplier);
        if (newCooldown < minCooldown) {
            newCooldown = minCooldown;
            pickupText = config.pickupTextMax;
        } else {
            m_player->setShootCooldown(newCooldown);
            pickupText = config.pickupText;
        }
    } else if (config.effectType == "frost_chance") {
        // 冰冻减速
        int value = config.getValue();
        int maxValue = config.getMaxValue();
        int currentFrostChance = m_player->getFrostChance();

        if (currentFrostChance >= maxValue) {
            pickupText = config.pickupTextMax;
            textColor = QColor(150, 200, 255);
        } else {
            m_player->addFrostChance(value);
            textParams["value"] = QString::number(value);
            pickupText = ItemEffectConfig::formatText(config.pickupText, textParams) +
                         QString(" (当前: %1%%)").arg(currentFrostChance + value);
        }
    } else if (config.effectType == "speed") {
        // 移动速度（基础速度从 config.json 读取）
        double currentSpeed = m_player->getSpeed();
        double baseSpeed = ConfigManager::instance().getPlayerDouble("speed", 5.0);
        double multiplier = config.getMultiplier();
        double maxMultiplier = config.getMaxMultiplier();
        double maxSpeed = baseSpeed * maxMultiplier;

        double newSpeed = currentSpeed * multiplier;
        if (newSpeed > maxSpeed) {
            newSpeed = maxSpeed;
            pickupText = config.pickupTextMax;
        } else {
            m_player->setSpeed(newSpeed);
            pickupText = config.pickupText;
        }
    } else if (config.effectType == "shield") {
        // 护盾
        int value = config.getValue();
        m_player->addShield(value);
        textParams["value"] = QString::number(value);
        pickupText = ItemEffectConfig::formatText(config.pickupText, textParams);
    } else if (config.effectType == "key") {
        // 钥匙
        int value = config.getValue();
        m_player->addKeys(value);
        pickupText = config.pickupText;
    } else if (m_type == DroppedItemType::TICKET) {
        // 车票：通关奖励，不显示普通提示，而是触发通关动画
        qDebug() << "DroppedItem: 车票applyEffect开始执行";
        ConfigManager::instance().setGameCompleted(true);
        qDebug() << "DroppedItem: 即将发送ticketPickedUp信号";
        emit ticketPickedUp();
        qDebug() << "DroppedItem: ticketPickedUp信号已发送";
        return;  // 车票不显示普通提示，由通关动画处理
    } else {
        // 未知类型
        pickupText = "获得道具";
    }

    // 显示拾取提示
    showPickupText(pickupText, textColor);

    qDebug() << "DroppedItem: 玩家拾取了" << config.name;
}

QString DroppedItem::getItemConfigKey() const {
    switch (m_type) {
        case DroppedItemType::RED_HEART:
            return "red_heart";
        case DroppedItemType::BLACK_HEART:
            return "black_heart";
        case DroppedItemType::BLOOD_BAG:
            return "blood_bag";
        case DroppedItemType::DAMAGE_BOOST:
            return "damage_boost";
        case DroppedItemType::FIRE_RATE_BOOST:
            return "fire_rate_boost";
        case DroppedItemType::FROST_SLOWDOWN:
            return "frost_slowdown";
        case DroppedItemType::MOVEMENT_SPEED:
            return "movement_speed";
        case DroppedItemType::SHIELD:
            return "shield";
        case DroppedItemType::KEY:
            return "key";
        case DroppedItemType::TICKET:
            return "ticket";
        default:
            return "unknown";
    }
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
