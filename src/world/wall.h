
#pragma once

#include <QGraphicsRectItem>
#include <QBrush>
#include <QPen>

class Wall : public QGraphicsRectItem {
public:
    explicit Wall(const QRectF &rect, QGraphicsItem *parent = nullptr)
            : QGraphicsRectItem(rect, parent) {
        setBrush(QBrush(Qt::gray));
        setPen(QPen(Qt::black));
        setOpacity(1.0);
        setZValue(50);
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemIsFocusable, false);
        setFlag(QGraphicsItem::ItemIsMovable, false);
    }
};
