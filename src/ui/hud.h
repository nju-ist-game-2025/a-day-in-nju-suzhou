#ifndef HUD_H
#define HUD_H

#include <QGraphicsItem>
#include <QObject>

class HUD : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    HUD(QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    
public slots:
    void updateHealth(float current, float max);

private:
    float currentHealth;
    float maxHealth;
};

#endif // HUD_H
