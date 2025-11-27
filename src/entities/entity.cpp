#include "entity.h"
#include <QPainter>
#include <QPointer>
#include <QTimer>

Entity::Entity(QGraphicsPixmapItem *parent)
        : QGraphicsPixmapItem(parent), crash_r(20), isFlashing(false), maskNeedsUpdate(true) {
    setTransformationMode(Qt::SmoothTransformation);
    damageScale = 1.0;
    facingRight = true;
    flippingInProgress = false;
}

void Entity::setPixmap(const QPixmap &pix) {
    // 直接使用基类实现；player/enemy 可能直接调用这个接口
    QGraphicsPixmapItem::setPixmap(pix);
    // 标记碰撞掩码需要更新
    maskNeedsUpdate = true;
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
    if (flippingInProgress)
        return;

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
    } else if (xdir > 0 && !facingRight) {
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
    if (isFlashing)
        return;

    isFlashing = true;

    QPixmap original = pixmap();  // 当前图像（可能已翻转）
    QPixmap flashPixmap = original;

    QPainter painter(&flashPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(flashPixmap.rect(), QColor(255, 0, 0, 180));
    painter.end();

    QGraphicsPixmapItem::setPixmap(flashPixmap);

    // 使用QPointer保护this指针，防止在定时器触发前对象被删除
    QPointer<Entity> self = this;
    QTimer::singleShot(120, this, [self, original]() {
        if (self && self->isFlashing) {
            // 恢复到捕获时的图片（保持当前尺寸）
            self->QGraphicsPixmapItem::setPixmap(original);
            self->isFlashing = false;
        }
    });
}

void Entity::cancelFlash() {
    // 立即取消闪烁状态，不恢复图片（因为图片即将被外部更改）
    isFlashing = false;
}

void Entity::takeDamage(int damage) {
    flash();
}

void Entity::generateCollisionMask() {
    QPixmap pix = pixmap();
    if (pix.isNull()) {
        collisionMask = QImage();
        maskNeedsUpdate = false;
        return;
    }

    // 转换为 ARGB32 格式以访问 Alpha 通道
    QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32);

    // 创建单色掩码图像
    collisionMask = QImage(img.size(), QImage::Format_Grayscale8);
    collisionMask.fill(0);

    // 遍历每个像素，检查 Alpha 值
    for (int y = 0; y < img.height(); ++y) {
        const QRgb *scanLine = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        uchar *maskLine = collisionMask.scanLine(y);
        for (int x = 0; x < img.width(); ++x) {
            // 如果 Alpha 大于阈值，标记为不透明（碰撞区域）
            if (qAlpha(scanLine[x]) > alphaThreshold) {
                maskLine[x] = 255;
            }
        }
    }

    maskNeedsUpdate = false;
}

const QImage &Entity::getCollisionMask() {
    // 惰性生成：只在需要且掩码过期时生成
    if (maskNeedsUpdate || collisionMask.isNull()) {
        generateCollisionMask();
    }
    return collisionMask;
}

QRectF Entity::pixmapSceneBoundingRect() const {
    // 获取实际pixmap的边界框（不受boundingRect()重写影响，如Boss的血条扩展）
    QPixmap pix = pixmap();
    if (pix.isNull()) {
        return QRectF();
    }
    // 考虑scale变换，使用实际显示尺寸
    qreal s = scale();
    return QRectF(scenePos(), QSizeF(pix.width() * s, pix.height() * s));
}

