#include "statuseffect.h"

StatusEffect::StatusEffect(double dur, QObject *parent)
        : QObject{parent}, duration(dur), target(nullptr) {
    effTimer = new QTimer(this);
    effTimer->setSingleShot(true);
    connect(effTimer, &QTimer::timeout, this, &StatusEffect::expire);
}

void StatusEffect::applyTo(Entity *tgt) {
    if (!tgt) return;
    target = tgt;
    onApplyEffect(target);
    effTimer->start(static_cast<int>(duration * 1000));
}

void StatusEffect::expire() {
    if (!target) return;
    onRemoveEffect(target);
    effTimer->stop();
}

PoisonEffect::PoisonEffect(Entity *target_, double duration, int damage_)
        : StatusEffect(duration), damage(damage_), target(target_) {
    if(target_) {
        poisonTimer = new QTimer(this);
        connect(poisonTimer, &QTimer::timeout, this, &PoisonEffect::emitApplyEffect);
    }
    //poisonTimer->start(1000);
}

void StatusEffect::showFloatText(QGraphicsScene* scene, const QString& text, const QPointF& position, const QColor& color) {
    QGraphicsTextItem* textItem = new QGraphicsTextItem(text);
    textItem->setPos(position);
    textItem->setDefaultTextColor(color);
    textItem->setFont(QFont("Microsoft YaHei", 10, QFont::Black));
    textItem->setZValue(1000);

    scene->addItem(textItem);

    // 使用QTimer实现简单动画
    QTimer* moveTimer = new QTimer;
    QTimer* fadeTimer = new QTimer;

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
