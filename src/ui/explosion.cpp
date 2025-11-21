#include "explosion.h"
#include <QDebug>
#include <QPainter>
#include <QGraphicsScene>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include "../core/resourcefactory.h"
#include "../core/configmanager.h"

Explosion::Explosion(QGraphicsItem *parent)
    : QObject(), QGraphicsPixmapItem(parent),
      m_currentFrame(0),
      m_animationTimer(new QTimer(this))
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            QString path = QString("assets/explosion/cell_%1_%2.png").arg(i).arg(j);
            QPixmap frame(path);
            if (!frame.isNull()) {
                m_frames.append(frame);
            } else {
                qWarning() << "加载爆炸帧失败:" << path;
                QPixmap placeholder(50, 50);
                placeholder.fill(Qt::transparent);
                QPainter painter(&placeholder);
                painter.setBrush(Qt::red);
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(5, 5, 40, 40);
                painter.end();
                m_frames.append(placeholder);
            }
        }
    }
    
    if (!m_frames.isEmpty()) {
        setPixmap(m_frames.first());
        setTransformOriginPoint(boundingRect().center());
    }
    
    connect(m_animationTimer, &QTimer::timeout, this, &Explosion::nextFrame);
}

void Explosion::startAnimation()
{
    if (m_frames.isEmpty()) return;
    
    m_currentFrame = 0;
    setPixmap(m_frames.first());
    m_animationTimer->start(50);
}

void Explosion::nextFrame()
{
    m_currentFrame++;
    
    if (m_currentFrame >= m_frames.size()) {
        m_animationTimer->stop();
        qDebug() << "爆炸动画播放完成";
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
        return;
    }
    
    setPixmap(m_frames[m_currentFrame]);
    
    qreal scale = 1.0 + (m_currentFrame * 0.5 / m_frames.size());
    setScale(scale);
}