bool Entity::pixelCollision(Entity *a, Entity *b) {
    if (!a || !b)
        return false;

    // 使用实际pixmap边界框，避免受boundingRect()重写影响（如Boss血条扩展）
    QRectF rectA = a->pixmapSceneBoundingRect();
    QRectF rectB = b->pixmapSceneBoundingRect();

    // 快速 AABB 检测：如果边界框不相交，直接返回 false
    QRectF intersection = rectA.intersected(rectB);
    if (intersection.isEmpty()) {
        return false;
    }

    // 获取碰撞掩码（惰性生成）
    const QImage &maskA = a->getCollisionMask();
    const QImage &maskB = b->getCollisionMask();

    // 如果任一掩码为空，回退到边界框碰撞
    if (maskA.isNull() || maskB.isNull()) {
        return true;  // 边界框已相交，视为碰撞
    }

    // 计算缩放比例（掩码可能与显示尺寸不同）
    double scaleAx = static_cast<double>(maskA.width()) / rectA.width();
    double scaleAy = static_cast<double>(maskA.height()) / rectA.height();
    double scaleBx = static_cast<double>(maskB.width()) / rectB.width();
    double scaleBy = static_cast<double>(maskB.height()) / rectB.height();

    // 遍历重叠区域检测像素碰撞
    // 使用步长优化：每隔几个像素采样一次以提升性能
    const int step = 2;  // 采样步长，可调整（1=最精确，2-4=性能优化）

    int startX = static_cast<int>(intersection.left());
    int endX = static_cast<int>(intersection.right());
    int startY = static_cast<int>(intersection.top());
    int endY = static_cast<int>(intersection.bottom());

    for (int y = startY; y < endY; y += step) {
        for (int x = startX; x < endX; x += step) {
            // 转换为 A 的本地坐标
            int localAx = static_cast<int>((x - rectA.left()) * scaleAx);
            int localAy = static_cast<int>((y - rectA.top()) * scaleAy);

            // 转换为 B 的本地坐标
            int localBx = static_cast<int>((x - rectB.left()) * scaleBx);
            int localBy = static_cast<int>((y - rectB.top()) * scaleBy);

            // 边界检查
            if (localAx < 0 || localAx >= maskA.width() ||
                localAy < 0 || localAy >= maskA.height() ||
                localBx < 0 || localBx >= maskB.width() ||
                localBy < 0 || localBy >= maskB.height()) {
                continue;
            }

            // 检查两个掩码在该点是否都为不透明
            // Grayscale8 格式中，255 表示不透明
            uchar pixelA = maskA.constScanLine(localAy)[localAx];
            uchar pixelB = maskB.constScanLine(localBy)[localBx];

            if (pixelA > 0 && pixelB > 0) {
                return true;  // 发现碰撞，立即返回
            }
        }
    }

    return false;  // 没有像素碰撞
}

bool Entity::pixelCollisionWithPixmapItem(Entity *entity, QGraphicsPixmapItem *item, int alphaThreshold) {
    if (!entity || !item)
        return false;

    // 使用实际pixmap边界框
    QRectF rectA = entity->pixmapSceneBoundingRect();
    // 考虑scale变换
    qreal scaleB = item->scale();
    QRectF rectB = QRectF(item->scenePos(), QSizeF(item->pixmap().width() * scaleB, item->pixmap().height() * scaleB));

    // 快速 AABB 检测：如果边界框不相交，直接返回 false
    QRectF intersection = rectA.intersected(rectB);
    if (intersection.isEmpty()) {
        return false;
    }

    // 获取 Entity 的碰撞掩码（惰性生成）
    const QImage &maskA = entity->getCollisionMask();

    // 为 QGraphicsPixmapItem 临时生成碰撞掩码
    QPixmap pixB = item->pixmap();
    if (pixB.isNull()) {
        return true;  // 边界框已相交，视为碰撞
    }

    QImage imgB = pixB.toImage().convertToFormat(QImage::Format_ARGB32);
    QImage maskB(imgB.size(), QImage::Format_Grayscale8);
    maskB.fill(0);

    for (int y = 0; y < imgB.height(); ++y) {
        const QRgb *scanLine = reinterpret_cast<const QRgb *>(imgB.constScanLine(y));
        uchar *maskLine = maskB.scanLine(y);
        for (int x = 0; x < imgB.width(); ++x) {
            if (qAlpha(scanLine[x]) > alphaThreshold) {
                maskLine[x] = 255;
            }
        }
    }

    // 如果任一掩码为空，回退到边界框碰撞
    if (maskA.isNull() || maskB.isNull()) {
        return true;
    }

    // 计算缩放比例
    double scaleAx = static_cast<double>(maskA.width()) / rectA.width();
    double scaleAy = static_cast<double>(maskA.height()) / rectA.height();
    double scaleBx = static_cast<double>(maskB.width()) / rectB.width();
    double scaleBy = static_cast<double>(maskB.height()) / rectB.height();

    // 遍历重叠区域检测像素碰撞
    const int step = 2;

    int startX = static_cast<int>(intersection.left());
    int endX = static_cast<int>(intersection.right());
    int startY = static_cast<int>(intersection.top());
    int endY = static_cast<int>(intersection.bottom());

    for (int y = startY; y < endY; y += step) {
        for (int x = startX; x < endX; x += step) {
            int localAx = static_cast<int>((x - rectA.left()) * scaleAx);
            int localAy = static_cast<int>((y - rectA.top()) * scaleAy);
            int localBx = static_cast<int>((x - rectB.left()) * scaleBx);
            int localBy = static_cast<int>((y - rectB.top()) * scaleBy);

            if (localAx < 0 || localAx >= maskA.width() ||
                localAy < 0 || localAy >= maskA.height() ||
                localBx < 0 || localBx >= maskB.width() ||
                localBy < 0 || localBy >= maskB.height()) {
                continue;
            }

            uchar pixelA = maskA.constScanLine(localAy)[localAx];
            uchar pixelB = maskB.constScanLine(localBy)[localBx];

            if (pixelA > 0 && pixelB > 0) {
                return true;
            }
        }
    }

    return false;
}
