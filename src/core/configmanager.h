#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

/**
 * @brief 配置管理器类 - 负责读取和提供游戏配置信息
 * 支持从JSON配置文件读取各种游戏设置
 */
class ConfigManager {
   public:
    /**
     * @brief 获取单例实例
     */
    static ConfigManager& instance();

    /**
     * @brief 加载配置文件
     * @param configPath 配置文件路径
     * @return 是否加载成功
     */
    bool loadConfig(const QString& configPath = "assets/config.json");

    /**
     * @brief 获取资产路径
     */
    QString getAssetPath(const QString& assetName) const;

    /**
     * @brief 获取尺寸配置
     */
    int getSize(const QString& sizeName) const;

    /**
     * @brief 获取游戏配置
     */
    int getGameInt(const QString& key) const;
    double getGameDouble(const QString& key) const;

   private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    QJsonObject configObject;
    bool loaded = false;
};

#endif  // CONFIGMANAGER_H