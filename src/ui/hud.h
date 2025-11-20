#ifndef HUD_H
#define HUD_H

#include <QGraphicsItem>
#include <QObject>

class HUD : public QObject, public QGraphicsItem {
Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    HUD(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
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
};

#endif // HUD_H
