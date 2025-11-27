#include "configmanager.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

ConfigManager &ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const QString &configPath) {
    QFile configFile(configPath);
    if (!configFile.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开配置文件:" << configPath;
        return false;
    }

    QByteArray configData = configFile.readAll();
    configFile.close();

    QJsonParseError parseError;
    QJsonDocument configDoc = QJsonDocument::fromJson(configData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << parseError.errorString();
        return false;
    }

    if (!configDoc.isObject()) {
        qWarning() << "配置文件根元素不是对象";
        return false;
    }

    configObject = configDoc.object();
    loaded = true;
    qDebug() << "配置文件加载成功:" << configPath;
    return true;
}

QString ConfigManager::getAssetPath(const QString &assetName) const {
    if (!loaded) {
        qWarning() << "配置文件未加载";
        return {};
    }

    QJsonObject assets = configObject.value("assets").toObject();
    return assets.value(assetName).toString();
}

void ConfigManager::setAssetPath(const QString &assetName, const QString &path) {
    if (!loaded) {
        qWarning() << "配置文件未加载";
        return;
    }

    QJsonObject assets = configObject.value("assets").toObject();
    assets[assetName] = path;
    configObject["assets"] = assets;
}

bool ConfigManager::saveConfig(const QString &configPath) {
    if (!loaded) {
        qWarning() << "配置文件未加载，无法保存";
        return false;
    }

    QFile configFile(configPath);
    if (!configFile.open(QIODevice::WriteOnly)) {
        qWarning() << "无法打开配置文件进行写入:" << configPath;
        return false;
    }

    QJsonDocument configDoc(configObject);
    configFile.write(configDoc.toJson(QJsonDocument::Indented));
    configFile.close();

    qDebug() << "配置文件保存成功:" << configPath;
    return true;
}

int ConfigManager::getSize(const QString &sizeName) const {
    if (!loaded) {
        qWarning() << "配置文件未加载";
        return 0;
    }

    QJsonObject sizes = configObject.value("sizes").toObject();
    return sizes.value(sizeName).toInt();
}

int ConfigManager::getEntitySize(const QString &category, const QString &typeName) const {
    if (!loaded) {
        qWarning() << "配置文件未加载";
        return 0;
    }

    QJsonObject sizes = configObject.value("sizes").toObject();

    // 获取分类对象（players, enemies, bosses）
    if (sizes.contains(category) && sizes.value(category).isObject()) {
        QJsonObject categoryObj = sizes.value(category).toObject();

        // 先尝试获取具体类型的尺寸
        if (categoryObj.contains(typeName)) {
            return categoryObj.value(typeName).toInt();
        }

        // 如果没有具体类型，返回该分类的默认值
        if (categoryObj.contains("default")) {
            return categoryObj.value("default").toInt();
        }
    }

    // 回退到旧的简单配置（兼容性）
    // players -> player, enemies -> enemy, bosses -> boss
    QString fallbackKey = category;
    if (category == "players")
        fallbackKey = "player";
    else if (category == "enemies")
        fallbackKey = "enemy";
    else if (category == "bosses")
        fallbackKey = "boss";

    return sizes.value(fallbackKey).toInt();
}

int ConfigManager::getBulletSize(const QString &bulletType) const {
    if (!loaded) {
        qWarning() << "配置文件未加载";
        return 20;  // 默认子弹大小
    }

    QJsonObject sizes = configObject.value("sizes").toObject();

    // 获取 bullets 分类
    if (sizes.contains("bullets") && sizes.value("bullets").isObject()) {
        QJsonObject bulletsObj = sizes.value("bullets").toObject();

        // 先尝试获取具体类型的子弹尺寸
        if (bulletsObj.contains(bulletType)) {
            return bulletsObj.value(bulletType).toInt();
        }

        // 如果没有具体类型，返回默认值
        if (bulletsObj.contains("default")) {
            return bulletsObj.value("default").toInt();
        }
    }

    // 回退到旧的 bullet 配置（兼容性）
    if (sizes.contains("bullet")) {
        return sizes.value("bullet").toInt();
    }

    return 20;  // 最终默认值
}

int ConfigManager::getGameInt(const QString &key) const {
    if (!loaded) {
        qWarning() << "配置文件未加载";
        return 0;
    }

    QJsonObject game = configObject.value("game").toObject();
    return game.value(key).toInt();
}

double ConfigManager::getGameDouble(const QString &key) const {
    if (!loaded) {
        qWarning() << "配置文件未加载";
        return 0.0;
    }

    QJsonObject game = configObject.value("game").toObject();
    return game.value(key).toDouble();
}

bool ConfigManager::isDevModeEnabled() const {
    if (!loaded) {
        qWarning() << "配置文件未加载";
        return false;
    }

    QJsonObject devMode = configObject.value("developer_mode").toObject();
    return devMode.value("enabled").toBool(false);
}