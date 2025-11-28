#include "itemeffectconfig.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

ItemEffectConfig& ItemEffectConfig::instance() {
    static ItemEffectConfig instance;
    return instance;
}

bool ItemEffectConfig::loadConfig(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ItemEffectConfig: 无法打开配置文件:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "ItemEffectConfig: JSON解析错误:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "ItemEffectConfig: 配置文件格式错误，需要JSON对象";
        return false;
    }

    QJsonObject root = doc.object();
    QJsonObject items = root.value("items").toObject();

    m_itemEffects.clear();

    for (auto it = items.begin(); it != items.end(); ++it) {
        QString key = it.key();
        QJsonObject itemObj = it.value().toObject();

        ItemEffectData effectData;
        effectData.name = itemObj.value("name").toString(key);
        effectData.description = itemObj.value("description").toString();

        QJsonObject effectObj = itemObj.value("effect").toObject();
        effectData.effectType = effectObj.value("type").toString();
        effectData.effectParams = effectObj;

        effectData.pickupText = itemObj.value("pickupText").toString();
        effectData.pickupTextFull = itemObj.value("pickupTextFull").toString();
        effectData.pickupTextMax = itemObj.value("pickupTextMax").toString();

        // 解析颜色
        QString colorStr = itemObj.value("color").toString("#FFFFFF");
        effectData.color = QColor(colorStr);

        m_itemEffects[key] = effectData;
        qDebug() << "ItemEffectConfig: 加载道具配置:" << key << "-" << effectData.name;
    }

    m_loaded = true;
    qDebug() << "ItemEffectConfig: 成功加载" << m_itemEffects.size() << "个道具配置";
    return true;
}

ItemEffectData ItemEffectConfig::getItemEffect(const QString& itemKey) const {
    if (m_itemEffects.contains(itemKey)) {
        return m_itemEffects[itemKey];
    }

    // 返回默认配置
    ItemEffectData defaultData;
    defaultData.name = itemKey;
    defaultData.description = "未知道具";
    defaultData.effectType = "unknown";
    defaultData.pickupText = "获得道具";
    defaultData.color = Qt::white;
    return defaultData;
}

QString ItemEffectConfig::formatText(const QString& text, const QMap<QString, QString>& params) {
    QString result = text;
    for (auto it = params.begin(); it != params.end(); ++it) {
        result.replace("{" + it.key() + "}", it.value());
    }
    return result;
}
