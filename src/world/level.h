#include <QObject>
#include <QTimer>
#include <QVector>
#include <QPointer>
#include <QPropertyAnimation>
#include "room.h"
#include "levelconfig.h"

class Player;
class Enemy;
class Boss;
class Chest;
class QGraphicsScene;

class Level : public QObject
{
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
    void clearSceneEntities();

    void showLevelStartText(LevelConfig &config);
    void openDoors(Room *cur);

    void showCredits(const QStringList &desc);

signals:
    void levelCompleted(int levelNumber);
    void roomEntered(int roomIndex);
    void enemiesCleared(int roomIndex, bool up = false, bool down = false,
                        bool left = false, bool right = false);

private:
    void initCurrentRoom(Room *room);
    void spawnEnemiesInRoom(int roomIndex);
    void spawnChestsInRoom(int roomIndex);
    void spawnDoors(const RoomConfig &roomCfg);
    void buildMinimapData(); // New method

    int m_levelNumber;
    QVector<Room *> m_rooms;
    int m_currentRoomIndex;
    Player *m_player;
    QGraphicsScene *m_scene;
    QVector<QPointer<Enemy>> m_currentEnemies;
    QVector<QPointer<Chest>> m_currentChests;
    QVector<QGraphicsItem *> m_doorItems;
    QVector<bool> visited;
    QTimer *checkChange;
    int visited_count;

private slots:
    void onEnemyDying(Enemy *enemy);
};
