#ifndef STATUSEFFECT_H
#define STATUSEFFECT_H

#include <QObject>
#include "Entity.h"
#include "player.h"

class StatusEffect : public QObject
{
    Q_OBJECT
    double duration;//状态效果持续时间，以秒计
    QTimer *effTimer;
    Entity *target;
public:
    explicit StatusEffect(double dur, QObject *parent = nullptr);
    void applyTo(Entity* tgt);//应用效果到实体
    //virtual void update(float deltaTime);//更新效果
    void expire();//过期或移除
    virtual void onApplyEffect(Entity* target) = 0;
    virtual void onRemoveEffect(Entity* target) = 0;
signals:
};


//速度
class SpeedEffect : public StatusEffect {
    double multiplier;//速度倍数，<1减速，>1加速
public:
    SpeedEffect(double duration, double mul)
        : StatusEffect(duration), multiplier(mul) {}
    void onApplyEffect(Entity* target) override {
        target->setSpeed(target->getSpeed() * multiplier);
    }
    void onRemoveEffect(Entity* target) override {
        target->setSpeed(target->getSpeed() / multiplier);
    }
};

//子弹速度
class bulletSpeedEffect : public StatusEffect {
    double multiplier;
public:
    bulletSpeedEffect(double duration, double mul)
        : StatusEffect(duration), multiplier(mul) {}
    void onApplyEffect(Entity* target) override {
        target->setshootSpeed(target->getshootSpeed() * multiplier);
    }
    void onRemoveEffect(Entity* target) override {
        target->setshootSpeed(target->getshootSpeed() / multiplier);
    }
};

//射速（冷却时间降低）
class shootSpeedEffect : public StatusEffect {
    double multiplier;
    int temp;
public:
    shootSpeedEffect(double duration, double mul)
        : StatusEffect(duration), multiplier(mul) {}
    void onApplyEffect(Entity* target) override {
        if(auto p = dynamic_cast<Player*>(target)){
            temp = p->getShootCooldown();
            p->setShootCooldown((int)(temp / multiplier));
        }
    }
    void onRemoveEffect(Entity* target) override {
        if(auto p = dynamic_cast<Player*>(target))
            p->setShootCooldown(temp);
    }
};

//伤害提升/降低
class DamageEffect : public StatusEffect {
    double multiplier;//伤害倍数
public:
    DamageEffect(double duration, double mul)
        : StatusEffect(duration), multiplier(mul) {};
    void onApplyEffect(Entity* target) override {
        target->setHurt(target->getHurt() * multiplier);
    }
    void onRemoveEffect(Entity* target) override {
        target->setHurt(target->getHurt() / multiplier);
    }
};

//提升血量（护盾）
class soulHeartEffect{
public:
    soulHeartEffect(Player* pl, int n) {pl->addSoulHearts(n);};
};
class blackHeartEffect{
public:
    blackHeartEffect(Player* pl, int n) {pl->addBlackHearts(n);};
};

//伤害减免
class decDamage : public StatusEffect{
    double scale;
public:
    decDamage(double duration, double s)
        : StatusEffect(duration), scale(s) {};
    void onApplyEffect(Entity* target) override {
        target->damageScale = scale;
    }
    void onRemoveEffect(Entity* target) override {
        target->damageScale = 1.0;
    }
};

//中毒(一段时间内持续减血)
class PoisonEffect : public StatusEffect {
    int damage;
    QTimer *poisonTimer;
    Entity* target;
public:
    PoisonEffect(Entity* target_, double duration, int damage_);
    void emitApplyEffect(){this->onApplyEffect(target);};
    void onApplyEffect(Entity* target) override {
        target->takeDamage(damage);
    }
    void onRemoveEffect(Entity* target) override {
        if(!poisonTimer) return;
        poisonTimer->stop();
    }
};

//无敌
class InvincibleEffect : public StatusEffect {
public:
    InvincibleEffect(double duration) : StatusEffect(duration) {};
    void onApplyEffect(Entity* target) override {
        target->setInvincible(true);
    }
    void onRemoveEffect(Entity* target) override {
        target->setInvincible(false);
    }
};

#endif // STATUSEFFECT_H
