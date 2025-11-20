#include "map.h"
#include "wall.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>

Map::~Map() {
    clear();
}

void Map::clear() {
    for (Wall *w: m_walls) {
        if (m_scene && w->scene() == m_scene) {
            m_scene->removeItem(w);
        }
        delete w;
    }
    m_walls.clear();
    m_scene = nullptr;
}

bool Map::loadFromFile(const QString &filePath, QGraphicsScene *scene) {
    if (!scene) return false;
    clear();
    m_scene = scene;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开地图文件:" << filePath;
        return false;
    }
    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "解析地图 JSON 错误:" << err.errorString();
        return false;
    }
    const QJsonObject root = doc.object();

    if (root.contains("walls") && root.value("walls").isArray()) {
        const QJsonArray walls = root.value("walls").toArray();
        for (const QJsonValue &v: walls) {
            if (!v.isObject()) continue;
            const QJsonObject o = v.toObject();
            double x = o.value("x").toDouble();
            double y = o.value("y").toDouble();
            double w = o.value("w").toDouble();
            double h = o.value("h").toDouble();
            Wall *wall = new Wall(QRectF(x, y, w, h));
            m_walls.append(wall);
            m_scene->addItem(wall);
        }
    } else {
        qDebug() << "地图 JSON 中没有找到 \"walls\" 数组:" << filePath;
    }

    return true;
}
