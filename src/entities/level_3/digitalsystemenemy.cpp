#include "digitalsystemenemy.h"
#include <QDebug>
#include "../../core/configmanager.h"

DigitalSystemEnemy::DigitalSystemEnemy(const QPixmap& pic, double scale)
    : ScalingEnemy(pic, scale) {
    // 从配置文件读取DigitalSystem敌人属性
    ConfigManager& config = ConfigManager::instance();
    setHealth(config.getEnemyInt("digital_system", "health", 25));
    setContactDamage(config.getEnemyInt("digital_system", "contact_damage", 3));
    setCircleRadius(config.getEnemyDouble("digital_system", "circle_radius", 180.0));
    setSpeed(config.getEnemyDouble("digital_system", "speed", 2.5));

    qDebug() << "创建DigitalSystem敌人";
}
