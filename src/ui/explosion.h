#ifndef EXPLOSION_H
#define EXPLOSION_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QTimer>
#include <QVector>
#include <QPixmap>

class Explosion : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    Explosion(QGraphicsItem *parent = nullptr);
    void startAnimation();

private slots:
    void nextFrame();

private:
    QVector<QPixmap> m_frames;
    int m_currentFrame;
    QTimer *m_animationTimer;
};

#endif