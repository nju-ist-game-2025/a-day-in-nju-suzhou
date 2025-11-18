#include "hud.h"
#include <QPainter>
#include <QFont>
#include <QDebug>

HUD::HUD(QGraphicsItem* parent) 
    : QGraphicsItem(parent), currentHealth(3.0f), maxHealth(3.0f) {
    setPos(10, 10);
}

QRectF HUD::boundingRect() const {
    return QRectF(0, 0, 200, 60);
}

void HUD::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    painter->setBrush(QColor(50, 50, 50, 200));
    painter->setPen(QPen(Qt::black, 2));
    painter->drawRect(0, 0, 150, 25);

    if (currentHealth > 0) {
        float healthWidth = (currentHealth / maxHealth) * 150;
        painter->setBrush(Qt::red);
        painter->setPen(QPen(Qt::darkRed, 1));
        painter->drawRect(0, 0, healthWidth, 25);
    }
    
    painter->setPen(Qt::white);
    QFont font = painter->font();
    font.setPointSize(10);
    font.setBold(true);
    painter->setFont(font);
    painter->drawText(QRect(0, 0, 150, 25), Qt::AlignCenter, 
                     QString("%1 / %2").arg(currentHealth).arg(maxHealth));
    
    painter->setPen(Qt::yellow);
    painter->drawText(QRect(0, 30, 200, 20), Qt::AlignLeft, "生命值:");
}

void HUD::updateHealth(float current, float max) {
    currentHealth = qMax(0.0f, current);
    maxHealth = qMax(1.0f, max);
    
    qDebug() << "HUD更新: 当前血量" << currentHealth << "/" << maxHealth;
    
    update();
}
