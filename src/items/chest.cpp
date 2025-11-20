#include "chest.h"
#include "player.h"
#include <QRandomGenerator>
#include <QApplication>

Chest::Chest(Player* pl, bool locked_, const QPixmap& pic_chest, double scale) :locked(locked_), player(pl){
    if (scale == 1.0) {
        this->setPixmap(pic_chest);
    } else {
        this->setPixmap(pic_chest.scaled(
            pic_chest.width() * scale,
            pic_chest.height() * scale,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    }

    DamageUpItem* dam = new DamageUpItem("", 1.05);
    items.push_back(dam);
    SpeedUpItem* spd = new SpeedUpItem("", 1.05);
    items.push_back(spd);
    ShootSpeedUpItem *shootspd = new ShootSpeedUpItem("", 1.05);
    items.push_back(shootspd);
    BulletSpeedUpItem *bltspd = new BulletSpeedUpItem("", 1.05);
    items.push_back(bltspd);
    RedHeartContainerItem *redheart = new RedHeartContainerItem("", 1);
    items.push_back(redheart);
    //BrimstoneItem *brim = new BrimstoneItem("");
    //items.push_back(brim);
    BombItem *bmb = new BombItem("", 1);
    items.push_back(bmb);
    KeyItem *key1 = new KeyItem("", 1);
    KeyItem *key2 = new KeyItem("", 2);
    items.push_back(key1);
    items.push_back(key2);

    checkOpen = new QTimer();
    connect(checkOpen, &QTimer::timeout, this, &Chest::open);
    checkOpen->start(16);
}

#include <QKeyEvent>

void Chest::open() {
    // 检查空格键是否被按下
        foreach (QGraphicsItem* item, scene()->items()) {
            if (auto player = dynamic_cast<Player*>(item)) {
                if (player->keysPressed[Qt::Key_Space] &&
                    abs(player->pos().x() - this->pos().x()) <= open_r &&
                    abs(player->pos().y() - this->pos().y()) <= open_r) {

                    if (locked) {
                        if (player->getKeys() > 0) {
                            player->addKeys(-1);
                            locked = false;
                        } else {
                            return;
                        }
                    }

                    qDebug() << "宝箱被打开";
                    int i = QRandomGenerator::global()->bounded(items.size());
                    items[i]->onPickup(player);
                    scene()->removeItem(this);
                    checkOpen->stop();
                    //delete this;
                    return;
                }
            }
        }
}

lockedChest::lockedChest(Player* pl, const QPixmap& pic_chest, double scale) :
    Chest(pl, true, pic_chest, scale)
{
    items.clear();

    //锁住的箱子奖励提高
    DamageUpItem* dam = new DamageUpItem("", 1.1);
    items.push_back(dam);
    SpeedUpItem* spd = new SpeedUpItem("", 1.1);
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
