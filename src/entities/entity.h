#ifndef ENTITY_H
#define ENTITY_H

#include <QGraphicsPixmapItem>

class Entity : public QGraphicsPixmapItem
{
public:
    explicit Entity(QGraphicsItem *parent = nullptr);
};

#endif // ENTITY_H
