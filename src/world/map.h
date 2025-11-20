#pragma once

#include <QString>
#include <QGraphicsScene>
#include <QVector>

class Wall;

class Map {
public:
    Map() = default;

    ~Map();

    bool loadFromFile(const QString &filePath, QGraphicsScene *scene);

    void clear();

private:
    QGraphicsScene *m_scene = nullptr;
    QVector<Wall *> m_walls;
};
