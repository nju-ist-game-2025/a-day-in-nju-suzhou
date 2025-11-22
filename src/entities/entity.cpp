#include "entity.h"
#include <QPainter>
#include <QTimer>

Entity::Entity(QGraphicsPixmapItem *parent)
        : QGraphicsPixmapItem(parent), crash_r(20), isFlashing(false) {
    setTransformationMode(Qt::SmoothTransformation);
    damageScale = 1.0;
    facingRight = true;
    flippingInProgress = false;
}

void Entity::setPixmap(const QPixmap &pix) {
    // 直接使用基类实现；player/enemy 可能直接调用这个接口
    QGraphicsPixmapItem::setPixmap(pix);
}

void Entity::setPixmapofDirs(QPixmap &downImg, QPixmap &upImg, QPixmap &leftImg, QPixmap &rightImg) {
    // 保存传入的四方向图，兼容旧代码
    down = downImg;
    up = upImg;
    right = rightImg;
    left = leftImg;

    // 如果 left 未提供但 right 提供，则自动生成 left（水平翻转）
    if (left.isNull() && !right.isNull()) {
        left = right.transformed(QTransform().scale(-1, 1));
    }

    // 如果目前没有 pixmap，优先设置为 right（假定资源朝右）
    if (!right.isNull() && pixmap().isNull()) {
        QGraphicsPixmapItem::setPixmap(right);
        facingRight = true;
    }
}

void Entity::updateFacing() {
    // 防止在翻转过程中递归触发
    if (flippingInProgress) return;

    // xdir 由 player/enemy 维护；当 xdir < 0 时应面向左，>0 时面向右
    if (xdir < 0 && facingRight) {
        // 需要从“当前图”翻转为左向
        if (!pixmap().isNull()) {
            flippingInProgress = true;
            QPixmap cur = pixmap();
            QPixmap flipped = cur.transformed(QTransform().scale(-1, 1));
            QGraphicsPixmapItem::setPixmap(flipped);
            facingRight = false;
            flippingInProgress = false;
        } else {
            // pixmap 为空时，尝试使用 left/right 资源
            if (!left.isNull()) {
                QGraphicsPixmapItem::setPixmap(left);
                facingRight = false;
            } else if (!right.isNull()) {
                QGraphicsPixmapItem::setPixmap(right.transformed(QTransform().scale(-1, 1)));
                facingRight = false;
            }
        }
    }
    else if (xdir > 0 && !facingRight) {
        // 需要从“当前图”翻转为右向
        if (!pixmap().isNull()) {
            flippingInProgress = true;
            QPixmap cur = pixmap();
            QPixmap flipped = cur.transformed(QTransform().scale(-1, 1));
            QGraphicsPixmapItem::setPixmap(flipped);
            facingRight = true;
            flippingInProgress = false;
        } else {
            // pixmap 为空时，尝试使用 right/left 资源
            if (!right.isNull()) {
                QGraphicsPixmapItem::setPixmap(right);
                facingRight = true;
            } else if (!left.isNull()) {
                QGraphicsPixmapItem::setPixmap(left.transformed(QTransform().scale(-1, 1)));
                facingRight = true;
            }
        }
    }
    // xdir == 0 不改变面朝（保持当前 facingRight）
}

void Entity::setPos(qreal x, qreal y) {
    // 在移动之前先更新朝向（这样 player/enemy 不用改）
    updateFacing();
    // 调用基类方法实际移动位置
    QGraphicsItem::setPos(x, y);
}

void Entity::setPos(const QPointF &pos) {
    updateFacing();
    QGraphicsItem::setPos(pos);
}

void Entity::flash() {
    if (isFlashing) return;

    isFlashing = true;

    QPixmap original = pixmap();  // 当前图像（可能已翻转）
    QPixmap flashPixmap = original;

    QPainter painter(&flashPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(flashPixmap.rect(), QColor(255, 0, 0, 180));
    painter.end();

    QGraphicsPixmapItem::setPixmap(flashPixmap);

    QTimer::singleShot(120, this, [this, original]() {
        QGraphicsPixmapItem::setPixmap(original);
        isFlashing = false;
    });
}

void Entity::takeDamage(int damage) {
    flash();
}
