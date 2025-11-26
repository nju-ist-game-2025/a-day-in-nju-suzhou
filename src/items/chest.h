#ifndef CHEST_H
#define CHEST_H

#include <QGraphicsPixmapItem>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include "item.h"
#include "player.h"
#include "statuseffect.h"

const int open_r = 40; // 打开宝箱的半径，后期可调

class Chest : public QObject, public QGraphicsPixmapItem {
Q_OBJECT
protected:
    bool locked;
    bool isOpened; // 防止重复打开
    QVector<Item *> items;
    QTimer *checkOpen;
    QPointer<Player> player; // 使用QPointer保护player指针

public:
    Chest(Player *pl, bool locked_, const QPixmap &pic_chest, double scale = 1.0);

    ~Chest() override;

    void addItem(Item *it) { items.push_back(it); };

    void open();

    void bonusEffects();

signals:

    void opened(Chest *chest);
};

class lockedChest : public Chest {
public:
    lockedChest(Player *pl, const QPixmap &pic_chest, double scale = 1.0);
};

#endif // CHEST_H
