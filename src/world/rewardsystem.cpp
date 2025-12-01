#include "rewardsystem.h"
#include <QDebug>
#include <QGraphicsScene>
#include "../entities/player.h"
#include "../entities/usagi.h"
#include "../items/droppeditem.h"
#include "../items/droppeditemfactory.h"

RewardSystem::RewardSystem(Player* player, QGraphicsScene* scene, QObject* parent)
    : QObject(parent), m_player(player), m_scene(scene) {
}

RewardSystem::~RewardSystem() {
    cleanup();
}

void RewardSystem::cleanup() {
    m_usagi = nullptr;
    m_rewardSequenceActive = false;
    m_bossRoomCleared = false;
    m_gKeyEnabled = false;
}

// ==================== 物品掉落 ====================

void RewardSystem::dropRandomItem(const QPointF& position) {
    if (!m_scene || !m_player)
        return;

    DroppedItemFactory::dropRandomItem(ItemDropPool::ENEMY_DROP, position, m_player, m_scene);
    qDebug() << "[RewardSystem] 在位置" << position << "掉落随机物品";
}

void RewardSystem::dropItemsFromPosition(const QPointF& position, int count, bool scatter) {
    if (!m_scene || !m_player || count <= 0)
        return;

    Q_UNUSED(scatter)  // 工厂类自动处理散开效果
    DroppedItemFactory::dropItemsScattered(ItemDropPool::ENEMY_DROP, position, count, m_player, m_scene);
    qDebug() << "[RewardSystem] 从位置" << position << "掉落" << count << "个物品";
}

bool RewardSystem::shouldEnemyDropItem() {
    return DroppedItemFactory::shouldEnemyDropItem();
}

// ==================== Boss奖励 ====================

void RewardSystem::startBossRewardSequence(int levelNumber, const QStringList& chestItems) {
    if (m_rewardSequenceActive)
        return;

    m_rewardSequenceActive = true;
    m_currentLevel = levelNumber;
    qDebug() << "[RewardSystem] 开始Boss奖励流程，关卡:" << levelNumber;

    // 创建乌萨奇
    m_usagi = new Usagi(m_scene, m_player, levelNumber, chestItems, this);

    // 连接信号
    connect(m_usagi, &Usagi::requestShowDialog, this, &RewardSystem::onUsagiRequestShowDialog);
    connect(m_usagi, &Usagi::rewardSequenceCompleted, this, &RewardSystem::onUsagiRewardCompleted);

    // 连接车票创建信号（第三关）
    connect(m_usagi, &Usagi::ticketCreated, this, [this](DroppedItem* ticket) {
        qDebug() << "[RewardSystem] 收到Usagi的ticketCreated信号";
        if (ticket) {
            connect(ticket, &DroppedItem::ticketPickedUp, this, [this]() {
                qDebug() << "[RewardSystem] 车票被拾取！";
                emit ticketPickedUp();
            });
            qDebug() << "[RewardSystem] 已连接车票的ticketPickedUp信号";
        }
    });

    // 启动奖励流程
    m_usagi->startRewardSequence();
}

void RewardSystem::onUsagiRequestShowDialog(const QStringList& dialog) {
    qDebug() << "[RewardSystem] 乌萨奇请求显示对话";
    emit requestShowDialog(dialog);
}

void RewardSystem::onUsagiRewardCompleted() {
    qDebug() << "[RewardSystem] 乌萨奇奖励流程完成";

    // 标记Boss房间已通关
    m_bossRoomCleared = true;

    // 第三关不激活G键（因为拾取车票后会触发通关动画）
    if (m_currentLevel != 3) {
        enableGKey();
        qDebug() << "[RewardSystem] G键已激活，等待玩家按G进入下一关";
    } else {
        qDebug() << "[RewardSystem] 第三关不激活G键，等待玩家拾取车票";
    }

    m_rewardSequenceActive = false;
    m_usagi = nullptr;

    emit rewardSequenceCompleted();
}

// ==================== G键控制 ====================

void RewardSystem::enableGKey() {
    m_gKeyEnabled = true;
    emit gKeyEnabled();
    emit requestShowGKeyHint();
}

void RewardSystem::disableGKey() {
    m_gKeyEnabled = false;
}

void RewardSystem::onDialogFinished() {
    if (m_usagi) {
        qDebug() << "[RewardSystem] 通知Usagi对话已结束";
        m_usagi->onDialogFinished();
    }
}
