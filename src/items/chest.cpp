#include "chest.h"
#include <QApplication>
#include <QRandomGenerator>
#include "../core/audiomanager.h"
#include "player.h"

Chest::Chest(Player *pl, bool locked_, const QPixmap &pic_chest, double scale) : locked(locked_), isOpened(false), player(pl)
{
    if (scale == 1.0)
    {
        this->setPixmap(pic_chest);
    }
    else
    {
        this->setPixmap(pic_chest.scaled(
            pic_chest.width() * scale,
            pic_chest.height() * scale,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    }

    DamageUpItem *dam = new DamageUpItem("", 1.05);
    items.push_back(dam);
    SpeedUpItem *spd = new SpeedUpItem("", 1.05);
    items.push_back(spd);
    ShootSpeedUpItem *shootspd = new ShootSpeedUpItem("", 1.05);
    items.push_back(shootspd);
    BulletSpeedUpItem *bltspd = new BulletSpeedUpItem("", 1.05);
    items.push_back(bltspd);
    RedHeartContainerItem *redheart1 = new RedHeartContainerItem("", 1);
    RedHeartContainerItem *redheart2 = new RedHeartContainerItem("", 1);
    items.push_back(redheart1);
    items.push_back(redheart2);
    // BrimstoneItem *brim = new BrimstoneItem("");
    // items.push_back(brim);
    BombItem *bmb = new BombItem("", 1);
    items.push_back(bmb);
    KeyItem *key1 = new KeyItem("", 1);
    KeyItem *key2 = new KeyItem("", 2);
    items.push_back(key1);
    items.push_back(key2);

    checkOpen = new QTimer(this);
    connect(checkOpen, &QTimer::timeout, this, &Chest::open);
    checkOpen->start(16);
}

Chest::~Chest()
{
    if (checkOpen)
    {
        checkOpen->stop();
        disconnect(checkOpen, nullptr, this, nullptr);
    }
}

#include <QKeyEvent>

void Chest::open()
{
    // 如果已经打开过，不再处理
    if (isOpened)
    {
        return;
    }

    if (!scene() || !player)
    {
        return;
    }

    // 检查玩家是否在范围内并按下空格键
    if (player->keysPressed[Qt::Key_Space] &&
        abs(player->pos().x() - this->pos().x()) <= open_r &&
        abs(player->pos().y() - this->pos().y()) <= open_r)
    {
        if (locked)
        {
            if (player->getKeys() > 0)
            {
                player->addKeys(-1);
                locked = false;
            }
            else
            {
                return;
            }
        }

        // 标记为已打开
        isOpened = true;

        emit opened(this);

        // 立即停止并断开定时器，防止重复触发
        if (checkOpen)
        {
            checkOpen->stop();
            disconnect(checkOpen, nullptr, this, nullptr);
        }

        AudioManager::instance().playSound("chest_open");
        qDebug() << "宝箱开启音效已触发";
        qDebug() << "宝箱被打开";

        // 保存player的QPointer副本
        QPointer<Player> playerPtr = player;

        int i = QRandomGenerator::global()->bounded(items.size());
        if (playerPtr)
        {
            items[i]->onPickup(playerPtr.data());
        }

        // 先应用效果，再删除对象
        if (playerPtr)
        {
            bonusEffects();
        }

        // 从场景移除
        if (scene())
        {
            scene()->removeItem(this);
        }

        // 延迟删除
        deleteLater();
    }
}

void Chest::bonusEffects()
{
    // 检查player是否仍然有效
    if (!player || !player->scene())
    {
        return;
    }

    // 获取原始指针用于创建效果
    Player *pl = player.data();

    QVector<StatusEffect *> effectstoPlayer;

    SpeedEffect *sp = new SpeedEffect(5, 1.5);
    effectstoPlayer.push_back(sp);
    DamageEffect *dam = new DamageEffect(5, 1.5);
    effectstoPlayer.push_back(dam);
    shootSpeedEffect *shootsp = new shootSpeedEffect(5, 1.5);
    effectstoPlayer.push_back(shootsp);
    blackHeartEffect *black1 = new blackHeartEffect(pl, 1);
    blackHeartEffect *black2 = new blackHeartEffect(pl, 1);
    effectstoPlayer.push_back(black1);
    effectstoPlayer.push_back(black2);
    decDamage *dec = new decDamage(5, 0.5);
    effectstoPlayer.push_back(dec);
    InvincibleEffect *inv = new InvincibleEffect(5);
    effectstoPlayer.push_back(inv);

    // 以1/3的概率获得额外增益效果
    int i = QRandomGenerator::global()->bounded(effectstoPlayer.size() * 3);
    if (i >= 0 && i < effectstoPlayer.size() && effectstoPlayer[i])
    {
        // 应用前再次检查player
        if (player)
        {
            effectstoPlayer[i]->applyTo(player.data());
        }
        for (StatusEffect *effect : effectstoPlayer)
        {
            if (effect != effectstoPlayer[i])
            { // 只删除未使用的
                effect->deleteLater();
            }
        }
    }
    else
    {
        // 如果没有选中任何效果，清理所有效果
        for (StatusEffect *effect : effectstoPlayer)
        {
            effect->deleteLater();
        }
    }
}

lockedChest::lockedChest(Player *pl, const QPixmap &pic_chest, double scale) : Chest(pl, true, pic_chest, scale)
{
    items.clear();

    // 锁住的箱子奖励提高
    DamageUpItem *dam = new DamageUpItem("", 1.1);
    items.push_back(dam);
    SpeedUpItem *spd = new SpeedUpItem("", 1.1);
    items.push_back(spd);
    ShootSpeedUpItem *shootspd = new ShootSpeedUpItem("", 1.1);
    items.push_back(shootspd);
    BulletSpeedUpItem *bltspd = new BulletSpeedUpItem("", 1.1);
    items.push_back(bltspd);
    RedHeartContainerItem *redheart1 = new RedHeartContainerItem("", 2);
    items.push_back(redheart1);
    BrimstoneItem *brim = new BrimstoneItem("");
    items.push_back(brim);
    BombItem *bmb = new BombItem("", 2);
    items.push_back(bmb);
}
