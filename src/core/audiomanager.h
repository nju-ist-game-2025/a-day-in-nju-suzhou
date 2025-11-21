#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QMap>
#include <QMediaPlayer>
#include <QObject>
#include <QSoundEffect>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioOutput>
#else
#include <QMediaPlaylist>
#endif

class AudioManager : public QObject {
    Q_OBJECT

   public:
    static AudioManager& instance();

    // 音效控制
    void playSound(const QString& soundName);
    void preloadSound(const QString& soundName, const QString& filePath);

    // 背景音乐控制
    void playMusic(const QString& musicFile);
    void stopMusic();
    void setMusicVolume(int volume);
    void setSoundVolume(int volume);

    // 状态检查
    bool isMusicPlaying() const;

   private:
    AudioManager(QObject* parent = nullptr);
    ~AudioManager();

    QMap<QString, QSoundEffect*> m_soundEffects;
    QMediaPlayer* m_musicPlayer;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QAudioOutput* m_audioOutput;
    QString m_currentMusicFile;
#else
    QMediaPlaylist* m_playlist;
#endif

    int m_soundVolume;
    int m_musicVolume;

   private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
};

#endif  // AUDIOMANAGER_H
