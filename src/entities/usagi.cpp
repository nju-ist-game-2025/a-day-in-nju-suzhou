#include "usagi.h"
#include <QDebug>
#include <QFile>
#include <QGraphicsOpacityEffect>
#include <QGraphicsScene>
#include "../core/resourcefactory.h"
#include "../items/chest.h"
#include "../items/droppeditemfactory.h"
#include "../items/item.h"
#include "player.h"

Usagi::Usagi(QGraphicsScene* scene, Player* player, int levelNumber, const QStringList& usagiChestItems, QObject* parent)
    : QObject(parent),
      QGraphicsPixmapItem(),
      m_scene(scene),
      m_player(player),
      m_levelNumber(levelNumber),
      m_usagiChestItems(usagiChestItems),
      m_fallTimer(nullptr),
      m_disappearTimer(nullptr),
      m_fallSpeed(5.0),
      m_hasLanded(false),
      m_isDisappearing(false),
      m_opacity(1.0) {
    setZValue(100);  // 显示在最上层

    // 加载乌萨奇图片
    loadUsagiImage();

    // 设置目标位置（画面中央）
    m_targetPos = QPointF(400 - pixmap().width() / 2, 300 - pixmap().height() / 2);

    // 设置初始位置（屏幕上方）
    setPos(m_targetPos.x(), -100);

    qDebug() << "[Usagi] 创建乌萨奇，目标位置:" << m_targetPos;
}

Usagi::~Usagi() {
    if (m_fallTimer) {
        m_fallTimer->stop();
        delete m_fallTimer;
    }
    if (m_disappearTimer) {
        m_disappearTimer->stop();
        delete m_disappearTimer;
    }
    qDebug() << "[Usagi] 乌萨奇已销毁";
}

void Usagi::loadUsagiImage() {
    QPixmap usagiPix;
    QStringList possiblePaths = {
        "assets/usagi/usagi.png",
        "../assets/usagi/usagi.png",
        "../../our_game/assets/usagi/usagi.png"};

    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            usagiPix = QPixmap(path);
            break;
        }
    }

    if (usagiPix.isNull()) {
        // 使用默认的灰色方块作为占位符
        usagiPix = QPixmap(80, 80);
        usagiPix.fill(Qt::magenta);
        qWarning() << "[Usagi] 无法加载乌萨奇图片，使用占位符";
    } else {
        // 缩放乌萨奇图片
        usagiPix = usagiPix.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    setPixmap(usagiPix);
}

void Usagi::startRewardSequence() {
    qDebug() << "[Usagi] 开始Boss奖励流程";

    // 添加到场景
    if (m_scene) {
        m_scene->addItem(this);
    }

    // 开始下落
    startFalling();
}

void Usagi::startFalling() {
    if (m_hasLanded)
        return;

    qDebug() << "[Usagi] 开始下落动画";

    m_fallTimer = new QTimer(this);
    connect(m_fallTimer, &QTimer::timeout, this, &Usagi::onFallTimer);
    m_fallTimer->start(16);  // 约60fps
}

void Usagi::onFallTimer() {
    if (m_hasLanded) {
        if (m_fallTimer) {
            m_fallTimer->stop();
        }
        return;
    }

    QPointF currentPos = pos();
    double newY = currentPos.y() + m_fallSpeed;

    // 检查是否到达目标位置
    if (newY >= m_targetPos.y()) {
        newY = m_targetPos.y();
        m_hasLanded = true;

        if (m_fallTimer) {
            m_fallTimer->stop();
        }

        qDebug() << "[Usagi] 乌萨奇已落地";
        onLanded();
    }

    setPos(currentPos.x(), newY);
}

void Usagi::onLanded() {
    qDebug() << "[Usagi] 显示恭喜对话";

    // 生成对话并请求Level显示
    QStringList dialog = generateCongratsDialog();
    emit requestShowDialog(dialog);
}

