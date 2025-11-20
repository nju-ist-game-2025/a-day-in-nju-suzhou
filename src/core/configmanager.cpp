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
        return QString();
    }

    QJsonObject assets = configObject.value("assets").toObject();
    return assets.value(assetName).toString();
}

int ConfigManager::getSize(const QString &sizeName) const {
    if (!loaded) {
        qWarning() << "配置文件未加载";
        return 0;
    }

    QJsonObject sizes = configObject.value("sizes").toObject();
    return sizes.value(sizeName).toInt();
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