#include "boss.h"

Boss::Boss(const QPixmap& pic, double scale)
    : Enemy(pic, scale) {
    setHealth(300);           // 更高血量
    setContactDamage(5);      // 更高伤害
    setVisionRange(1000);     // 全图视野
    setAttackRange(60);       // 更大攻击范围
    setAttackCooldown(1500);  // 攻击频率略低
    setSpeed(1.5);            // 更笨重

    damageScale = 0.8;  // 伤害减免

    // 使用绕圈接近模式
    setMovementPattern(MOVE_CIRCLE);
    setCircleRadius(120.0);
}

Boss::~Boss() {
    // qDebug() <<"Boss被击败！";
}

// 重写边界框以包含血条区域
QRectF Boss::boundingRect() const {
    QRectF base = QGraphicsPixmapItem::boundingRect();
    return base.adjusted(0, -20, 0, 0);
}

void Boss::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    // 基础图像
    QGraphicsPixmapItem::paint(painter, option, widget);

    // 绘制血条
    drawHealthBar(painter);
}

void Boss::drawHealthBar(QPainter* painter) {
    if (!painter)
        return;

    // 获取当前图像的宽度
    qreal pixmapWidth = pixmap().width();

    // 血条参数
    qreal barWidth = pixmapWidth;  // 血条宽度与boss图像同宽
    qreal barHeight = 8.0;
    qreal barY = -15.0;  // 血条在boss头顶上方15像素
    qreal barX = 0.0;

    // 计算血量百分比
    qreal healthPercent = (maxHealth > 0) ? static_cast<qreal>(health) / maxHealth : 0.0;
    healthPercent = qBound(0.0, healthPercent, 1.0);

    // 保存画笔状态
    painter->save();

    // 绘制血条背景
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(50, 50, 50, 200));
    painter->drawRect(QRectF(barX, barY, barWidth, barHeight));

    // 绘制当前血量（根据血量百分比变色）
    QColor healthColor;
    if (healthPercent > 0.5) {
        healthColor = QColor(50, 200, 50, 220);  // 绿色
    } else if (healthPercent > 0.25) {
        healthColor = QColor(255, 165, 0, 220);  // 橙色
    } else {
        healthColor = QColor(220, 50, 50, 220);  // 红色
    }
    painter->setBrush(healthColor);
    painter->drawRect(QRectF(barX, barY, barWidth * healthPercent, barHeight));

    // 绘制边框
    painter->setPen(QPen(QColor(30, 30, 30), 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(barX, barY, barWidth, barHeight));

    // 恢复画笔状态
    painter->restore();
}