QStringList Usagi::generateCongratsDialog() {
    QStringList dialog;

    if (m_levelNumber == 1) {
        dialog = {
            "【乌萨奇】\n『哇哦！恭喜你！』",
            "【乌萨奇】\n『你成功打败了梦魇！』",
            "【乌萨奇】\n『看来你已经准备好面对新的一天了呢～』",
            "「智科er」 \n 你...你是谁？",
            "【乌萨奇】\n『我是乌萨奇！是来给你送奖励的！』",
            "【乌萨奇】\n『作为战胜梦魇的奖励，这两个宝箱送给你～』",
            "【乌萨奇】\n『打开它们，然后继续你的旅程吧！』",
            "（乌萨奇消失了，留下了两个宝箱）"};
    } else if (m_levelNumber == 2) {
        dialog = {
            "【乌萨奇】\n『太厉害了！』",
            "【乌萨奇】\n『你成功拯救了洗衣机！』",
            "【乌萨奇】\n『那台可怜的洗衣机终于可以安息了～』",
            "「智科er」 \n 乌萨奇？你又出现了！",
            "【乌萨奇】\n『当然啦！每次你战胜Boss我都会来的！』",
            "【乌萨奇】\n『这是你的奖励～希望这些能帮助你继续前进！』",
            "【乌萨奇】\n『记得要好好爱护公共设施哦～』",
            "（乌萨奇消失了，留下了两个宝箱）"};
    } else if (m_levelNumber == 3) {
        dialog = {
            "【乌萨奇】\n『哇啊啊啊！！！』",
            "【乌萨奇】\n『你居然打败了奶牛张老师！！！』",
            "【乌萨奇】\n『概率论的噩梦终于结束了呢～』",
            "「智科er」 \n 乌萨奇...这次真的太难了...",
            "【乌萨奇】\n『辛苦啦！你真的超级厉害！』",
            "【乌萨奇】\n『根据我的计算，通关概率只有0.01%呢～』",
            "「智科er」 \n 那我岂不是...",
            "【乌萨奇】\n『你就是那个传说中的欧皇！』",
            "【乌萨奇】\n『好啦好啦，这是你应得的奖励～』",
            "【乌萨奇】\n『诶嘿嘿，这次只有一个宝箱哦～』",
            "「智科er」 \n 什么？怎么变少了？",
            "【乌萨奇】\n『因为里面装的东西很特别～』",
            "【乌萨奇】\n『是我亲手为你准备的...一张车票！』",
            "「智科er」 \n 车票？去哪里的车票？",
            "【乌萨奇】\n『目的地嘛...随便你填哪里都行哦～』",
            "【乌萨奇】\n『反正哪怕概率只有0.01%，我也会陪你到的！』",
            "【乌萨奇】\n『好了，去打开它吧！祝你旅途愉快～』",
            "（乌萨奇开心地消失了）"};
    } else {
        dialog = {
            "【乌萨奇】\n『恭喜通关！』",
            "【乌萨奇】\n『你真的很厉害呢！』",
            "【乌萨奇】\n『这是给你的奖励～』",
            "（乌萨奇消失了，留下了两个宝箱）"};
    }

    return dialog;
}

void Usagi::onDialogFinished() {
    qDebug() << "[Usagi] 对话结束，开始消失";
    startDisappearing();
}

void Usagi::startDisappearing() {
    if (m_isDisappearing)
        return;

    m_isDisappearing = true;
    qDebug() << "[Usagi] 开始消失动画";

    m_disappearTimer = new QTimer(this);
    connect(m_disappearTimer, &QTimer::timeout, this, &Usagi::onDisappearTimer);
    m_disappearTimer->start(50);  // 渐隐速度
}

void Usagi::onDisappearTimer() {
    m_opacity -= 0.05;

    if (m_opacity <= 0) {
        m_opacity = 0;
        if (m_disappearTimer) {
            m_disappearTimer->stop();
        }

        qDebug() << "[Usagi] 乌萨奇已消失，生成奖励宝箱";

        // 从场景移除自身
        if (scene()) {
            scene()->removeItem(this);
        }

        // 生成奖励宝箱
        spawnRewardChests();
        return;
    }

    // 设置透明度
    setOpacity(m_opacity);
}

