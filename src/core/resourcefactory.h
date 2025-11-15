#ifndef RESOURCEFACTORY_H
#define RESOURCEFACTORY_H

#include <QColor>
#include <QDebug>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QString>

/**
 * @brief 资源工厂类 - 封装各种图形资源的创建和加载
 * 避免代码重复，便于维护和修改
 * 支持从文件加载PNG图片或使用默认图形
 */
class ResourceFactory {
   public:
    /**
     * @brief 从文件加载图片，如果失败则抛出错误
     * @param filePath 图片文件路径（相对于exe或绝对路径）
     * @return 加载的QPixmap
     * @throws QString 加载失败时抛出错误信息
     */
    static QPixmap loadImage(const QString& filePath) {
        if (!QFile::exists(filePath)) {
            QString errorMsg = QString("资源文件不存在: %1").arg(filePath);
            qCritical() << errorMsg;
            throw errorMsg;
        }

        QPixmap pixmap(filePath);
        if (pixmap.isNull()) {
            QString errorMsg = QString("无法加载图片: %1").arg(filePath);
            qCritical() << errorMsg;
            throw errorMsg;
        }

        return pixmap;
    }

    /**
     * @brief 从文件加载图片并缩放到指定尺寸
     * @param filePath 图片文件路径
     * @param width 目标宽度
     * @param height 目标高度
     * @return 缩放后的QPixmap
     * @throws QString 加载失败时抛出错误信息
     */
    static QPixmap loadImageScaled(const QString& filePath, int width, int height) {
        QPixmap pixmap = loadImage(filePath);
        if (pixmap.width() != width || pixmap.height() != height) {
            return pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        return pixmap;
    }

    /**
     * @brief 创建圆形图像（默认图形）
     */
    static QPixmap createCircle(int size, const QColor& fillColor, const QColor& borderColor = Qt::transparent, int borderWidth = 0) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(fillColor);

        if (borderWidth > 0 && borderColor != Qt::transparent) {
            painter.setPen(QPen(borderColor, borderWidth));
        } else {
            painter.setPen(Qt::NoPen);
        }

        painter.drawEllipse(0, 0, size - 1, size - 1);
        painter.end();

        return pixmap;
    }

    /**
     * @brief 加载玩家图像
     * @throws QString 加载失败时抛出错误信息
     */
    static QPixmap createPlayerImage(int size, const QString& imagePath) {
        return loadImageScaled(imagePath, size, size);
    }

    /**
     * @brief 加载子弹图像
     * @throws QString 加载失败时抛出错误信息
     */
    static QPixmap createBulletImage(int size, const QString& imagePath) {
        return loadImageScaled(imagePath, size, size);
    }

    static QPixmap createChestImage(int size, const QString& imagePath) {
        return loadImageScaled(imagePath, size, size);
    }

    /**
     * @brief 加载敌人图像
     * @throws QString 加载失败时抛出错误信息
     */
    static QPixmap createEnemyImage(int size, const QString& imagePath) {
        return loadImageScaled(imagePath, size, size);
    }

    /**
     * @brief 加载背景图片
     * @throws QString 加载失败时抛出错误信息
     */
    static QPixmap loadBackgroundImage(const QString& imagePath, int width = 800, int height = 600) {
        QPixmap pixmap = loadImage(imagePath);
        return pixmap.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
};

#endif  // RESOURCEFACTORY_H
