#include "configvalidator.h"
#include <QDebug>
#include "configmanager.h"

// 静态成员初始化
QStringList ConfigValidator::s_errors;
QStringList ConfigValidator::s_warnings;
QStringList ConfigValidator::s_successes;

void ConfigValidator::logError(const QString& msg) {
    s_errors.append(msg);
    qCritical() << "[CONFIG ERROR]" << msg;
}

void ConfigValidator::logWarning(const QString& msg) {
    s_warnings.append(msg);
    qWarning() << "[CONFIG WARNING]" << msg;
}

void ConfigValidator::logSuccess(const QString& msg) {
    s_successes.append(msg);
    qDebug() << "[CONFIG OK]" << msg;
}

void ConfigValidator::clearLogs() {
    s_errors.clear();
    s_warnings.clear();
    s_successes.clear();
}

bool ConfigValidator::validateAllConfigs() {
    clearLogs();
    qDebug() << "========== 开始配置验证 ==========";

    bool playerOk = validatePlayerConfig();
    bool enemiesOk = validateEnemyConfigs();
    bool bossesOk = validateBossConfigs();

    qDebug() << "========== 配置验证完成 ==========";
    qDebug() << "成功:" << s_successes.count() << "警告:" << s_warnings.count() << "错误:" << s_errors.count();

    return playerOk && enemiesOk && bossesOk;
}

bool ConfigValidator::validatePlayerConfig() {
    qDebug() << "--- 验证玩家配置 ---";
    ConfigManager& config = ConfigManager::instance();
    bool allOk = true;

    // 必须的配置项
    struct {
        const char* key;
        int minVal;
        int maxVal;
        bool isInt;
    } intConfigs[] = {
        {"health", 1, 100, true},
        {"shoot_cooldown", 50, 5000, true},
        {"bullet_hurt", 1, 100, true},
        {"crash_radius", 10, 200, true},
        {"teleport_cooldown", 1000, 60000, true},
        {"ultimate_cooldown", 10000, 300000, true},
        {"ultimate_duration", 1000, 60000, true},
    };

    struct {
        const char* key;
        double minVal;
        double maxVal;
    } doubleConfigs[] = {
        {"speed", 0.5, 20.0},
        {"teleport_distance", 50.0, 500.0},
        {"ultimate_damage_multiplier", 1.0, 10.0},
        {"ultimate_bullet_scale", 1.0, 5.0},
    };

    for (const auto& cfg : intConfigs) {
        int val = config.getPlayerInt(cfg.key, -9999);
        if (val == -9999) {
            logError(QString("玩家配置缺失: %1").arg(cfg.key));
            allOk = false;
        } else if (val < cfg.minVal || val > cfg.maxVal) {
            logWarning(QString("玩家配置 %1=%2 超出合理范围[%3,%4]").arg(cfg.key).arg(val).arg(cfg.minVal).arg(cfg.maxVal));
        } else {
            logSuccess(QString("玩家 %1=%2").arg(cfg.key).arg(val));
        }
    }

    for (const auto& cfg : doubleConfigs) {
        double val = config.getPlayerDouble(cfg.key, -9999.0);
        if (val < -9998.0) {
            logError(QString("玩家配置缺失: %1").arg(cfg.key));
            allOk = false;
        } else if (val < cfg.minVal || val > cfg.maxVal) {
            logWarning(QString("玩家配置 %1=%2 超出合理范围[%3,%4]").arg(cfg.key).arg(val).arg(cfg.minVal).arg(cfg.maxVal));
        } else {
            logSuccess(QString("玩家 %1=%2").arg(cfg.key).arg(val));
        }
    }

    return allOk;
}

