#ifndef USAGI_H
#define USAGI_H

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QObject>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include "../world/levelconfig.h"

class Player;
class Chest;

/**
 * @brief Usagi - 乌萨奇 NPC (Boss奖励机制管理器)
 *
 * Boss被击败后从天而降的奖励NPC，负责整个奖励流程：
 * 1. 从屏幕上方掉落到画面中央
 * 2. 落地后触发恭喜对话
 * 3. 对话结束后消失，生成奖励宝箱
 * 4. 玩家打开所有宝箱后通知门打开
 */
class Usagi : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT

   public:
    /**
     * @brief 创建乌萨奇并启动奖励流程
     * @param scene 游戏场景
     * @param player 玩家对象
     * @param levelNumber 关卡编号（用于生成不同的对话）
     * @param rewardItems Boss奖励道具配置
     * @param parent 父对象
     */
    explicit Usagi(QGraphicsScene* scene, Player* player, int levelNumber, const QVector<BossRewardItem>& rewardItems, QObject* parent = nullptr);
    ~Usagi() override;

    // 开始整个奖励流程
    void startRewardSequence();

   signals:
    // 整个奖励流程完成（所有宝箱打开），门可以打开了
    void rewardSequenceCompleted();

    // 请求显示对话（发送给Level处理）
    void requestShowDialog(const QStringList& dialog);

   public slots:
    // Level的对话结束后调用
    void onDialogFinished();

   private slots:
    void onFallTimer();
    void onDisappearTimer();
    void onChestOpened(Chest* chest);

   private:
    // 内部方法
    void loadUsagiImage();
    void startFalling();
    void onLanded();
    void startDisappearing();
    void spawnRewardChests();
    void checkAllChestsOpened();
    QStringList generateCongratsDialog();

   private:
    QGraphicsScene* m_scene;
    QPointer<Player> m_player;
    int m_levelNumber;
    QVector<BossRewardItem> m_rewardItems;

    QTimer* m_fallTimer;
    QTimer* m_disappearTimer;

    QPointF m_targetPos;    // 目标落地位置
    double m_fallSpeed;     // 下落速度
    bool m_hasLanded;       // 是否已落地
    bool m_isDisappearing;  // 是否正在消失
    double m_opacity;       // 当前透明度

    QVector<QPointer<Chest>> m_rewardChests;  // 奖励宝箱
};

#endif  // USAGI_H
