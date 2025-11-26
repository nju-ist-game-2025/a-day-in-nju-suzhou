#include "statuseffect.h"
#include <memory>

StatusEffect::StatusEffect(double dur, QObject *parent)
    : QObject{parent}, duration(dur), target(nullptr)
{
    effTimer = new QTimer(this);
    effTimer->setSingleShot(true);
    connect(effTimer, &QTimer::timeout, this, &StatusEffect::expire);
}

void StatusEffect::applyTo(Entity *tgt)
{
    if (!tgt)
        return;
    target = tgt;
    onApplyEffect(target);
    effTimer->start(static_cast<int>(duration * 1000));
}

void StatusEffect::expire()
{
    if (target)
    {
        onRemoveEffect(target);
    }
    effTimer->stop();
    deleteLater(); // 自我销毁，防止内存泄漏
}

PoisonEffect::PoisonEffect(Entity *target_, double duration, int damage_)
    : StatusEffect(duration), damage(damage_), poisonTimer(nullptr)
{
    // 无论 target_ 是否为空，都创建定时器
    // 定时器会在 onApplyEffect 中启动
    poisonTimer = new QTimer(this);
    connect(poisonTimer, &QTimer::timeout, this, &PoisonEffect::emitApplyEffect);
    Q_UNUSED(target_); // target_ 参数目前未使用，保留用于兼容性
}

void StatusEffect::showFloatText(QGraphicsScene *scene, const QString &text, const QPointF &position, const QColor &color)
{
    if (!scene)
        return;

    QGraphicsTextItem *textItem = new QGraphicsTextItem(text);
    textItem->setPos(position);
    textItem->setDefaultTextColor(color);
    textItem->setFont(QFont("Microsoft YaHei", 10, QFont::Black));
    textItem->setZValue(1000);

    scene->addItem(textItem);

    // 使用 QPointer 追踪 textItem，防止场景清理后访问悬空指针
    QPointer<QGraphicsTextItem> textPtr(textItem);

    // 使用shared_ptr存储所有需要的状态
    auto stepPtr = std::make_shared<int>(0);
    auto textDeleted = std::make_shared<bool>(false);

    // 创建定时器
    QTimer *moveTimer = new QTimer;
    QTimer *fadeTimer = new QTimer;

    // 上升动画
    connect(moveTimer, &QTimer::timeout, [textPtr, moveTimer, stepPtr, textDeleted]()
            {
        if (*textDeleted || !textPtr) {
            moveTimer->stop();
            moveTimer->deleteLater();
            return;
        }
        textPtr->setPos(textPtr->pos() + QPointF(0, -2));
        (*stepPtr)++;
        if (*stepPtr >= 25) {
            moveTimer->stop();
            moveTimer->deleteLater();
        } });

    // 淡出动画
    connect(fadeTimer, &QTimer::timeout, [textPtr, scene, fadeTimer, textDeleted]()
            {
        if (*textDeleted) {
            fadeTimer->stop();
            fadeTimer->deleteLater();
            return;
        }

        if (!textPtr) {
            *textDeleted = true;
            fadeTimer->stop();
            fadeTimer->deleteLater();
            return;
        }

        qreal opacity = textPtr->opacity() - 0.05;
        if (opacity <= 0) {
            *textDeleted = true;
            if (scene && textPtr->scene() == scene) {
                scene->removeItem(textPtr.data());
            }
            delete textPtr.data();
            fadeTimer->stop();
            fadeTimer->deleteLater();
        } else {
            textPtr->setOpacity(opacity);
        } });

    moveTimer->start(150);
    fadeTimer->start(100);
}
