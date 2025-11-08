#include "entity.h"

Entity::Entity(QGraphicsPixmapItem *parent)
    : QGraphicsPixmapItem(parent), crash_r(20)
{
    setTransformationMode(Qt::SmoothTransformation);
    damageScale = 1.0;
}

void Entity::setPixmapofDirs(QPixmap& down, QPixmap& up, QPixmap& left, QPixmap& right) {
    this->down = down;
    this->up = up;
    this->left = left;
    this->right = right;
}
