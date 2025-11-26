#include "item.h"

Item::Item(const QString &name, const QString &desc) {
    this->name = name;
    this->description = desc;
}

void Item::showFloatText(QGraphicsScene *scene, const QString &text, const QPointF &position, const QColor &color) {
    QGraphicsTextItem * textItem = new QGraphicsTextItem(text);
    textItem->setPos(position);
    textItem->setDefaultTextColor(color);
    textItem->setFont(QFont("Microsoft YaHei", 10, QFont::Black));
    textItem->setZValue(1000);

    scene->addItem(textItem);

    // 使用QTimer实现简单动画
    QTimer * moveTimer = new QTimer;
    QTimer * fadeTimer = new QTimer;

    // 使用值捕获，避免悬挂引用
    int step = 0;
    connect(moveTimer, &QTimer::timeout, [textItem, moveTimer, &step]() {  // 值捕获
        textItem->setPos(textItem->pos() + QPointF(0, -2));
        step++;
        if (step >= 25) {
            moveTimer->stop();
            moveTimer->deleteLater();
        }
    });

    connect(fadeTimer, &QTimer::timeout, [textItem, scene, fadeTimer]() {  // 值捕获
        qreal opacity = textItem->opacity() - 0.05;
        if (opacity <= 0) {
            scene->removeItem(textItem);
            delete textItem;
            fadeTimer->stop();
            fadeTimer->deleteLater();
        } else {
            textItem->setOpacity(opacity);
        }
    });

    moveTimer->start(150);
    fadeTimer->start(100);
}
