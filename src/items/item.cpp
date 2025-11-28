#include "item.h"
#include <QGraphicsTextItem>
#include <QPointer>
#include <QTimer>
#include <memory>

Item::Item(const QString& name, const QString& desc) {
    this->name = name;
    this->description = desc;
}

void Item::showFloatText(QGraphicsScene* scene, const QString& text, const QPointF& position, const QColor& color) {
    if (!scene)
        return;

    QGraphicsTextItem* textItem = new QGraphicsTextItem(text);
    textItem->setPos(position);
    textItem->setDefaultTextColor(color);
    textItem->setFont(QFont("Microsoft YaHei", 10, QFont::Black));
    textItem->setZValue(1000);

    scene->addItem(textItem);

    // 使用 QPointer 追踪 textItem，防止场景清理后访问悬空指针
    QPointer<QGraphicsTextItem> textPtr(textItem);
    QPointer<QGraphicsScene> scenePtr(scene);

    // 使用 shared_ptr 存储状态
    auto stepPtr = std::make_shared<int>(0);

    // 创建定时器
    QTimer* moveTimer = new QTimer;
    QTimer* fadeTimer = new QTimer;

    // 上升动画
    connect(moveTimer, &QTimer::timeout, [textPtr, moveTimer, stepPtr]() {
        if (!textPtr) {
            moveTimer->stop();
            moveTimer->deleteLater();
            return;
        }
        textPtr->setPos(textPtr->pos() + QPointF(0, -2));
        (*stepPtr)++;
        if (*stepPtr >= 25) {
            moveTimer->stop();
            moveTimer->deleteLater();
        }
    });

    // 淡出动画
    connect(fadeTimer, &QTimer::timeout, [textPtr, scenePtr, fadeTimer]() {
        if (!textPtr) {
            fadeTimer->stop();
            fadeTimer->deleteLater();
            return;
        }

        qreal opacity = textPtr->opacity() - 0.05;
        if (opacity <= 0) {
            if (scenePtr && textPtr->scene() == scenePtr) {
                scenePtr->removeItem(textPtr.data());
            }
            delete textPtr.data();
            fadeTimer->stop();
            fadeTimer->deleteLater();
        } else {
            textPtr->setOpacity(opacity);
        }
    });

    moveTimer->start(150);
    fadeTimer->start(100);
}
