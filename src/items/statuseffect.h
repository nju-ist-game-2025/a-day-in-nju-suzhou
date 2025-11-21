#ifndef STATUSEFFECT_H
#define STATUSEFFECT_H

#include <QDebug>
#include <QObject>
#include <QPainter>
#include <QPointer>
#include "Entity.h"
#include "player.h"

class StatusEffect : public QObject {
    Q_OBJECT
    double duration;  // 状态效果持续时间，以秒计
    QTimer* effTimer;

   protected:
    QPointer<Entity> target;  // 使用 QPointer 自动处理对象销毁

   public:
    explicit StatusEffect(double dur, QObject* parent = nullptr);
    void applyTo(Entity* tgt);  // 应用效果到实体
    // virtual void update(float deltaTime);//更新效果
    void expire();  // 过期或移除
    virtual void onApplyEffect(Entity* target) {};
    virtual void onRemoveEffect(Entity* target) {};
    static void showFloatText(QGraphicsScene* scene, const QString& text, const QPointF& position, const QColor& color = Qt::black);
   signals:
};

// 速度
class SpeedEffect : public StatusEffect {
    double multiplier;  // 速度倍数，<1减速，>1加速
   public:
    SpeedEffect(double duration, double mul)
        : StatusEffect(duration), multiplier(mul) {}
    void onApplyEffect(Entity* target) override {
        if (!target)
            return;
        target->setSpeed(target->getSpeed() * multiplier);
        if (multiplier > 1) {
            showFloatText(target->scene(), QString("⚡短暂速度提升↑"), target->pos(), Qt::blue);
        } else if (multiplier < 1)
            showFloatText(target->scene(), QString("⚡短暂速度下降↓"), target->pos(), Qt::blue);
    }
    void onRemoveEffect(Entity* target) override {
        if (!target)
            return;
        target->setSpeed(target->getSpeed() / multiplier);
    }
};

// 子弹速度
class bulletSpeedEffect : public StatusEffect {
    double multiplier;

   public:
    bulletSpeedEffect(double duration, double mul)
        : StatusEffect(duration), multiplier(mul) {}
    void onApplyEffect(Entity* target) override {
        if (!target)
            return;
        target->setshootSpeed(target->getshootSpeed() * multiplier);
        if (multiplier > 1)
            showFloatText(target->scene(), QString("短暂子弹速度提升↑"), target->pos());
        else if (multiplier < 1)
            showFloatText(target->scene(), QString("短暂子弹速度下降↓"), target->pos());
    }
    void onRemoveEffect(Entity* target) override {
        if (!target)
            return;
        target->setshootSpeed(target->getshootSpeed() / multiplier);
    }
};

// 射速（冷却时间降低）
class shootSpeedEffect : public StatusEffect {
    double multiplier;
    int temp;

   public:
    shootSpeedEffect(double duration, double mul)
        : StatusEffect(duration), multiplier(mul) {}
    void onApplyEffect(Entity* target) override {
        if (!target)
            return;
        if (auto p = dynamic_cast<Player*>(target)) {
            temp = p->getShootCooldown();
            p->setShootCooldown((int)(temp / multiplier));
            if (multiplier > 1)
                showFloatText(target->scene(), QString("🔫短暂射速提升↑"), target->pos());
            else if (multiplier < 1)
                showFloatText(target->scene(), QString("🔫短暂射速下降↓"), target->pos());
        }
    }
    void onRemoveEffect(Entity* target) override {
        if (!target)
            return;
        if (auto p = dynamic_cast<Player*>(target))
            p->setShootCooldown(temp);
    }
};

// 伤害提升/降低
class DamageEffect : public StatusEffect {
    double multiplier;  // 伤害倍数
   public:
    DamageEffect(double duration, double mul)
        : StatusEffect(duration), multiplier(mul) {};
    void onApplyEffect(Entity* target) override {
        if (!target)
            return;
        target->setHurt(target->getHurt() * multiplier);
        if (multiplier > 1)
            showFloatText(target->scene(), QString("⚔️短暂伤害提升↑"), target->pos(), Qt::red);
        else if (multiplier < 1)
            showFloatText(target->scene(), QString("⚔️短暂伤害下降↓"), target->pos(), Qt::red);
    }
    void onRemoveEffect(Entity* target) override {
        if (!target)
            return;
        target->setHurt(target->getHurt() / multiplier);
    }
};

// 提升血量（护盾）
class soulHeartEffect : public StatusEffect {
    int hearts;

   public:
    soulHeartEffect(Player* pl, int n) : StatusEffect(1), hearts(n) {
                                             // 构造函数不再直接应用效果，而是通过 onApplyEffect
                                         };

    void onApplyEffect(Entity* target) override {
        if (auto pl = dynamic_cast<Player*>(target)) {
            pl->addSoulHearts(hearts);
            StatusEffect::showFloatText(pl->scene(), QString("♥ ++魂心"), pl->pos(), Qt::green);
        }
    }
};

class blackHeartEffect : public StatusEffect {
    int hearts;

   public:
    blackHeartEffect(Player* pl, int n) : StatusEffect(1), hearts(n) {
                                              // 构造函数不再直接应用效果
                                          };

    void onApplyEffect(Entity* target) override {
        if (auto pl = dynamic_cast<Player*>(target)) {
            pl->addBlackHearts(hearts);
            StatusEffect::showFloatText(pl->scene(), QString("♥ ++黑心"), pl->pos(), Qt::darkGray);
        }
    }
};

// 伤害减免
class decDamage : public StatusEffect {
    double scale;

   public:
    decDamage(double duration, double s)
        : StatusEffect(duration), scale(s) {};
    void onApplyEffect(Entity* target) override {
        if (!target)
            return;
        target->damageScale = scale;
        showFloatText(target->scene(), QString("🛡️短暂伤害减免"), target->pos(), Qt::green);
    }
    void onRemoveEffect(Entity* target) override {
        if (!target)
            return;
        target->damageScale = 1.0;
    }
};

// 中毒(一段时间内持续减血)
class PoisonEffect : public StatusEffect {
    int damage;
    QTimer* poisonTimer;
    // Entity* target; // 使用基类的 QPointer<Entity> target

   public:
    PoisonEffect(Entity* target_, double duration, int damage_);
    void emitApplyEffect() {
        if (target)
            this->onApplyEffect(target);
    };
    void onApplyEffect(Entity* target) override {
        if (!target)
            return;
        target->takeDamage(damage);
        showFloatText(target->scene(), QString("中毒"), target->pos(), Qt::darkGreen);

        // 如果是第一次应用，启动中毒定时器
        if (poisonTimer && !poisonTimer->isActive()) {
            qDebug() << "启动中毒定时器";
            poisonTimer->start(1000);
        }
    }
    void onRemoveEffect(Entity* target) override {
        if (!target)
            return;
        if (!poisonTimer)
            return;
        poisonTimer->stop();
    }
};

// 无敌
class InvincibleEffect : public StatusEffect {
   public:
    InvincibleEffect(double duration) : StatusEffect(duration) {};
    void onApplyEffect(Entity* target) override {
        if (!target)
            return;
        target->setInvincible(true);
        showFloatText(target->scene(), QString("🛡️短暂无敌"), target->pos(), Qt::darkYellow);
    }
    void onRemoveEffect(Entity* target) override {
        if (!target)
            return;
        target->setInvincible(false);
    }
};

#endif  // STATUSEFFECT_H