bool ConfigValidator::validateEnemyConfigs() {
    qDebug() << "--- 验证敌人配置 ---";
    ConfigManager& config = ConfigManager::instance();
    bool allOk = true;

    // 所有敌人类型
    QStringList enemyTypes = {
        "clock_normal", "clock_boom", "pillow",
        "sock_normal", "sock_angrily", "sock_shooter", "orbiting_sock",
        "pants", "walker",
        "yanglin", "zhuhao", "xuke", "probability_theory",
        "invigilator", "digital_system", "optimization"};

    // 每种敌人必须有的基础配置
    QStringList requiredIntKeys = {"health", "contact_damage"};
    QStringList requiredDoubleKeys = {"speed"};

    for (const QString& enemyType : enemyTypes) {
        qDebug() << "  检查敌人:" << enemyType;

        // 检查必须的整数配置
        for (const QString& key : requiredIntKeys) {
            int val = config.getEnemyInt(enemyType, key, -9999);
            if (val == -9999) {
                logError(QString("敌人[%1]缺失配置: %2").arg(enemyType, key));
                allOk = false;
            } else {
                logSuccess(QString("敌人[%1] %2=%3").arg(enemyType, key).arg(val));
            }
        }

        // 检查必须的浮点配置
        for (const QString& key : requiredDoubleKeys) {
            double val = config.getEnemyDouble(enemyType, key, -9999.0);
            if (val < -9998.0) {
                logError(QString("敌人[%1]缺失配置: %2").arg(enemyType, key));
                allOk = false;
            } else {
                logSuccess(QString("敌人[%1] %2=%3").arg(enemyType, key).arg(val));
            }
        }

        // 根据敌人类型检查特定配置
        if (enemyType == "clock_normal") {
            double zigzag = config.getEnemyDouble(enemyType, "zigzag_amplitude", -1);
            if (zigzag < 0)
                logWarning(QString("敌人[%1]缺失zigzag_amplitude").arg(enemyType));
        } else if (enemyType == "clock_boom") {
            double radius = config.getEnemyDouble(enemyType, "explosion_radius", -1);
            if (radius < 0)
                logWarning(QString("敌人[%1]缺失explosion_radius").arg(enemyType));
        } else if (enemyType == "pillow") {
            double radius = config.getEnemyDouble(enemyType, "circle_radius", -1);
            if (radius < 0)
                logWarning(QString("敌人[%1]缺失circle_radius").arg(enemyType));
        } else if (enemyType == "sock_shooter") {
            int cooldown = config.getEnemyInt(enemyType, "shoot_cooldown", -1);
            if (cooldown < 0)
                logWarning(QString("敌人[%1]缺失shoot_cooldown").arg(enemyType));
        } else if (enemyType == "sock_angrily") {
            int chargeTime = config.getEnemyInt(enemyType, "dash_charge_time", -1);
            double dashSpeed = config.getEnemyDouble(enemyType, "dash_speed", -1);
            if (chargeTime < 0)
                logWarning(QString("敌人[%1]缺失dash_charge_time").arg(enemyType));
            if (dashSpeed < 0)
                logWarning(QString("敌人[%1]缺失dash_speed").arg(enemyType));
        } else if (enemyType == "orbiting_sock") {
            double orbitRadius = config.getEnemyDouble(enemyType, "orbit_radius", -1);
            double orbitSpeed = config.getEnemyDouble(enemyType, "orbit_speed", -1);
            if (orbitRadius < 0)
                logWarning(QString("敌人[%1]缺失orbit_radius").arg(enemyType));
            if (orbitSpeed < 0)
                logWarning(QString("敌人[%1]缺失orbit_speed").arg(enemyType));
        } else if (enemyType == "walker") {
            int trailDuration = config.getEnemyInt(enemyType, "trail_duration", -1);
            if (trailDuration < 0)
                logWarning(QString("敌人[%1]缺失trail_duration").arg(enemyType));
        } else if (enemyType == "yanglin") {
            int spinCooldown = config.getEnemyInt(enemyType, "spinning_cooldown", -1);
            if (spinCooldown < 0)
                logWarning(QString("敌人[%1]缺失spinning_cooldown").arg(enemyType));
        } else if (enemyType == "zhuhao") {
            double edgeSpeed = config.getEnemyDouble(enemyType, "edge_move_speed", -1);
            int bulletCount = config.getEnemyInt(enemyType, "bullets_per_wave", -1);
            if (edgeSpeed < 0)
                logWarning(QString("敌人[%1]缺失edge_move_speed").arg(enemyType));
            if (bulletCount < 0)
                logWarning(QString("敌人[%1]缺失bullets_per_wave").arg(enemyType));
        } else if (enemyType == "xuke") {
            int shootCooldown = config.getEnemyInt(enemyType, "shoot_cooldown", -1);
            if (shootCooldown < 0)
                logWarning(QString("敌人[%1]缺失shoot_cooldown").arg(enemyType));
        } else if (enemyType == "invigilator") {
            double patrolRadius = config.getEnemyDouble(enemyType, "patrol_radius", -1);
            if (patrolRadius < 0)
                logWarning(QString("敌人[%1]缺失patrol_radius").arg(enemyType));
        } else if (enemyType == "digital_system" || enemyType == "optimization") {
            double circleRadius = config.getEnemyDouble(enemyType, "circle_radius", -1);
            if (circleRadius < 0)
                logWarning(QString("敌人[%1]缺失circle_radius").arg(enemyType));
        }
    }

    return allOk;
}

