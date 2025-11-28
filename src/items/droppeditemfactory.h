#ifndef DROPPEDITEMFACTORY_H
#define DROPPEDITEMFACTORY_H

#include <QString>
#include <QVector>
#include "droppeditem.h"

class Player;
class QGraphicsScene;

/**
 * @brief 掉落物品类型分组
 */
enum class ItemDropPool {
    NORMAL_CHEST,  // 普通宝箱物品池
    LOCKED_CHEST,  // 需要钥匙的宝箱物品池
    ENEMY_DROP,    // 敌人掉落物品池
    BOSS_CHEST     // Boss宝箱物品池（自定义）
};

/**
 * @brief 掉落物品工厂类
 *
 * 负责管理不同来源的道具掉落逻辑
 */
class DroppedItemFactory {
   public:
    /**
     * @brief 从指定物品池获取随机物品类型
     * @param pool 物品池类型
     * @return 随机物品类型
     */
    static DroppedItemType getRandomItemType(ItemDropPool pool);

    /**
     * @brief 在场景中创建掉落物品
     * @param type 物品类型
     * @param pos 位置
     * @param player 玩家引用
     * @param scene 场景
     * @return 创建的掉落物品指针
     */
    static DroppedItem* createDroppedItem(DroppedItemType type, const QPointF& pos, Player* player, QGraphicsScene* scene);

    /**
     * @brief 从指定位置掉落随机物品（使用指定物品池）
     * @param pool 物品池
     * @param pos 位置
     * @param player 玩家引用
     * @param scene 场景
     * @return 创建的掉落物品指针
     */
    static DroppedItem* dropRandomItem(ItemDropPool pool, const QPointF& pos, Player* player, QGraphicsScene* scene);

    /**
     * @brief 从指定位置掉落多个物品（散开效果）
     * @param pool 物品池
     * @param pos 中心位置
     * @param count 物品数量
     * @param player 玩家引用
     * @param scene 场景
     */
    static void dropItemsScattered(ItemDropPool pool, const QPointF& pos, int count, Player* player, QGraphicsScene* scene);

    /**
     * @brief 掉落指定类型的物品列表（用于自定义宝箱）
     * @param types 物品类型列表
     * @param pos 中心位置
     * @param player 玩家引用
     * @param scene 场景
     * @return 创建的物品列表
     */
    static QVector<DroppedItem*> dropSpecificItems(const QVector<DroppedItemType>& types, const QPointF& pos, Player* player, QGraphicsScene* scene);

    /**
     * @brief 根据字符串名称获取物品类型
     * @param name 物品名称（如 "black_heart", "red_heart" 等）
     * @return 物品类型，未知名称返回 RED_HEART
     */
    static DroppedItemType getItemTypeFromName(const QString& name);

    /**
     * @brief 检查敌人死亡是否应该掉落物品（5%概率）
     * @return 是否应该掉落
     */
    static bool shouldEnemyDropItem();

   private:
    // 不同物品池的物品类型和权重
    static QVector<QPair<DroppedItemType, int>> getNormalChestPool();
    static QVector<QPair<DroppedItemType, int>> getLockedChestPool();
    static QVector<QPair<DroppedItemType, int>> getEnemyDropPool();

    // 根据权重随机选择物品
    static DroppedItemType selectByWeight(const QVector<QPair<DroppedItemType, int>>& pool);
};

#endif  // DROPPEDITEMFACTORY_H
