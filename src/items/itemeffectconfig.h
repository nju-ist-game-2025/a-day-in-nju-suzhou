#ifndef ITEMEFFECTCONFIG_H
#define ITEMEFFECTCONFIG_H

#include <QColor>
#include <QJsonObject>
#include <QMap>
#include <QString>

/**
 * @brief 单个道具的效果配置
 */
struct ItemEffectData {
    QString name;              // 道具名称
    QString description;       // 道具描述
    QString effectType;        // 效果类型
    QJsonObject effectParams;  // 效果参数
    QString pickupText;        // 拾取提示文字
    QString pickupTextFull;    // 已满时的提示文字
    QString pickupTextMax;     // 达到上限时的提示文字
    QColor color;              // 提示文字颜色

    // 常用效果参数的便捷访问
    int getValue() const { return effectParams.value("value").toInt(1); }
    double getMultiplier() const { return effectParams.value("multiplier").toDouble(1.0); }
    double getMaxMultiplier() const { return effectParams.value("maxMultiplier").toDouble(1.0); }
    int getMaxValue() const { return effectParams.value("maxValue").toInt(100); }
    int getMaxHealthBonus() const { return effectParams.value("maxHealthBonus").toInt(0); }
    int getCurrentHealthBonus() const { return effectParams.value("currentHealthBonus").toInt(0); }
    int getBaseCooldown() const { return effectParams.value("baseCooldown").toInt(150); }
    int getHealPerHeart() const { return effectParams.value("healPerHeart").toInt(6); }

    // 寒冰效果参数
    double getSlowFactor() const { return effectParams.value("slowFactor").toDouble(0.7); }
    double getSlowDuration() const { return effectParams.value("slowDuration").toDouble(2.0); }
    int getMaxSlowStacks() const { return effectParams.value("maxSlowStacks").toInt(2); }
};

/**
 * @brief 道具效果配置管理器（单例）
 *
 * 从JSON配置文件读取道具效果配置
 */
class ItemEffectConfig {
   public:
    /**
     * @brief 获取单例实例
     */
    static ItemEffectConfig& instance();

    /**
     * @brief 加载配置文件
     * @param filePath 配置文件路径
     * @return 是否加载成功
     */
    bool loadConfig(const QString& filePath = "assets/item_effects.json");

    /**
     * @brief 获取道具效果配置
     * @param itemKey 道具键名（如 "red_heart", "damage_boost"）
     * @return 道具效果配置，如果不存在则返回默认配置
     */
    ItemEffectData getItemEffect(const QString& itemKey) const;

    /**
     * @brief 检查配置是否已加载
     */
    bool isLoaded() const { return m_loaded; }

    /**
     * @brief 格式化拾取提示文字
     * @param text 原始文字（包含{value}等占位符）
     * @param params 参数映射
     * @return 格式化后的文字
     */
    static QString formatText(const QString& text, const QMap<QString, QString>& params);

   private:
    ItemEffectConfig() = default;
    ~ItemEffectConfig() = default;
    ItemEffectConfig(const ItemEffectConfig&) = delete;
    ItemEffectConfig& operator=(const ItemEffectConfig&) = delete;

    QMap<QString, ItemEffectData> m_itemEffects;  // 道具效果映射
    bool m_loaded = false;
};

#endif  // ITEMEFFECTCONFIG_H
