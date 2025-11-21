#ifndef HUD_H
#define HUD_H

#include <QGraphicsItem>
#include <QObject>
#include "player.h"

class HUD : public QObject, public QGraphicsItem {
Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    HUD(Player *pl, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void paintSoul(QPainter *painter);

    void paintBlack(QPainter *painter);

    void paintKey(QPainter *painter);

    void paintEffects(QPainter *painter, const QString& text, int count, double duration, QColor color = Qt::black);
public slots:

    void updateHealth(float current, float max);

    void triggerDamageFlash();

private slots:

    void endDamageFlash();

private:
    float currentHealth;
    float maxHealth;
    bool isFlashing;
    bool isScreenFlashing;
    QTimer *flashTimer;
    int flashCount;
    QTimer *screenFlashTimer;
    Player *player;
};

#endif // HUD_H
