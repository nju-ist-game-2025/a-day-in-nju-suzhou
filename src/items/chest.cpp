#include "chest.h"
#include "player.h"
#include <QRandomGenerator>

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

    checkOpen = new QTimer();
    connect(checkOpen, &QTimer::timeout, this, &Chest::open);
    checkOpen->start(16);
}

void Chest::open() {
    foreach (QGraphicsItem* item, scene()->items()) {
        if (auto it = dynamic_cast<Player*>(item)) {
            if (abs(it->pos().x() - this->pos().x()) <= open_r &&
                abs(it->pos().y() - this->pos().y()) <= open_r) {
                int i = QRandomGenerator::global()->bounded(items.size());
                items[i]->onPickup(it);
            }
        }
    }
}
