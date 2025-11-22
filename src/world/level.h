#include <QObject>
#include <QTimer>
#include <QVector>
#include <QPointer>
#include "room.h"

class Player;
class Enemy;
class Boss;
class Chest;
class QGraphicsScene;

class Level : public QObject {
Q_OBJECT

public:
    explicit Level(Player *player, QGraphicsScene *scene, QObject *parent = nullptr);
    ~Level() override;

    void init(int levelNumber);
    int currentLevel() const { return m_levelNumber; }
    bool isAllRoomsCompleted() const;
    bool enterNextRoom();
    Room *currentRoom() const;
    void loadRoom(int roomIndex);
    void onPlayerDied();
    void bonusEffects();
    void clearCurrentRoomEntities();

    void showLevelStartText(int levelNum);

signals:
    void levelCompleted(int levelNumber);
    void roomEntered(int roomIndex);
    void enemiesCleared(int roomIndex);

private:
    void initCurrentRoom(Room *room);
    void spawnEnemiesInRoom(int roomIndex);
    void spawnChestsInRoom(int roomIndex);

    int m_levelNumber;
    QVector<Room *> m_rooms;
    int m_currentRoomIndex;
    Player *m_player;
    QGraphicsScene *m_scene;
    QVector<QPointer<Enemy>> m_currentEnemies;
    QVector<QPointer<Chest>> m_currentChests;
    QVector<bool> visited;
    QTimer *checkChange;
    int visited_count;

private slots:
    void onEnemyDying(Enemy *enemy);
};
