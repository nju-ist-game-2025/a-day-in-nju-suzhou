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
    static ConfigManager &instance();

    /**
     * @brief 加载配置文件
     * @param configPath 配置文件路径
     * @return 是否加载成功
     */
    bool loadConfig(const QString &configPath = "assets/config.json");

    /**
     * @brief 获取资产路径
     */
    [[nodiscard]] QString getAssetPath(const QString &assetName) const;

    /**
     * @brief 设置资产路径
     */
    void setAssetPath(const QString &assetName, const QString &path);

    /**
     * @brief 保存配置到文件
     */
    bool saveConfig(const QString &configPath = "assets/config.json");

    /**
     * @brief 获取尺寸配置
     */
    [[nodiscard]] int getSize(const QString &sizeName) const;

    /**
     * @brief 获取特定类型实体的尺寸
     * @param category 分类：players, enemies, bosses
     * @param typeName 具体类型名称，如 clock_normal, nightmare
     * @return 尺寸值，如果找不到则返回该分类的默认值
     */
    [[nodiscard]] int getEntitySize(const QString &category, const QString &typeName) const;

    /**
     * @brief 获取子弹尺寸
     * @param bulletType 子弹类型名称：player, sock_shooter, boss_washmachine 等
     * @return 尺寸值，如果找不到则返回默认值
     */
    [[nodiscard]] int getBulletSize(const QString &bulletType) const;

    /**
     * @brief 获取游戏配置
     */
    [[nodiscard]] int getGameInt(const QString &key) const;

    [[nodiscard]] double getGameDouble(const QString &key) const;

private:
    ConfigManager() = default;

    ~ConfigManager() = default;

    ConfigManager(const ConfigManager &) = delete;

    ConfigManager &operator=(const ConfigManager &) = delete;

    QJsonObject configObject;
    bool loaded = false;
};

#endif // CONFIGMANAGER_H