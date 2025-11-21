#ifndef CHEST_H
#define CHEST_H

#include <QGraphicsPixmapItem>
#include <QVector>
#include <QTimer>
#include "player.h"
#include "item.h"
#include "statuseffect.h"

const int open_r = 40;//打开宝箱的半径，后期可调

class Chest : public QObject, public QGraphicsPixmapItem {
protected:
    bool locked;
    QVector<Item *> items;
    QTimer *checkOpen;
    Player *player;
public:
    Chest(Player *pl, bool locked_, const QPixmap &pic_chest, double scale = 1.0);

    void addItem(Item *it) { items.push_back(it); };

    void open();

    void bonusEffects();
};

class lockedChest : public Chest {
public:
    lockedChest(Player *pl, const QPixmap &pic_chest, double scale = 1.0);
};

#endif // CHEST_H