bool ConfigValidator::validateBossConfigs() {
    qDebug() << "--- 验证Boss配置 ---";
    ConfigManager& config = ConfigManager::instance();
    bool allOk = true;

    // Boss类型和对应的阶段
    struct BossInfo {
        QString type;
        QStringList phases;
    };

    QList<BossInfo> bosses = {
        {"nightmare", {"phase1", "phase2"}},
        {"washmachine", {"phase1", "phase2", "phase3"}},
        {"teacher", {"phase1", "phase2", "phase3"}}};

    // 每个阶段必须有的基础配置
    QStringList requiredIntKeys = {"contact_damage"};
    QStringList phase1RequiredIntKeys = {"health", "attack_cooldown"};

    for (const BossInfo& boss : bosses) {
        qDebug() << "  检查Boss:" << boss.type;

        for (const QString& phase : boss.phases) {
            qDebug() << "    阶段:" << phase;

            // 检查contact_damage
            for (const QString& key : requiredIntKeys) {
                int val = config.getBossInt(boss.type, phase, key, -9999);
                if (val == -9999) {
                    logError(QString("Boss[%1][%2]缺失配置: %3").arg(boss.type, phase, key));
                    allOk = false;
                } else {
                    logSuccess(QString("Boss[%1][%2] %3=%4").arg(boss.type, phase, key).arg(val));
                }
            }

            // 第一阶段需要health
            if (phase == "phase1") {
                for (const QString& key : phase1RequiredIntKeys) {
                    int val = config.getBossInt(boss.type, phase, key, -9999);
                    if (val == -9999) {
                        logError(QString("Boss[%1][%2]缺失配置: %3").arg(boss.type, phase, key));
                        allOk = false;
                    } else {
                        logSuccess(QString("Boss[%1][%2] %3=%4").arg(boss.type, phase, key).arg(val));
                    }
                }

                // 检查speed
                double speed = config.getBossDouble(boss.type, phase, "speed", -9999.0);
                if (speed < -9998.0) {
                    logError(QString("Boss[%1][%2]缺失配置: speed").arg(boss.type, phase));
                    allOk = false;
                } else {
                    logSuccess(QString("Boss[%1][%2] speed=%3").arg(boss.type, phase).arg(speed));
                }
            }

            // 检查特定Boss特定阶段的配置
            if (boss.type == "nightmare") {
                double dashSpeed = config.getBossDouble(boss.type, phase, "dash_speed", -1);
                int dashChargeTime = config.getBossInt(boss.type, phase, "dash_charge_time", -1);
                if (dashSpeed < 0)
                    logWarning(QString("Boss[%1][%2]缺失dash_speed").arg(boss.type, phase));
                if (dashChargeTime < 0)
                    logWarning(QString("Boss[%1][%2]缺失dash_charge_time").arg(boss.type, phase));
            } else if (boss.type == "washmachine") {
                if (phase == "phase2" || phase == "phase3") {
                    double dashSpeed = config.getBossDouble(boss.type, phase, "dash_speed", -1);
                    int dashChargeTime = config.getBossInt(boss.type, phase, "dash_charge_time", -1);
                    if (dashSpeed < 0)
                        logWarning(QString("Boss[%1][%2]缺失dash_speed").arg(boss.type, phase));
                    if (dashChargeTime < 0)
                        logWarning(QString("Boss[%1][%2]缺失dash_charge_time").arg(boss.type, phase));
                }
            } else if (boss.type == "teacher") {
                if (phase == "phase2") {
                    double dashSpeed = config.getBossDouble(boss.type, phase, "dash_speed", -1);
                    if (dashSpeed < 0)
                        logWarning(QString("Boss[%1][%2]缺失dash_speed").arg(boss.type, phase));
                }
            }
        }
    }

    return allOk;
}

QString ConfigValidator::getValidationReport() {
    QString report;
    report += "========== 配置验证报告 ==========\n\n";

    if (!s_errors.isEmpty()) {
        report += QString("❌ 错误 (%1):\n").arg(s_errors.count());
        for (const QString& err : s_errors) {
            report += QString("  - %1\n").arg(err);
        }
        report += "\n";
    }

    if (!s_warnings.isEmpty()) {
        report += QString("⚠️ 警告 (%1):\n").arg(s_warnings.count());
        for (const QString& warn : s_warnings) {
            report += QString("  - %1\n").arg(warn);
        }
        report += "\n";
    }

    report += QString("✅ 成功加载: %1 项配置\n").arg(s_successes.count());
    report += "\n========================================\n";

    return report;
}
