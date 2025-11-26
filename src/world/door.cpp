#include "door.h"
#include "../core/resourcefactory.h"
#include "../core/configmanager.h"
#include <QDebug>

Door::Door(Direction dir, QGraphicsItem *parent)
        : QGraphicsPixmapItem(parent), m_direction(dir), m_state(Closed), m_currentFrame(0) {
    loadImages();
    setPixmap(m_closedImage);
    setZValue(50); // 确保门在其他物体之上

    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, [this]() {
        if (m_currentFrame < m_animationFrames.size()) {
            setPixmap(m_animationFrames[m_currentFrame]);
            m_currentFrame++;
        } else {
            m_animationTimer->stop();
            setPixmap(m_openImage);
            m_state = Open;
            emit openingFinished();
        }
    });

    m_fadeAnimation = new QPropertyAnimation(this, "opacity");
}

Door::~Door() {
    if (m_animationTimer) {
        m_animationTimer->stop();
    }
}

void Door::loadImages() {
    try {
        // 根据门的方向加载对应的图片
        QString dirName;
        switch (m_direction) {
            case Up:
                dirName = "up";
                break;
            case Down:
                dirName = "down";
                break;
            case Left:
                dirName = "left";
                break;
            case Right:
                dirName = "right";
                break;
        }

        // 直接构建assets路径（不通过ConfigManager）
        QString basePath = "assets/door/" + dirName + "/";

        // 加载原始图片
        QString closedPath = basePath + QString("door_%1_closed.png").arg(dirName);
        QPixmap originalClosed = ResourceFactory::loadImage(closedPath);

        QString openPath = basePath + QString("door_%1_open.png").arg(dirName);
        QPixmap originalOpen = ResourceFactory::loadImage(openPath);

        QString halfPath = basePath + QString("door_%1_openhalf.png").arg(dirName);
        QPixmap originalHalf = ResourceFactory::loadImage(halfPath);

        // 根据方向确定缩放尺寸
        int targetWidth, targetHeight;
        if (m_direction == Up || m_direction == Down) {
            // 上下门：横向门，宽度较大
            targetWidth = 120; // 门的宽度
            targetHeight = 80; // 门的高度
        } else {
            // 左右门：竖向门，高度较大
            targetWidth = 80;   // 门的宽度
            targetHeight = 120; // 门的高度
        }

        // 缩放图片到合适尺寸
        m_closedImage = originalClosed.scaled(targetWidth, targetHeight,
                                              Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        m_openImage = originalOpen.scaled(targetWidth, targetHeight,
                                          Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QPixmap halfImage = originalHalf.scaled(targetWidth, targetHeight,
                                                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        // 创建动画序列：关闭 -> 半开 -> 半开 -> 打开 -> 打开
        m_animationFrames.clear();
        m_animationFrames.append(m_closedImage); // 第1帧：关闭
        m_animationFrames.append(halfImage);     // 第2帧：半开
        m_animationFrames.append(halfImage);     // 第3帧：半开（停留）
        m_animationFrames.append(m_openImage);   // 第4帧：打开
        m_animationFrames.append(m_openImage);   // 第5帧：打开（停留）

        qDebug() << "门图片加载完成:" << dirName << "方向，缩放至" << targetWidth << "x" << targetHeight;
    }
    catch (const QString &error) {
        qWarning() << "加载门图片失败:" << error;
        // 创建简单的占位图
        m_closedImage = QPixmap(80, 80);
        m_closedImage.fill(Qt::darkRed);
        m_openImage = QPixmap(80, 80);
        m_openImage.fill(Qt::darkGreen);

        // 创建简单的动画帧
        for (int i = 0; i < 5; i++) {
            QPixmap frame(80, 80);
            int red = 255 - (i * 50);
            int green = (i * 50);
            frame.fill(QColor(red, green, 0));
            m_animationFrames.append(frame);
        }
    }
}

void Door::open() {
    if (m_state != Closed) {
        return;
    }

    qDebug() << "开始播放开门动画";
    m_state = Opening;
    playOpeningAnimation();
}

void Door::setOpenState() {
    // 直接设置为打开状态，不播放动画
    if (m_state == Open) {
        return;
    }

    m_state = Open;
    setPixmap(m_openImage);
    qDebug() << "门设置为打开状态（无动画）";
}

void Door::playOpeningAnimation() {
    m_currentFrame = 0;

    // 使用逐帧动画，5帧动画总时长约400ms
    // 每帧80ms，让开门动画流畅但不会太快
    m_animationTimer->start(80);

    qDebug() << "开门动画启动，总帧数:" << m_animationFrames.size();
}
