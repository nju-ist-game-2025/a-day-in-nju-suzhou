#ifndef HUD_H
#define HUD_H

#include <QDebug>
#include <QGraphicsItem>
#include <QObject>
#include "player.h"

class HUD : public QObject, public QGraphicsItem {
Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    HUD(Player *pl, QGraphicsItem *parent = nullptr);

    ~HUD() override;

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void paintShield(QPainter *painter);

    void paintBlack(QPainter *painter);

    void paintFrostChance(QPainter *painter);

    void paintKey(QPainter *painter);

    void paintEffects(QPainter *painter, const QString &text, int count, double duration, QColor color = Qt::black);

    void paintMinimap(QPainter *painter);

    void paintTeleportCooldown(QPainter *painter);

    void paintUltimateStatus(QPainter *painter);

    struct RoomNode {
        int id;
        int x, y;
        bool visited;
        bool hasBoss;  // 是否为boss房间
        int up, down, left, right;
    };

public slots:

    void updateHealth(float current, float max);

    void updateMinimap(int currentRoom, const QVector<int> &roomLayout);
    void triggerDamageFlash();

    void setMapLayout(const QVector<RoomNode> &nodes);
    void syncVisitedRooms(const QVector<bool> &visitedArray);  // 同步已访问房间状态

private slots:

    void endDamageFlash();

private:
    float currentHealth;
    float maxHealth;
    bool isFlashing;
    bool isScreenFlashing;
    QTimer *flashTimer;
    int flashCount;
    QTimer *screenFlashTimer;
    Player *player;
    int currentRoomIndex;
    QTimer *m_hudTimer;  // HUD刷新定时器

    QVector<RoomNode> mapNodes;
};

#endif  // HUD_H
