#ifndef ITEM_H
#define ITEM_H

#include <QObject>
#include "Entity.h"
#include "player.h"

class Item : public QObject
{
    Q_OBJECT
    QString name;//道具名称
    QString description;//道具描述
public:
    explicit Item(const QString& name, const QString& desc);
    virtual void onPickup(Player* player) {};//拾取时触发效果
    QString getName() { return name; }
    QString getDescription() { return description; }
signals:
    void itemPickedUp(Item* item);
};



//伤害提升
class DamageUpItem : public Item {
    double multiplier;
public:
    DamageUpItem(const QString& name, double mul)
        : Item(name, "伤害提升"), multiplier(mul) {}
    void onPickup(Player* player) override {
        player->setHurt(player->getHurt() * multiplier);
    }
};

//速度提升
class SpeedUpItem : public Item {
    double multiplier;
public:
    SpeedUpItem(const QString& name, double mul)
        : Item(name, "速度提升"), multiplier(mul) {}
    void onPickup(Player* player) override {
        player->setSpeed(player->getSpeed() * multiplier);
    }
};

//射速提升（冷却时间降低）
class ShootSpeedUpItem : public Item {
    double multiplier;
public:
    ShootSpeedUpItem(const QString& name, double mul)
        : Item(name, "射速提升"), multiplier(mul) {}
    void onPickup(Player* player) override {
        int currentCD = player->getShootCooldown();
        player->setShootCooldown((int)(currentCD / multiplier));
    }
};

//子弹速度提升
class BulletSpeedUpItem : public Item {
    double multiplier;
public:
    BulletSpeedUpItem(const QString& name, double mul)
        : Item(name, "子弹速度提升"), multiplier(mul) {}
    void onPickup(Player* player) override {
        player->setshootSpeed(player->getshootSpeed() * multiplier);
    }
};

//红心容器
class RedHeartContainerItem : public Item {
    int heartCount;
public:
    RedHeartContainerItem(const QString& name, int count)
        : Item(name, "增加红心容器"), heartCount(count) {}
    void onPickup(Player* player) override {
        player->addRedContainers(heartCount);
        player->addRedHearts(heartCount);
    }
};

//改变攻击方式
class BrimstoneItem : public Item {
public:
    BrimstoneItem(const QString& name)
        : Item(name, "改变攻击方式") {}
    void onPickup(Player* player) override {
        player->setShootType(1); // 0=普通, 1=激光
        player->setShootCooldown(20); // 降低射速平衡强度
        player->damageScale *= 1.5;
    }
};


//炸弹
class BombItem : public Item {
    int count;//数量
public:
    BombItem(const QString& name, int count)
        : Item(name, "获得炸弹"), count(count) {}
    void onPickup(Player* player) override {
        player->addBombs(count);
    }
};


#endif // ITEM_H
