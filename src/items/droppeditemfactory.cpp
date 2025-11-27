#include "droppeditemfactory.h"
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QtMath>
#include "../constants.h"
#include "../entities/player.h"

DroppedItemType DroppedItemFactory::getRandomItemType(ItemDropPool pool) {
    QVector<QPair<DroppedItemType, int>> itemPool;

    switch (pool) {
        case ItemDropPool::NORMAL_CHEST:
            itemPool = getNormalChestPool();
            break;
        case ItemDropPool::LOCKED_CHEST:
            itemPool = getLockedChestPool();
            break;
        case ItemDropPool::ENEMY_DROP:
            itemPool = getEnemyDropPool();
            break;
        case ItemDropPool::BOSS_CHEST:
            // Boss宝箱使用自定义物品，这里返回红心作为默认
            return DroppedItemType::RED_HEART;
    }

    return selectByWeight(itemPool);
}

QVector<QPair<DroppedItemType, int>> DroppedItemFactory::getNormalChestPool() {
    // 普通宝箱可以掉落：key, red_heart, shield, movement_speed_boost, damage_boost
    // 权重分配：红心30%, 钥匙20%, 护盾20%, 移速15%, 伤害15%
    return {
        {DroppedItemType::RED_HEART, 30},
        {DroppedItemType::KEY, 20},
        {DroppedItemType::SHIELD, 20},
        {DroppedItemType::MOVEMENT_SPEED, 15},
        {DroppedItemType::DAMAGE_BOOST, 15}};
}

QVector<QPair<DroppedItemType, int>> DroppedItemFactory::getLockedChestPool() {
    // 需要钥匙的宝箱可以掉落：blood_bag, frost_slowdown, fire_rate_boost
    // 权重分配：血袋35%, 冰霜减速35%, 射速提升30%
    return {
        {DroppedItemType::BLOOD_BAG, 35},
        {DroppedItemType::FROST_SLOWDOWN, 35},
        {DroppedItemType::FIRE_RATE_BOOST, 30}};
}

QVector<QPair<DroppedItemType, int>> DroppedItemFactory::getEnemyDropPool() {
    // 敌人掉落物品池（包含所有非黑心的物品）
    // 红心概率最高，其他较为均衡
    return {
        {DroppedItemType::RED_HEART, 35},
        {DroppedItemType::KEY, 10},
        {DroppedItemType::SHIELD, 10},
        {DroppedItemType::MOVEMENT_SPEED, 10},
        {DroppedItemType::DAMAGE_BOOST, 10},
        {DroppedItemType::BLOOD_BAG, 8},
        {DroppedItemType::FROST_SLOWDOWN, 10},
        {DroppedItemType::FIRE_RATE_BOOST, 7}};
}

DroppedItemType DroppedItemFactory::selectByWeight(const QVector<QPair<DroppedItemType, int>>& pool) {
    if (pool.isEmpty()) {
        return DroppedItemType::RED_HEART;
    }

    // 计算总权重
    int totalWeight = 0;
    for (const auto& item : pool) {
        totalWeight += item.second;
    }

    if (totalWeight <= 0) {
        return pool.first().first;
    }

    // 随机选择
    int roll = QRandomGenerator::global()->bounded(totalWeight);
    int cumulative = 0;

    for (const auto& item : pool) {
        cumulative += item.second;
        if (roll < cumulative) {
            return item.first;
        }
    }

    return pool.last().first;
}

DroppedItem* DroppedItemFactory::createDroppedItem(DroppedItemType type, const QPointF& pos, Player* player, QGraphicsScene* scene) {
    if (!player || !scene) {
        return nullptr;
    }

    DroppedItem* item = new DroppedItem(type, pos, player);
    scene->addItem(item);
    return item;
}

DroppedItem* DroppedItemFactory::dropRandomItem(ItemDropPool pool, const QPointF& pos, Player* player, QGraphicsScene* scene) {
    DroppedItemType type = getRandomItemType(pool);
    return createDroppedItem(type, pos, player, scene);
}

void DroppedItemFactory::dropItemsScattered(ItemDropPool pool, const QPointF& pos, int count, Player* player, QGraphicsScene* scene) {
    if (!player || !scene || count <= 0) {
        return;
    }

    // 散开角度间隔
    double angleStep = 360.0 / count;
    double scatterDistance = 50.0;

    for (int i = 0; i < count; ++i) {
        DroppedItemType type = getRandomItemType(pool);

        QPointF dropPos = pos;

        if (count > 1) {
            // 计算散开位置
            double angle = (angleStep * i + QRandomGenerator::global()->bounded(20) - 10) * M_PI / 180.0;
            double dist = scatterDistance + QRandomGenerator::global()->bounded(20) - 10;
            double offsetX = dist * qCos(angle);
            double offsetY = dist * qSin(angle);
            dropPos = pos + QPointF(offsetX, offsetY);

            // 边界检查
            dropPos.setX(qBound(50.0, dropPos.x(), scene_bound_x - 50.0));
            dropPos.setY(qBound(50.0, dropPos.y(), scene_bound_y - 50.0));
        }

        createDroppedItem(type, dropPos, player, scene);
    }
}

void DroppedItemFactory::dropSpecificItems(const QVector<DroppedItemType>& types, const QPointF& pos, Player* player, QGraphicsScene* scene) {
    if (!player || !scene || types.isEmpty()) {
        return;
    }

    int count = types.size();
    double angleStep = 360.0 / count;
    double scatterDistance = 50.0;

    for (int i = 0; i < count; ++i) {
        QPointF dropPos = pos;

        if (count > 1) {
            double angle = (angleStep * i) * M_PI / 180.0;
            double offsetX = scatterDistance * qCos(angle);
            double offsetY = scatterDistance * qSin(angle);
            dropPos = pos + QPointF(offsetX, offsetY);

            dropPos.setX(qBound(50.0, dropPos.x(), scene_bound_x - 50.0));
            dropPos.setY(qBound(50.0, dropPos.y(), scene_bound_y - 50.0));
        }

        createDroppedItem(types[i], dropPos, player, scene);
    }
}

DroppedItemType DroppedItemFactory::getItemTypeFromName(const QString& name) {
    static const QMap<QString, DroppedItemType> nameMap = {
        {"red_heart", DroppedItemType::RED_HEART},
        {"black_heart", DroppedItemType::BLACK_HEART},
        {"blood_bag", DroppedItemType::BLOOD_BAG},
        {"damage_boost", DroppedItemType::DAMAGE_BOOST},
        {"fire_rate_boost", DroppedItemType::FIRE_RATE_BOOST},
        {"frost_slowdown", DroppedItemType::FROST_SLOWDOWN},
        {"movement_speed_boost", DroppedItemType::MOVEMENT_SPEED},
        {"movement_speed", DroppedItemType::MOVEMENT_SPEED},
        {"shield", DroppedItemType::SHIELD},
        {"key", DroppedItemType::KEY}};

    QString lowerName = name.toLower();
    if (nameMap.contains(lowerName)) {
        return nameMap[lowerName];
    }

    qWarning() << "Unknown item type name:" << name << ", defaulting to RED_HEART";
    return DroppedItemType::RED_HEART;
}

bool DroppedItemFactory::shouldEnemyDropItem() {
    // 5%概率
    return QRandomGenerator::global()->bounded(100) < 5;
}
