#include "entity.h"
#include <QPainter>
#include <QTimer>

void Entity::flash() {
    QPixmap originalPixmap = pixmap();
    QPixmap flashPixmap = originalPixmap;
    QPainter painter(&flashPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(flashPixmap.rect(), QColor(255, 0, 0, 180));  // 红色半透明
    painter.end();
    setPixmap(flashPixmap);
    QTimer::singleShot(120, this, [this, originalPixmap]() { setPixmap(originalPixmap); });
}

void Entity::takeDamage(int damage) {
    // 默认实现仅闪烁
    flash();
}
#include "entity.h"

Entity::Entity(QGraphicsPixmapItem* parent)
    : QGraphicsPixmapItem(parent), crash_r(20) {
    setTransformationMode(Qt::SmoothTransformation);
    damageScale = 1.0;
}

void Entity::setPixmapofDirs(QPixmap& down, QPixmap& up, QPixmap& left, QPixmap& right) {
    this->down = down;
    this->up = up;
    this->left = left;
    this->right = right;
}
