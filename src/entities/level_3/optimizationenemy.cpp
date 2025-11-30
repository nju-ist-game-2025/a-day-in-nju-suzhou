#include "optimizationenemy.h"
#include <QDebug>
#include "../../core/configmanager.h"

OptimizationEnemy::OptimizationEnemy(const QPixmap& pic, double scale)
    : ScalingEnemy(pic, scale) {
    // 从配置文件读取Optimization敌人属性
    ConfigManager& config = ConfigManager::instance();
    setHealth(config.getEnemyInt("optimization", "health", 25));
    setContactDamage(config.getEnemyInt("optimization", "contact_damage", 3));
    setCircleRadius(config.getEnemyDouble("optimization", "circle_radius", 180.0));
    setSpeed(config.getEnemyDouble("optimization", "speed", 2.5));

    qDebug() << "创建Optimization敌人";
}
