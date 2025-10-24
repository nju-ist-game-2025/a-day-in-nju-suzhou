#include "entity.h"

Entity::Entity(QGraphicsPixmapItem *parent)
    : QGraphicsPixmapItem(parent)
{
    setTransformationMode(Qt::SmoothTransformation);
}

void Entity::setPixmapofDirs(QPixmap& down, QPixmap& up, QPixmap& left, QPixmap& right) {
    this->down = down;
    this->up = up;
    this->left = left;
    this->right = right;
}
