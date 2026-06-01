#include "SoundManager.h"

#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QUrl>

SoundManager::SoundManager(const QString& bgmDir, QObject* parent)
    : QObject(parent)
    , m_bgmDir(bgmDir)
{
    auto scanDir = [&](const QString& subdir, QStringList& outList) {
        QStringList filters = {QStringLiteral("*.mp3"), QStringLiteral("*.wav")};
        QDir dir(bgmDir + QStringLiteral("/") + subdir);
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
        for (const auto& fi : files)
            outList << fi.absoluteFilePath();
    };

    scanDir(QStringLiteral("menu"),    m_menuBgmFiles);
    scanDir(QStringLiteral("game"),    m_gameBgmFiles);
    scanDir(QStringLiteral("failure"), m_failureBgmFiles);
    scanDir(QStringLiteral("success"), m_successBgmFiles);

    if (m_menuBgmFiles.isEmpty())    qWarning() << "SoundManager: no menu BGM found";
    if (m_gameBgmFiles.isEmpty())    qWarning() << "SoundManager: no game BGM found";
    if (m_failureBgmFiles.isEmpty()) qWarning() << "SoundManager: no failure BGM found";
    if (m_successBgmFiles.isEmpty()) qWarning() << "SoundManager: no success BGM found";

    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(BGM_VOLUME);

    m_bgmPlayer = new QMediaPlayer(this);
    m_bgmPlayer->setAudioOutput(m_audioOutput);

    m_eventOutput = new QAudioOutput(this);
    m_eventOutput->setVolume(BGM_VOLUME);

    m_eventPlayer = new QMediaPlayer(this);
    m_eventPlayer->setAudioOutput(m_eventOutput);

    m_splitOutput = new QAudioOutput(this);
    m_splitOutput->setVolume(SFX_VOLUME);

    m_splitPlayer = new QMediaPlayer(this);
    m_splitPlayer->setAudioOutput(m_splitOutput);

    auto loadEffect = [&](QSoundEffect*& effect, const QString& filename) {
        effect = new QSoundEffect(this);
        QString fullPath = bgmDir + QStringLiteral("/sfx/") + filename;
        connect(effect, &QSoundEffect::statusChanged, this, [effect, filename]() {
            if (effect->status() == QSoundEffect::Error) {
                qWarning() << "SoundManager: failed to load" << filename;
            }
        });
        effect->setSource(QUrl::fromLocalFile(fullPath));
        effect->setVolume(SFX_VOLUME);
    };

    loadEffect(m_startGameSound, QStringLiteral("eject.wav"));

    m_splitPlayer->setSource(QUrl::fromLocalFile(bgmDir + QStringLiteral("/sfx/split.wav")));

    connect(m_bgmPlayer, &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error err, const QString& errStr) {
        qWarning() << "SoundManager: BGM player error" << err << errStr;
    });
    connect(m_splitPlayer, &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error err, const QString& errStr) {
        qWarning() << "SoundManager: split player error" << err << errStr;
    });
    connect(m_eventPlayer, &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error err, const QString& errStr) {
        qWarning() << "SoundManager: event player error" << err << errStr;
    });
}

SoundManager::~SoundManager()
{
    m_bgmPlayer->stop();
    m_eventPlayer->stop();
    m_splitPlayer->stop();
    if (m_startGameSound) m_startGameSound->stop();
}

QString SoundManager::randomBgm(const QStringList& list) const
{
    if (list.isEmpty()) return {};
    int idx = QRandomGenerator::global()->bounded(list.size());
    return list.at(idx);
}

void SoundManager::playRandomBgm(const QStringList& list)
{
    m_eventPlayer->stop();
    QString bgmPath = randomBgm(list);
    if (bgmPath.isEmpty()) return;
    m_bgmPlayer->setSource(QUrl::fromLocalFile(bgmPath));
    m_bgmPlayer->setLoops(QMediaPlayer::Infinite);
    m_bgmPlayer->play();
}

void SoundManager::playMenuMusic()
{
    playRandomBgm(m_menuBgmFiles);
}

void SoundManager::playGameMusic()
{
    if (m_isPaused) {
        m_bgmPlayer->setPosition(m_currentPosition);
        m_bgmPlayer->play();
        m_isPaused = false;
        return;
    }
    playRandomBgm(m_gameBgmFiles);
}

void SoundManager::playGameOverMusic()
{
    playEventMusic(m_failureBgmFiles);
}

void SoundManager::playVictoryMusic()
{
    playEventMusic(m_successBgmFiles);
}

void SoundManager::playEventMusic(const QStringList& list)
{
    m_bgmPlayer->stop();
    m_eventPlayer->stop();
    QString path = randomBgm(list);
    if (path.isEmpty()) return;
    m_eventPlayer->setSource(QUrl::fromLocalFile(path));
    m_eventPlayer->setLoops(1);
    m_eventPlayer->play();
}

void SoundManager::playSplitSound()
{
    if (m_splitPlayer) {
        m_splitPlayer->stop();
        m_splitPlayer->play();
    }
}

void SoundManager::playStartGameSound()
{
    if (m_startGameSound) m_startGameSound->play();
}

void SoundManager::stopMusic()
{
    m_bgmPlayer->stop();
}

void SoundManager::pauseMusic()
{
    if (m_bgmPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_currentPosition = m_bgmPlayer->position();
        m_bgmPlayer->pause();
        m_isPaused = true;
    }
}

void SoundManager::resumeMusic()
{
    if (m_isPaused) {
        m_bgmPlayer->setPosition(m_currentPosition);
        m_bgmPlayer->play();
        m_isPaused = false;
    }
}