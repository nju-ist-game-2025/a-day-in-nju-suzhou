#include "explosion.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QGraphicsScene>
#include <QPainter>
#include <QTimer>
#include "../core/configmanager.h"
#include "../core/resourcefactory.h"

// 静态成员初始化
QVector<QPixmap> Explosion::s_frames;
bool Explosion::s_framesLoaded = false;

void Explosion::preloadFrames() {
    if (s_framesLoaded)
        return;

    s_frames.clear();
    s_frames.reserve(16);  // 预分配空间

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            QString path = QString("assets/explosion/cell_%1_%2.png").arg(i).arg(j);
            QPixmap frame(path);
            if (!frame.isNull()) {
                s_frames.append(frame);
            } else {
                qWarning() << "加载爆炸帧失败:" << path;
                QPixmap placeholder(50, 50);
                placeholder.fill(Qt::transparent);
                QPainter painter(&placeholder);
                painter.setBrush(Qt::red);
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(5, 5, 40, 40);
                painter.end();
                s_frames.append(placeholder);
            }
        }
    }

    s_framesLoaded = true;
    qDebug() << "爆炸动画帧预加载完成，共" << s_frames.size() << "帧";
}

Explosion::Explosion(QGraphicsItem *parent)
        : QObject(), QGraphicsPixmapItem(parent), m_currentFrame(0), m_animationTimer(new QTimer(this)) {
    // 确保帧已加载（如果没有预加载，这里会加载）
    if (!s_framesLoaded) {
        preloadFrames();
    }

    if (!s_frames.isEmpty()) {
        setPixmap(s_frames.first());
        setTransformOriginPoint(boundingRect().center());
    }

    connect(m_animationTimer, &QTimer::timeout, this, &Explosion::nextFrame);
}

void Explosion::startAnimation() {
    if (s_frames.isEmpty())
        return;

    m_currentFrame = 0;
    setPixmap(s_frames.first());
    m_animationTimer->start(50);
}

void Explosion::nextFrame() {
    m_currentFrame++;

    if (m_currentFrame >= s_frames.size()) {
        m_animationTimer->stop();
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
        return;
    }

    setPixmap(s_frames[m_currentFrame]);

    qreal scale = 1.0 + (m_currentFrame * 0.5 / s_frames.size());
    setScale(scale);
}