#include "audiomanager.h"
#include <QDebug>
#include <QDir>

AudioManager& AudioManager::instance()
{
    static AudioManager instance;
    return instance;
}

AudioManager::AudioManager(QObject* parent) 
    : QObject(parent), 
      m_soundVolume(100), 
      m_musicVolume(80)
{
    // 初始化音乐播放器
    m_musicPlayer = new QMediaPlayer(this);
    m_playlist = new QMediaPlaylist(this);
    m_musicPlayer->setPlaylist(m_playlist);
    m_musicPlayer->setVolume(m_musicVolume);
    
    // 实现循环播放
    connect(m_musicPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &AudioManager::onMediaStatusChanged);
}

AudioManager::~AudioManager()
{
    qDeleteAll(m_soundEffects);
    m_soundEffects.clear();
}

void AudioManager::preloadSound(const QString& soundName, const QString& filePath)
{
    if (m_soundEffects.contains(soundName)) {
        qDebug() << "Sound already preloaded:" << soundName;
        return;
    }
    
    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        qWarning() << "Sound file not found:" << filePath;
        return;
    }
    
    QSoundEffect* effect = new QSoundEffect(this);
    effect->setSource(QUrl::fromLocalFile(filePath));
    effect->setVolume(m_soundVolume / 100.0);
    m_soundEffects[soundName] = effect;
    
    qDebug() << "Preloaded sound:" << soundName << "from" << filePath;
}

void AudioManager::playSound(const QString& soundName)
{
    if (m_soundEffects.contains(soundName)) {
        m_soundEffects[soundName]->play();
    } else {
        qWarning() << "Sound effect not found:" << soundName;
    }
}

void AudioManager::playMusic(const QString& musicFile)
{
    if (!QFile::exists(musicFile)) {
        qWarning() << "Music file not found:" << musicFile;
        return;
    }

    m_playlist->clear();
    m_playlist->addMedia(QUrl::fromLocalFile(musicFile));
    m_playlist->setPlaybackMode(QMediaPlaylist::Loop);
    m_musicPlayer->play();
    qDebug() << "Playing music:" << musicFile;
}

void AudioManager::stopMusic()
{
    m_musicPlayer->stop();
}

void AudioManager::setMusicVolume(int volume)
{
    m_musicVolume = qBound(0, volume, 100);
    m_musicPlayer->setVolume(m_musicVolume);
}

void AudioManager::setSoundVolume(int volume)
{
    m_soundVolume = qBound(0, volume, 100);
    for (QSoundEffect* effect : m_soundEffects) {
        effect->setVolume(m_soundVolume / 100.0);
    }
}

bool AudioManager::isMusicPlaying() const
{
    return m_musicPlayer->state() == QMediaPlayer::PlayingState;
}

void AudioManager::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        qDebug() << "Music playback finished";
    }
}
