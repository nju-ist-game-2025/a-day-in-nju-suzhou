#include "entity.h"
#include <QPainter>
#include <QTimer>

void Entity::flash() {
    // 如果已经在闪烁中，不重复执行
    if (isFlashing) return;

    isFlashing = true;

    // 保存原始图片
    originalPixmap = pixmap();

    QPixmap flashPixmap = originalPixmap;
    QPainter painter(&flashPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(flashPixmap.rect(), QColor(255, 0, 0, 180));  // 红色半透明
    painter.end();
    setPixmap(flashPixmap);

    // 使用单次定时器恢复原始图片
    QTimer::singleShot(120, this, [this]() {
        setPixmap(originalPixmap);
        isFlashing = false;
    });
}

void Entity::takeDamage(int damage) {
    // 默认实现仅闪烁
    flash();
}

#include "entity.h"

Entity::Entity(QGraphicsPixmapItem *parent)
        : QGraphicsPixmapItem(parent), crash_r(20), isFlashing(false) {
    setTransformationMode(Qt::SmoothTransformation);
    damageScale = 1.0;
}

void Entity::setPixmapofDirs(QPixmap &down, QPixmap &up, QPixmap &left, QPixmap &right) {
    this->down = down;
    this->up = up;
    this->left = left;
    this->right = right;
}