void Usagi::spawnRewardChests() {
    if (!m_player || !m_scene) {
        qWarning() << "[Usagi] 无法生成宝箱：player或scene为空";
        emit rewardSequenceCompleted();
        return;
    }

    // 创建Boss特供宝箱图片
    QPixmap chestPix = ResourceFactory::createBossChestImage(50);
    if (chestPix.isNull()) {
        qWarning() << "[Usagi] 无法创建Boss宝箱图片，跳过奖励";
        emit rewardSequenceCompleted();
        return;
    }

    // 第三关只给一个宝箱，里面是车票
    if (m_levelNumber == 3) {
        QPointF chestPos(400, 300);  // 画面中央

        BossChest* chest = new BossChest(m_player, chestPix, 1.0);
        chest->setPos(chestPos);

        // 设置车票作为唯一物品
        QVector<QString> ticketItem = {"ticket"};
        chest->setCustomItems(ticketItem);

        m_scene->addItem(chest);
        m_rewardChests.append(QPointer<Chest>(chest));

        connect(chest, &Chest::opened, this, &Usagi::onChestOpened);

        // 连接车票掉落信号 - 发送给Level让它直接连接
        connect(chest, &BossChest::ticketDropped, this, [this](DroppedItem* ticket) {
            qDebug() << "[Usagi] 收到宝箱的ticketDropped信号，发送ticketCreated给Level";
            if (ticket) {
                emit ticketCreated(ticket);
            }
        });

        qDebug() << "[Usagi] 第三关Boss特供宝箱已生成（车票）";
    } else {
        // 前两关给两个宝箱
        QPointF chest1Pos(300, 300);
        QPointF chest2Pos(500, 300);

        BossChest* chest1 = new BossChest(m_player, chestPix, 1.0);
        chest1->setPos(chest1Pos);

        BossChest* chest2 = new BossChest(m_player, chestPix, 1.0);
        chest2->setPos(chest2Pos);

        // 使用新的物品掉落系统配置宝箱物品
        if (!m_usagiChestItems.isEmpty()) {
            // 将物品分配到两个宝箱
            QVector<QString> chest1Items, chest2Items;
            for (int i = 0; i < m_usagiChestItems.size(); ++i) {
                if (i % 2 == 0) {
                    chest1Items.append(m_usagiChestItems[i]);
                } else {
                    chest2Items.append(m_usagiChestItems[i]);
                }
            }
            chest1->setCustomItems(chest1Items);
            chest2->setCustomItems(chest2Items);
            qDebug() << "[Usagi] 使用自定义物品配置，宝箱1:" << chest1Items.size() << "个，宝箱2:" << chest2Items.size() << "个";
        } else {
            // 没有配置物品时，使用默认的Boss宝箱掉落池
            qDebug() << "[Usagi] 没有配置自定义物品，使用默认Boss宝箱掉落池";
        }

        // 添加到场景
        m_scene->addItem(chest1);
        m_rewardChests.append(QPointer<Chest>(chest1));

        m_scene->addItem(chest2);
        m_rewardChests.append(QPointer<Chest>(chest2));

        // 连接宝箱打开信号
        connect(chest1, &Chest::opened, this, &Usagi::onChestOpened);
        connect(chest2, &Chest::opened, this, &Usagi::onChestOpened);

        qDebug() << "[Usagi] Boss特供宝箱已生成，共2个";
    }
}

void Usagi::onChestOpened(Chest* chest) {
    qDebug() << "[Usagi] 奖励宝箱被打开";

    // 从列表中移除
    QPointer<Chest> chestPtr(chest);
    m_rewardChests.removeAll(chestPtr);

    // 检查是否所有宝箱都已打开
    checkAllChestsOpened();
}

void Usagi::checkAllChestsOpened() {
    // 清理已失效的指针
    m_rewardChests.erase(
        std::remove_if(m_rewardChests.begin(), m_rewardChests.end(),
                       [](const QPointer<Chest>& ptr) { return ptr.isNull(); }),
        m_rewardChests.end());

    if (m_rewardChests.isEmpty()) {
        qDebug() << "[Usagi] 所有奖励宝箱已打开，通知Level打开门";
        emit rewardSequenceCompleted();

        // 删除自身
        deleteLater();
    } else {
        qDebug() << "[Usagi] 还有" << m_rewardChests.size() << "个奖励宝箱未打开";
    }
}
