#ifndef CHEST_H
#define CHEST_H

#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include "item.h"
#include "player.h"
#include "statuseffect.h"

const int open_r = 40; // 打开宝箱的半径，后期可调

/**
 * @brief 宝箱类型枚举
 */
enum class ChestType {
    Normal, // 普通宝箱 - 无需钥匙，使用 chest.png
    Locked, // 高级宝箱 - 需要钥匙，使用 chest_up.png
    Boss    // Boss特供宝箱 - 乌萨奇提供，使用 chest_boss.png
};

/**
 * @brief 宝箱基类
 */
class Chest : public QObject, public QGraphicsPixmapItem {
Q_OBJECT

protected:
    ChestType m_chestType;
    bool m_isOpened;
    QVector<Item *> m_items;
    QTimer *m_checkOpenTimer;
    QPointer<Player> m_player;
    QGraphicsTextItem *m_hintText; // 提示文字
    QTimer *m_hintTimer;           // 提示文字消失定时器

    virtual void initItems();           // 初始化物品列表
    void showHint(const QString &text); // 显示提示文字
    void hideHint();                    // 隐藏提示文字

public:
    Chest(Player *pl, ChestType type, const QPixmap &pic_chest, double scale = 1.0);

    ~Chest() override;

    void addItem(Item *it) { m_items.push_back(it); }

    ChestType getChestType() const { return m_chestType; }

    bool isOpened() const { return m_isOpened; }

    virtual void tryOpen(); // 尝试打开（检查条件）
    virtual void doOpen();  // 执行打开逻辑
    virtual void bonusEffects();

signals:

    void opened(Chest *chest);
};

/**
 * @brief 普通宝箱 - 无需钥匙直接打开
 * 使用图片：chest.png
 */
class NormalChest : public Chest {
Q_OBJECT

protected:
    void initItems() override;

public:
    NormalChest(Player *pl, const QPixmap &pic_chest, double scale = 1.0);
};

/**
 * @brief 高级宝箱（锁住的宝箱） - 需要钥匙打开
 * 使用图片：chest_up.png
 * 没有钥匙时显示提示："你需要1把钥匙来打开它！\n钥匙可以在探索过程中获取"
 */
class LockedChest : public Chest {
Q_OBJECT

protected:
    void initItems() override;

public:
    LockedChest(Player *pl, const QPixmap &pic_chest, double scale = 1.0);

    void tryOpen() override;
};

/**
 * @brief Boss特供宝箱 - 只在boss关的乌萨奇提供
 * 使用图片：chest_boss.png
 * 包含普通宝箱内容 + 必定额外3点血量
 */
class BossChest : public Chest {
Q_OBJECT

protected:
    void initItems() override;

public:
    BossChest(Player *pl, const QPixmap &pic_chest, double scale = 1.0);

    void doOpen() override; // 重写以添加额外3点血量
};

// 保留旧类名的别名，便于兼容
using lockedChest = LockedChest;

#endif // CHEST_H
