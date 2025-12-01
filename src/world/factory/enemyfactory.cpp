#include "enemyfactory.h"
#include <QDebug>
#include "../entities/enemy.h"

// Level 1 敌人
#include "../entities/level_1/clockenemy.h"
#include "../entities/level_1/pillowenemy.h"

// Level 2 敌人
#include "../entities/level_2/pantsenemy.h"
#include "../entities/level_2/sockenemy.h"
#include "../entities/level_2/sockshooter.h"
#include "../entities/level_2/walker.h"

// Level 3 敌人
#include "../entities/level_3/digitalsystemenemy.h"
#include "../entities/level_3/optimizationenemy.h"
#include "../entities/level_3/probabilityenemy.h"
#include "../entities/level_3/xukeenemy.h"
#include "../entities/level_3/yanglinenemy.h"
#include "../entities/level_3/zhuhaoenemy.h"

EnemyFactory& EnemyFactory::instance() {
    static EnemyFactory factory;
    return factory;
}

std::string EnemyFactory::makeKey(int levelNumber, const QString& enemyType) const {
    return std::to_string(levelNumber) + "_" + enemyType.toStdString();
}

void EnemyFactory::registerEnemy(int levelNumber, const QString& enemyType, CreatorFunc creator) {
    std::string key = makeKey(levelNumber, enemyType);
    m_creators[key] = creator;
    qDebug() << "EnemyFactory: 注册敌人类型" << enemyType << "关卡" << levelNumber;
}

bool EnemyFactory::isRegistered(int levelNumber, const QString& enemyType) const {
    std::string key = makeKey(levelNumber, enemyType);
    return m_creators.find(key) != m_creators.end();
}

Enemy* EnemyFactory::createEnemy(int levelNumber, const QString& enemyType, const QPixmap& pic, double scale) {
    std::string key = makeKey(levelNumber, enemyType);

    auto it = m_creators.find(key);
    if (it != m_creators.end()) {
        return it->second(pic, scale);
    }

    // 未找到注册的类型，返回默认敌人
    qDebug() << "EnemyFactory: 未知敌人类型，使用默认Enemy:" << enemyType << "关卡" << levelNumber;
    return new Enemy(pic, scale);
}

// ==================== 静态注册所有敌人类型 ====================

// Level 1 敌人注册
REGISTER_ENEMY(1, "clock_normal", ClockEnemy);
REGISTER_ENEMY(1, "pillow", PillowEnemy);

// Level 2 敌人注册
REGISTER_ENEMY(2, "sock_normal", SockNormal);
REGISTER_ENEMY(2, "sock_angrily", SockAngrily);
REGISTER_ENEMY(2, "pants", PantsEnemy);
REGISTER_ENEMY(2, "sock_shooter", SockShooter);
REGISTER_ENEMY(2, "walker", Walker);

// Level 3 敌人注册
REGISTER_ENEMY(3, "optimization", OptimizationEnemy);
REGISTER_ENEMY(3, "digital_system", DigitalSystemEnemy);
REGISTER_ENEMY(3, "yanglin", YanglinEnemy);
REGISTER_ENEMY(3, "xuke", XukeEnemy);
REGISTER_ENEMY(3, "probability_theory", ProbabilityEnemy);
REGISTER_ENEMY(3, "zhuhao", ZhuhaoEnemy);
