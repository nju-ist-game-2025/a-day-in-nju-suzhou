#ifndef CONFIGVALIDATOR_H
#define CONFIGVALIDATOR_H

#include <QString>
#include <QStringList>

/**
 * @brief 配置验证器 - 验证所有实体配置是否正确加载
 */
class ConfigValidator {
   public:
    /**
     * @brief 验证所有配置是否正确加载
     * @return 验证是否全部通过
     */
    static bool validateAllConfigs();

    /**
     * @brief 验证玩家配置
     */
    static bool validatePlayerConfig();

    /**
     * @brief 验证所有敌人配置
     */
    static bool validateEnemyConfigs();

    /**
     * @brief 验证所有Boss配置
     */
    static bool validateBossConfigs();

    /**
     * @brief 获取验证报告
     */
    static QString getValidationReport();

   private:
    static QStringList s_errors;
    static QStringList s_warnings;
    static QStringList s_successes;

    static void logError(const QString& msg);
    static void logWarning(const QString& msg);
    static void logSuccess(const QString& msg);
    static void clearLogs();
};

#endif  // CONFIGVALIDATOR_H
