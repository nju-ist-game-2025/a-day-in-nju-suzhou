#include "statuseffect.h"

StatusEffect::StatusEffect(double dur, QObject *parent)
    : QObject{parent}, duration(dur), target(nullptr) {
    effTimer = new QTimer(this);
    effTimer->setSingleShot(true);
    connect(effTimer, &QTimer::timeout, this, &StatusEffect::expire);
}

void StatusEffect::applyTo(Entity* tgt) {
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

PoisonEffect::PoisonEffect(Entity* target_, double duration, int damage_)
    : StatusEffect(duration), damage(damage_), target(target_) {
    poisonTimer = new QTimer(this);
    connect(poisonTimer, &QTimer::timeout, this, &PoisonEffect::emitApplyEffect);
    poisonTimer->start(1000);
}
