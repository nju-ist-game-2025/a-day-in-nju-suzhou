#include "bossfactory.h"
#include <QDebug>
#include "../entities/boss.h"

// 各关卡Boss
#include "../entities/level_1/nightmareboss.h"
#include "../entities/level_2/washmachineboss.h"
#include "../entities/level_3/teacherboss.h"

BossFactory& BossFactory::instance() {
    static BossFactory factory;
    return factory;
}

void BossFactory::registerBoss(int levelNumber, CreatorFunc creator) {
    m_creators[levelNumber] = creator;
    qDebug() << "BossFactory: 注册关卡" << levelNumber << "的Boss";
}

bool BossFactory::isRegistered(int levelNumber) const {
    return m_creators.find(levelNumber) != m_creators.end();
}

Boss* BossFactory::createBoss(int levelNumber, const QPixmap& pic, double scale, QGraphicsScene* scene) {
    auto it = m_creators.find(levelNumber);
    if (it != m_creators.end()) {
        return it->second(pic, scale, scene);
    }

    // 未找到注册的Boss类型，返回默认Boss
    qDebug() << "BossFactory: 关卡" << levelNumber << "未注册Boss，使用默认Boss";
    return new Boss(pic, scale);
}

// ==================== 静态注册所有Boss类型 ====================

// Level 1: Nightmare Boss (需要场景参数)
REGISTER_BOSS_WITH_SCENE(1, NightmareBoss);

// Level 2: WashMachine Boss
REGISTER_BOSS(2, WashMachineBoss);

// Level 3: Teacher Boss
REGISTER_BOSS(3, TeacherBoss);
