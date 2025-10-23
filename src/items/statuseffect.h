#ifndef STATUSEFFECT_H
#define STATUSEFFECT_H

#include <QObject>

class StatusEffect : public QObject
{
    Q_OBJECT
public:
    explicit StatusEffect(QObject *parent = nullptr);

signals:
};

#endif // STATUSEFFECT_H
