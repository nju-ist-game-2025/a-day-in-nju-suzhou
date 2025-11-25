#ifndef EXPLOSION_H
#define EXPLOSION_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPixmap>
#include <QTimer>
#include <QVector>

class Explosion : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
   public:
    Explosion(QGraphicsItem* parent = nullptr);
    void startAnimation();

    // 预加载爆炸帧（在游戏初始化时调用一次）
    static void preloadFrames();
    static bool isFramesLoaded() { return s_framesLoaded; }

   private slots:
    void nextFrame();

   private:
    int m_currentFrame;
    QTimer* m_animationTimer;

    // 静态缓存，所有爆炸实例共享
    static QVector<QPixmap> s_frames;
    static bool s_framesLoaded;
};

#endif