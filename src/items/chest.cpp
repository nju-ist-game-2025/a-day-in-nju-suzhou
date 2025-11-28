#include "chest.h"
#include <QApplication>
#include <QFont>
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QtMath>
#include "../constants.h"
#include "../core/audiomanager.h"
#include "droppeditemfactory.h"
#include "player.h"

// ==================== Chest 基类实现 ====================

Chest::Chest(Player* pl, ChestType type, const QPixmap& pic_chest, double scale)
    : m_chestType(type), m_isOpened(false), m_player(pl), m_hintText(nullptr), m_hintTimer(nullptr) {
    if (scale == 1.0) {
        this->setPixmap(pic_chest);
    } else {
        this->setPixmap(pic_chest.scaled(
            pic_chest.width() * scale,
            pic_chest.height() * scale,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    }

    // 创建提示文字定时器
    m_hintTimer = new QTimer(this);
    m_hintTimer->setSingleShot(true);
    connect(m_hintTimer, &QTimer::timeout, this, &Chest::hideHint);

    // 创建检查打开定时器
    m_checkOpenTimer = new QTimer(this);
    connect(m_checkOpenTimer, &QTimer::timeout, this, &Chest::tryOpen);
    m_checkOpenTimer->start(16);
}

Chest::~Chest() {
    if (m_checkOpenTimer) {
        m_checkOpenTimer->stop();
        disconnect(m_checkOpenTimer, nullptr, this, nullptr);
    }
    if (m_hintTimer) {
        m_hintTimer->stop();
    }
    hideHint();
}

void Chest::initItems() {
    // 基类不初始化物品，由子类实现
}

void Chest::showHint(const QString& text, const QColor& color) {
    if (!scene())
        return;

    // 如果已有提示，先隐藏
    hideHint();

    // 创建提示文字
    m_hintText = new QGraphicsTextItem();
    m_hintText->setPlainText(text);

    QFont font;
    font.setPointSize(12);
    font.setBold(true);
    m_hintText->setFont(font);
    m_hintText->setDefaultTextColor(color);

    // 设置位置在宝箱上方
    qreal textWidth = m_hintText->boundingRect().width();
    qreal chestWidth = this->boundingRect().width();
    m_hintText->setPos(this->pos().x() + chestWidth / 2 - textWidth / 2,
                       this->pos().y() - 60);
    m_hintText->setZValue(1000);

    scene()->addItem(m_hintText);

    // 3秒后自动隐藏
    m_hintTimer->start(3000);
}

void Chest::hideHint() {
    if (m_hintText) {
        if (m_hintText->scene()) {
            m_hintText->scene()->removeItem(m_hintText);
        }
        delete m_hintText;
        m_hintText = nullptr;
    }
}

void Chest::tryOpen() {
    // 如果已经打开过，不再处理
    if (m_isOpened) {
        return;
    }

    if (!scene() || !m_player) {
        return;
    }

    // 检查玩家是否在范围内并按下空格键
    if (m_player->isKeyPressed(Qt::Key_Space) &&
        abs(m_player->pos().x() - this->pos().x()) <= open_r &&
        abs(m_player->pos().y() - this->pos().y()) <= open_r) {
        doOpen();
    }
}

void Chest::doOpen() {
    // 标记为已打开
    m_isOpened = true;

    emit opened(this);

    // 立即停止并断开定时器，防止重复触发
    if (m_checkOpenTimer) {
        m_checkOpenTimer->stop();
        disconnect(m_checkOpenTimer, nullptr, this, nullptr);
    }

    AudioManager::instance().playSound("chest_open");
    qDebug() << "宝箱开启音效已触发";
    qDebug() << "宝箱被打开，类型:" << static_cast<int>(m_chestType);

    // 保存player的QPointer副本
    QPointer<Player> playerPtr = m_player;

    // 普通宝箱和锁定宝箱只掉落1个物品
    dropItems(1);

    // 隐藏提示
    hideHint();

    // 从场景移除
    if (scene()) {
        scene()->removeItem(this);
    }

    // 延迟删除
    deleteLater();
}

void Chest::bonusEffects() {
    // 检查player是否仍然有效
    if (!m_player || !m_player->scene()) {
        return;
    }

    // 获取原始指针用于创建效果
    Player* pl = m_player.data();

    QVector<StatusEffect*> effectstoPlayer;

    SpeedEffect* sp = new SpeedEffect(5, 1.5);
    effectstoPlayer.push_back(sp);
    DamageEffect* dam = new DamageEffect(5, 1.5);
    effectstoPlayer.push_back(dam);
    shootSpeedEffect* shootsp = new shootSpeedEffect(5, 1.5);
    effectstoPlayer.push_back(shootsp);
    blackHeartEffect* black1 = new blackHeartEffect(pl, 1);
    blackHeartEffect* black2 = new blackHeartEffect(pl, 1);
    effectstoPlayer.push_back(black1);
    effectstoPlayer.push_back(black2);
    decDamage* dec = new decDamage(5, 0.5);
    effectstoPlayer.push_back(dec);
    InvincibleEffect* inv = new InvincibleEffect(5);
    effectstoPlayer.push_back(inv);

    // 以1/3的概率获得额外增益效果
    int i = QRandomGenerator::global()->bounded(effectstoPlayer.size() * 3);
    if (i >= 0 && i < effectstoPlayer.size() && effectstoPlayer[i]) {
        // 应用前再次检查player
        if (m_player) {
            effectstoPlayer[i]->applyTo(m_player.data());
        }
        for (StatusEffect* effect : effectstoPlayer) {
            if (effect != effectstoPlayer[i]) {  // 只删除未使用的
                effect->deleteLater();
            }
        }
    } else {
        // 如果没有选中任何效果，清理所有效果
        for (StatusEffect* effect : effectstoPlayer) {
            effect->deleteLater();
        }
    }
}

// 掉落物品到场景中（使用指定物品池）
void Chest::dropItems(int count) {
    if (!scene() || !m_player || count <= 0)
        return;

    QPointF chestPos = this->pos() + QPointF(boundingRect().width() / 2, boundingRect().height() / 2);

    // 根据宝箱类型选择物品池
    ItemDropPool pool;
    switch (m_chestType) {
        case ChestType::Locked:
            pool = ItemDropPool::LOCKED_CHEST;
            break;
        case ChestType::Boss:
            pool = ItemDropPool::BOSS_CHEST;
            break;
        case ChestType::Normal:
        default:
            pool = ItemDropPool::NORMAL_CHEST;
            break;
    }

    // 使用工厂类掉落物品
    DroppedItemFactory::dropItemsScattered(pool, chestPos, count, m_player.data(), scene());

    qDebug() << "[Chest] 从位置" << chestPos << "掉落" << count << "个物品，类型:" << static_cast<int>(m_chestType);
}

// ==================== NormalChest 普通宝箱实现 ====================

NormalChest::NormalChest(Player* pl, const QPixmap& pic_chest, double scale)
    : Chest(pl, ChestType::Normal, pic_chest, scale) {
    initItems();
}

void NormalChest::initItems() {
    m_items.clear();

    // 普通宝箱物品
    DamageUpItem* dam = new DamageUpItem("", 1.05);
    m_items.push_back(dam);
    SpeedUpItem* spd = new SpeedUpItem("", 1.05);
    m_items.push_back(spd);
    ShootSpeedUpItem* shootspd = new ShootSpeedUpItem("", 1.05);
    m_items.push_back(shootspd);
    BulletSpeedUpItem* bltspd = new BulletSpeedUpItem("", 1.05);
    m_items.push_back(bltspd);
    RedHeartItem* redheart1 = new RedHeartItem("", 1);
    RedHeartContainerItem* redheart2 = new RedHeartContainerItem("", 1);
    m_items.push_back(redheart1);
    m_items.push_back(redheart2);
    KeyItem* key1 = new KeyItem("", 1);
    KeyItem* key2 = new KeyItem("", 2);
    m_items.push_back(key1);
    m_items.push_back(key2);
}

// ==================== LockedChest 高级宝箱实现 ====================

LockedChest::LockedChest(Player* pl, const QPixmap& pic_chest, double scale)
    : Chest(pl, ChestType::Locked, pic_chest, scale) {
    initItems();
}

void LockedChest::initItems() {
    m_items.clear();

    // 高级宝箱奖励提高
    DamageUpItem* dam = new DamageUpItem("", 1.1);
    m_items.push_back(dam);
    SpeedUpItem* spd = new SpeedUpItem("", 1.1);
    m_items.push_back(spd);
    ShootSpeedUpItem* shootspd = new ShootSpeedUpItem("", 1.1);
    m_items.push_back(shootspd);
    BulletSpeedUpItem* bltspd = new BulletSpeedUpItem("", 1.1);
    m_items.push_back(bltspd);
    RedHeartContainerItem* redheart1 = new RedHeartContainerItem("", 2);
    m_items.push_back(redheart1);
    BrimstoneItem* brim = new BrimstoneItem("");
    m_items.push_back(brim);
}

void LockedChest::tryOpen() {
    // 如果已经打开过，不再处理
    if (m_isOpened) {
        return;
    }

    if (!scene() || !m_player) {
        return;
    }

    // 检查玩家是否在范围内并按下空格键
    if (m_player->isKeyPressed(Qt::Key_Space) &&
        abs(m_player->pos().x() - this->pos().x()) <= open_r &&
        abs(m_player->pos().y() - this->pos().y()) <= open_r) {
        // 检查是否有钥匙
        if (m_player->getKeys() > 0) {
            m_player->addKeys(-1);  // 消耗一把钥匙
            doOpen();
        } else {
            // 没有钥匙，显示提示（黑色文字）
            showHint("你需要1把钥匙来打开它！\n钥匙可以在探索过程中获取", Qt::black);
        }
    }
}

// ==================== BossChest Boss特供宝箱实现 ====================

BossChest::BossChest(Player* pl, const QPixmap& pic_chest, double scale)
    : Chest(pl, ChestType::Boss, pic_chest, scale) {
    initItems();
}

void BossChest::initItems() {
    m_items.clear();

    // Boss宝箱包含普通宝箱内容
    DamageUpItem* dam = new DamageUpItem("", 1.05);
    m_items.push_back(dam);
    SpeedUpItem* spd = new SpeedUpItem("", 1.05);
    m_items.push_back(spd);
    ShootSpeedUpItem* shootspd = new ShootSpeedUpItem("", 1.05);
    m_items.push_back(shootspd);
    BulletSpeedUpItem* bltspd = new BulletSpeedUpItem("", 1.05);
    m_items.push_back(bltspd);
    RedHeartItem* redheart1 = new RedHeartItem("", 1);
    RedHeartContainerItem* redheart2 = new RedHeartContainerItem("", 1);
    m_items.push_back(redheart1);
    m_items.push_back(redheart2);
    KeyItem* key1 = new KeyItem("", 1);
    KeyItem* key2 = new KeyItem("", 2);
    m_items.push_back(key1);
    m_items.push_back(key2);
}

void BossChest::setCustomItems(const QVector<QString>& itemNames) {
    m_customItemNames = itemNames;
    qDebug() << "[BossChest] 设置自定义物品:" << itemNames;
}

void BossChest::doOpen() {
    // 标记为已打开
    m_isOpened = true;

    emit opened(this);

    // 立即停止并断开定时器，防止重复触发
    if (m_checkOpenTimer) {
        m_checkOpenTimer->stop();
        disconnect(m_checkOpenTimer, nullptr, this, nullptr);
    }

    AudioManager::instance().playSound("chest_open");
    qDebug() << "Boss宝箱开启！";

    // 保存player的QPointer副本
    QPointer<Player> playerPtr = m_player;

    if (!m_customItemNames.isEmpty()) {
        // 使用自定义物品列表（从level配置中读取，每个宝箱掉2个道具）
        QVector<DroppedItemType> customTypes;
        for (const QString& name : m_customItemNames) {
            customTypes.append(DroppedItemFactory::getItemTypeFromName(name));
        }

        QPointF chestPos = this->pos() + QPointF(boundingRect().width() / 2, boundingRect().height() / 2);
        DroppedItemFactory::dropSpecificItems(customTypes, chestPos, m_player.data(), scene());
        qDebug() << "Boss宝箱掉落自定义物品:" << m_customItemNames.size() << "个";
    } else {
        // 默认行为：每个Boss宝箱掉落2个物品
        dropItems(2);
        qDebug() << "Boss宝箱掉落默认2个物品";
    }

    // 隐藏提示
    hideHint();

    // 从场景移除
    if (scene()) {
        scene()->removeItem(this);
    }

    // 延迟删除
    deleteLater();
}
