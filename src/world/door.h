#ifndef DOOR_H
#define DOOR_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPropertyAnimation>
#include <QTimer>

class Door : public QObject, public QGraphicsPixmapItem {
Q_OBJECT

public:
    enum DoorState {
        Closed,
        Opening,
        Open
    };

    enum Direction {
        Up,
        Down,
        Left,
        Right
    };

    Door(Direction dir, QGraphicsItem *parent = nullptr);

    ~Door();

    void open();

    void setOpenState(); // 直接设置为打开状态（用于邻房间）
    bool isOpen() const { return m_state == Open; }

    DoorState state() const { return m_state; }

    Direction direction() const { return m_direction; } // 获取门的方向

private:
    void loadImages();

    void playOpeningAnimation();

    Direction m_direction;
    DoorState m_state;

    QPixmap m_closedImage;
    QPixmap m_openImage;
    QList<QPixmap> m_animationFrames; // 开门动画帧

    QTimer *m_animationTimer;
    int m_currentFrame;
    QPropertyAnimation *m_fadeAnimation;

signals:

    void openingFinished();
};

#endif // DOOR_H
