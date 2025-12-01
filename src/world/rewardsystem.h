#ifndef REWARDSYSTEM_H
#define REWARDSYSTEM_H

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QStringList>

class Player;
class Usagi;
class DroppedItem;
class QGraphicsScene;

/**
 * @brief 奖励系统类 - 封装物品掉落、Boss奖励、宝箱等逻辑
 *
 * 职责：
 *   - 敌人死亡掉落物品
 *   - Boss击败后的奖励流程（乌萨奇）
 *   - 宝箱物品生成
 *   - 车票拾取（通关物品）
 */
class RewardSystem : public QObject {
    Q_OBJECT

   public:
    explicit RewardSystem(Player* player, QGraphicsScene* scene, QObject* parent = nullptr);
    ~RewardSystem() override;

    /**
     * @brief 清理资源
     */
    void cleanup();

    // ==================== 物品掉落 ====================

    /**
     * @brief 在指定位置掉落随机物品
     * @param position 掉落位置
     */
    void dropRandomItem(const QPointF& position);

    /**
     * @brief 从指定位置掉落多个物品（散开效果）
     * @param position 中心位置
     * @param count 物品数量
     * @param scatter 是否散开
     */
    void dropItemsFromPosition(const QPointF& position, int count, bool scatter = true);

    /**
     * @brief 检查敌人是否应该掉落物品
     * @return 是否掉落
     */
    static bool shouldEnemyDropItem();

    // ==================== Boss奖励 ====================

    /**
     * @brief 启动Boss奖励流程
     * @param levelNumber 关卡号
     * @param chestItems 宝箱物品列表
     */
    void startBossRewardSequence(int levelNumber, const QStringList& chestItems);

    /**
     * @brief 检查奖励流程是否激活
     */
    bool isRewardSequenceActive() const { return m_rewardSequenceActive; }

    /**
     * @brief 检查Boss房间是否已通关
     */
    bool isBossRoomCleared() const { return m_bossRoomCleared; }

    /**
     * @brief 设置Boss房间已通关
     */
    void setBossRoomCleared(bool cleared) { m_bossRoomCleared = cleared; }

    // ==================== G键控制 ====================

    /**
     * @brief 检查G键是否激活
     */
    bool isGKeyEnabled() const { return m_gKeyEnabled; }

    /**
     * @brief 启用G键
     */
    void enableGKey();

    /**
     * @brief 禁用G键
     */
    void disableGKey();

    /**
     * @brief 对话结束时调用（通知Usagi）
     */
    void onDialogFinished();

   signals:
    /**
     * @brief 请求显示对话
     */
    void requestShowDialog(const QStringList& dialog);

    /**
     * @brief 奖励流程完成
     */
    void rewardSequenceCompleted();

    /**
     * @brief G键激活信号
     */
    void gKeyEnabled();

    /**
     * @brief 车票拾取信号（通关）
     */
    void ticketPickedUp();

    /**
     * @brief 请求显示G键提示
     */
    void requestShowGKeyHint();

   private slots:
    /**
     * @brief 乌萨奇请求显示对话
     */
    void onUsagiRequestShowDialog(const QStringList& dialog);

    /**
     * @brief 乌萨奇奖励完成
     */
    void onUsagiRewardCompleted();

   private:
    Player* m_player = nullptr;
    QGraphicsScene* m_scene = nullptr;

    // Boss奖励相关
    QPointer<Usagi> m_usagi;
    bool m_rewardSequenceActive = false;
    bool m_bossRoomCleared = false;

    // G键控制
    bool m_gKeyEnabled = false;

    // 当前关卡号（用于奖励流程）
    int m_currentLevel = 0;
};

#endif  // REWARDSYSTEM_H
