#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <QObject>
#include <QTimer>

class GameController : public QObject {
Q_OBJECT
public:
    explicit GameController(QObject *parent = nullptr);

signals:
};

#endif // GAMECONTROLLER_H
