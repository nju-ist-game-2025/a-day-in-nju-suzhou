#ifndef DROPPEDITEM_H
#define DROPPEDITEM_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>

class Player;
class QGraphicsScene;

/**
 * @brief 道具类型枚举
 */
enum class DroppedItemType {
    RED_HEART,        // 红心 - 增加1点血量（若已满则不增加）
    BLACK_HEART,      // 黑心 - 复活用
    BLOOD_BAG,        // 血袋 - 增加2点血量上限和2点当前血量
    DAMAGE_BOOST,     // 伤害提升 - 子弹伤害+1
    FIRE_RATE_BOOST,  // 射速提升 - 射速x1.5（上限6倍）
    FROST_SLOWDOWN,   // 冰冻减速 - 增加20%寒冰子弹概率
    MOVEMENT_SPEED,   // 移动速度 - 移速+20%（上限3倍）
    SHIELD,           // 护盾 - 增加一个护盾
    KEY,              // 钥匙
    TICKET            // 车票 - 通关奖励
};

/**
 * @brief 掉落道具实体类
 *
 * 道具从敌人或宝箱中掉落后，在场景中显示对应图片。
 * 玩家接触道具后触发拾取效果。
 */
class DroppedItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

   public:
    /**
     * @brief 构造函数
     * @param type 道具类型
     * @param pos 初始位置
     * @param player 玩家引用
     * @param parent 父对象
     */
    explicit DroppedItem(DroppedItemType type, const QPointF& pos, Player* player, QObject* parent = nullptr);

    ~DroppedItem() override;

    /**
     * @brief 获取道具类型
     */
    DroppedItemType getType() const { return m_type; }

    /**
     * @brief 获取道具名称（用于显示提示）
     */
    QString getItemName() const;

    /**
     * @brief 设置散落动画的目标位置
     * @param targetPos 目标位置
     */
    void setScatterTarget(const QPointF& targetPos);

    /**
     * @brief 暂停道具（用于游戏暂停）
     */
    void setPaused(bool paused);

   signals:
    /**
     * @brief 车票拾取信号（触发通关动画）
     */
    void ticketPickedUp();

   private slots:
    /**
     * @brief 检测与玩家的碰撞
     */
    void checkPlayerCollision();

    /**
     * @brief 延迟结束后允许拾取
     */
    void enablePickup();

    /**
     * @brief 拾取动画完成后应用效果
     */
    void onPickupAnimationFinished();

   private:
    /**
     * @brief 加载道具图片
     */
    void loadItemPixmap();

    /**
     * @brief 应用道具效果
     */
    void applyEffect();

    /**
     * @brief 获取道具的配置key
     */
    QString getItemConfigKey() const;

    /**
     * @brief 显示拾取提示文字
     * @param text 提示文字
     * @param color 文字颜色
     */
    void showPickupText(const QString& text, const QColor& color = Qt::white);

    /**
     * @brief 开始拾取动画
     */
    void startPickupAnimation();

    /**
     * @brief 开始散落动画
     */
    void startScatterAnimation();

    DroppedItemType m_type;      // 道具类型
    QPointer<Player> m_player;   // 玩家引用
    QTimer* m_collisionTimer;    // 碰撞检测定时器
    QTimer* m_pickupDelayTimer;  // 拾取延迟定时器
    bool m_canPickup;            // 是否可以拾取
    bool m_isPickingUp;          // 是否正在拾取中
    bool m_isPaused;             // 是否暂停
    QPointF m_scatterTarget;     // 散落目标位置
    bool m_hasScatterTarget;     // 是否有散落目标

    // 道具图片路径映射
    static QString getItemImagePath(DroppedItemType type);
};

#endif  // DROPPEDITEM_H
