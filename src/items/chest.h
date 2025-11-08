#ifndef CHEST_H
#define CHEST_H

#include <QGraphicsPixmapItem>
#include <QVector>
#include <QTimer>
#include "player.h"
#include "item.h"

const int open_r = 40;//打开宝箱的半径，后期可调

class Chest : public QObject, public QGraphicsPixmapItem
{
    bool locked;
    QVector<Item*> items;
    QTimer* checkOpen;
    Player* player;
public:
    Chest(Player* pl, bool locked_, const QPixmap& pic_chest, double scale = 1.0);
    void addItem(Item* it) {items.push_back(it);};
    void open();
};

#endif // CHEST_H
