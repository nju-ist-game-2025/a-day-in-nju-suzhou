#include "levelconfig.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

LevelConfig::LevelConfig() : m_startRoomIndex(0)
{
}

bool LevelConfig::loadFromFile(int levelNumber)
{
    QString filePath = QString("assets/levels/level%1.json").arg(levelNumber);
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "无法打开关卡配置文件:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "关卡配置 JSON 解析错误:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject())
    {
        qWarning() << "关卡配置文件根元素不是对象";
        return false;
    }

    m_description = readDescriptionsFromJson(filePath);

    return loadFromJson(doc.object());
}

bool LevelConfig::loadFromJson(const QJsonObject &levelObj)
{
    m_rooms.clear();

    // 读取关卡名称
    m_levelName = levelObj.value("name").toString("未命名关卡");

    // 读取起始房间索引
    m_startRoomIndex = levelObj.value("startRoom").toInt(0);

    // 读取房间列表
    QJsonArray roomsArray = levelObj.value("rooms").toArray();
    for (const QJsonValue &roomVal : roomsArray)
    {
        if (roomVal.isObject())
        {
            RoomConfig roomCfg = parseRoomConfig(roomVal.toObject());
            m_rooms.append(roomCfg);
        }
    }

    if (m_rooms.isEmpty())
    {
        qWarning() << "关卡配置中没有房间";
        return false;
    }

    qDebug() << "加载关卡配置成功:" << m_levelName << "共" << m_rooms.size() << "个房间";
    return true;
}

const RoomConfig &LevelConfig::getRoom(int index) const
{
    static RoomConfig defaultRoom;
    if (index < 0 || index >= m_rooms.size())
    {
        qWarning() << "房间索引越界:" << index;
        return defaultRoom;
    }
    return m_rooms[index];
}

QStringList LevelConfig::readDescriptionsFromJson(const QString &filePath)
{
    QStringList descriptions;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return descriptions;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        return descriptions;
    }

    QJsonObject rootObj = doc.object();

    if (!rootObj.contains("description"))
    {
        qWarning() << "找不到description字段";
        return descriptions;
    }

    QJsonArray descArray = rootObj["description"].toArray();

    for (const QJsonValue &value : descArray)
    {
        QString text = value.toString();
        descriptions.append(text);
    }
    return descriptions;
}

RoomConfig LevelConfig::parseRoomConfig(const QJsonObject &roomObj)
{
    RoomConfig cfg;

    // 背景图片
    cfg.backgroundImage = roomObj.value("background").toString();

    // Boss标志
    cfg.hasBoss = roomObj.value("hasBoss").toBool(false);

    // 解析敌人生成配置列表
    // enemyCount 将由各种敌人的 count 自动累加计算，不再从 JSON 读取
    cfg.enemyCount = 0;
    if (roomObj.contains("enemies") && roomObj.value("enemies").isArray())
    {
        QJsonArray enemiesArray = roomObj.value("enemies").toArray();
        for (const QJsonValue &enemyVal : enemiesArray)
        {
            if (enemyVal.isObject())
            {
                QJsonObject enemyObj = enemyVal.toObject();
                QString type = enemyObj.value("type").toString();
                int count = enemyObj.value("count").toInt(0);
                if (!type.isEmpty() && count > 0)
                {
                    cfg.enemies.append(EnemySpawnConfig(type, count));
                    cfg.enemyCount += count; // 自动累加敌人总数
                }
            }
        }
    }

    // 宝箱配置
    cfg.hasChest = roomObj.value("hasChest").toBool(false);
    cfg.isChestLocked = roomObj.value("isChestLocked").toBool(false);

    // Boss对话配置
    if (roomObj.contains("bossDialog") && roomObj.value("bossDialog").isArray())
    {
        QJsonArray dialogArray = roomObj.value("bossDialog").toArray();
        for (const QJsonValue &dialogVal : dialogArray)
        {
            cfg.bossDialog.append(dialogVal.toString());
        }
    }

    // Boss对话背景图片配置
    if (roomObj.contains("bossDialogBackground"))
    {
        cfg.bossDialogBackground = roomObj.value("bossDialogBackground").toString();
    }

    // 门连接配置
    QJsonObject doors = roomObj.value("doors").toObject();
    cfg.doorUp = doors.value("up").toInt(-1);
    cfg.doorDown = doors.value("down").toInt(-1);
    cfg.doorLeft = doors.value("left").toInt(-1);
    cfg.doorRight = doors.value("right").toInt(-1);

    return cfg;
}
