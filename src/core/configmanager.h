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
    [[nodiscard]] QString getAssetPath(const QString& assetName) const;

    /**
     * @brief 设置资产路径
     */
    void setAssetPath(const QString& assetName, const QString& path);

    /**
     * @brief 保存配置到文件
     */
    bool saveConfig(const QString& configPath = "assets/config.json");

    /**
     * @brief 获取尺寸配置
     */
    [[nodiscard]] int getSize(const QString& sizeName) const;

    /**
     * @brief 获取特定类型实体的尺寸
     * @param category 分类：players, enemies, bosses
     * @param typeName 具体类型名称，如 clock_normal, nightmare
     * @return 尺寸值，如果找不到则返回该分类的默认值
     */
    [[nodiscard]] int getEntitySize(const QString& category, const QString& typeName) const;

    /**
     * @brief 获取子弹尺寸
     * @param bulletType 子弹类型名称：player, sock_shooter, boss_washmachine 等
     * @return 尺寸值，如果找不到则返回默认值
     */
    [[nodiscard]] int getBulletSize(const QString& bulletType) const;

    /**
     * @brief 获取游戏配置
     */
    [[nodiscard]] int getGameInt(const QString& key) const;

    [[nodiscard]] double getGameDouble(const QString& key) const;

    /**
     * @brief 检查开发者模式是否启用
     * @return 是否启用开发者模式
     */
    [[nodiscard]] bool isDevModeEnabled() const;

    /**
     * @brief 检查配置验证是否启用
     * @return 是否启用配置验证
     */
    [[nodiscard]] bool isConfigValidationEnabled() const;

    // ============== 玩家配置 ==============
    /**
     * @brief 获取玩家配置的整数值
     * @param key 配置键名，如 health, shoot_cooldown
     * @param defaultValue 默认值
     */
    [[nodiscard]] int getPlayerInt(const QString& key, int defaultValue = 0) const;

    /**
     * @brief 获取玩家配置的浮点数值
     * @param key 配置键名，如 speed, teleport_distance
     * @param defaultValue 默认值
     */
    [[nodiscard]] double getPlayerDouble(const QString& key, double defaultValue = 0.0) const;

    // ============== 敌人配置 ==============
    /**
     * @brief 获取敌人配置的整数值
     * @param enemyType 敌人类型，如 clock_normal, sock_shooter
     * @param key 配置键名，如 health, contact_damage
     * @param defaultValue 默认值
     */
    [[nodiscard]] int getEnemyInt(const QString& enemyType, const QString& key, int defaultValue = 0) const;

    /**
     * @brief 获取敌人配置的浮点数值
     * @param enemyType 敌人类型
     * @param key 配置键名，如 speed, vision_range
     * @param defaultValue 默认值
     */
    [[nodiscard]] double getEnemyDouble(const QString& enemyType, const QString& key, double defaultValue = 0.0) const;

    /**
     * @brief 获取敌人配置的字符串值
     * @param enemyType 敌人类型
     * @param key 配置键名，如 move_pattern, description
     * @param defaultValue 默认值
     */
    [[nodiscard]] QString getEnemyString(const QString& enemyType, const QString& key, const QString& defaultValue = "") const;

    // ============== Boss配置 ==============
    /**
     * @brief 获取Boss指定阶段配置的整数值
     * @param bossType Boss类型，如 nightmare, washmachine, teacher
     * @param phase 阶段，如 phase1, phase2, phase3
     * @param key 配置键名
     * @param defaultValue 默认值
     */
    [[nodiscard]] int getBossInt(const QString& bossType, const QString& phase, const QString& key, int defaultValue = 0) const;

    /**
     * @brief 获取Boss指定阶段配置的浮点数值
     * @param bossType Boss类型
     * @param phase 阶段
     * @param key 配置键名
     * @param defaultValue 默认值
     */
    [[nodiscard]] double getBossDouble(const QString& bossType, const QString& phase, const QString& key, double defaultValue = 0.0) const;

    /**
     * @brief 获取Boss配置的字符串值（非阶段相关）
     * @param bossType Boss类型
     * @param key 配置键名
     * @param defaultValue 默认值
     */
    [[nodiscard]] QString getBossString(const QString& bossType, const QString& key, const QString& defaultValue = "") const;

   private:
    ConfigManager() = default;

    ~ConfigManager() = default;

    ConfigManager(const ConfigManager&) = delete;

    ConfigManager& operator=(const ConfigManager&) = delete;

    QJsonObject configObject;
    bool loaded = false;
};

#endif  // CONFIGMANAGER_H