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

    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.8f);

    m_bgmPlayer = new QMediaPlayer(this);
    m_bgmPlayer->setAudioOutput(m_audioOutput);

    m_eventOutput = new QAudioOutput(this);
    m_eventOutput->setVolume(0.8f);

    m_eventPlayer = new QMediaPlayer(this);
    m_eventPlayer->setAudioOutput(m_eventOutput);

    m_splitOutput = new QAudioOutput(this);
    m_splitOutput->setVolume(0.6f);

    m_splitPlayer = new QMediaPlayer(this);
    m_splitPlayer->setAudioOutput(m_splitOutput);

    auto loadEffect = [&](QSoundEffect*& effect, const QString& filename) {
        effect = new QSoundEffect(this);
        effect->setSource(QUrl::fromLocalFile(bgmDir + QStringLiteral("/sfx/") + filename));
        if (effect->status() == QSoundEffect::Error) {
            qWarning() << "SoundManager: failed to load" << filename;
            delete effect;
            effect = nullptr;
            return;
        }
        effect->setVolume(0.6f);
    };

    loadEffect(m_startGameSound, QStringLiteral("eject.wav"));
    loadEffect(m_gameOverSound, QStringLiteral("hurt.wav"));
    loadEffect(m_victorySound, QStringLiteral("skill.wav"));

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
}

QString SoundManager::randomBgm(const QStringList& list) const
{
    if (list.isEmpty()) return {};
    int idx = QRandomGenerator::global()->bounded(list.size());
    return list.at(idx);
}

void SoundManager::playRandomBgm(const QStringList& list)
{
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
    preloadEventMusic();
}

void SoundManager::playGameOverMusic()
{
    m_bgmPlayer->stop();
    m_eventPlayer->stop();
    if (m_eventPlayer->source().isEmpty()) {
        QString path = randomBgm(m_failureBgmFiles);
        if (path.isEmpty()) return;
        m_eventPlayer->setSource(QUrl::fromLocalFile(path));
    }
    m_eventPlayer->setLoops(1);
    m_eventPlayer->play();
}

void SoundManager::playVictoryMusic()
{
    m_bgmPlayer->stop();
    m_eventPlayer->stop();
    if (m_eventPlayer->source().isEmpty()) {
        QString path = randomBgm(m_successBgmFiles);
        if (path.isEmpty()) return;
        m_eventPlayer->setSource(QUrl::fromLocalFile(path));
    }
    m_eventPlayer->setLoops(1);
    m_eventPlayer->play();
}

void SoundManager::preloadEventMusic()
{
    QString path = randomBgm(m_failureBgmFiles);
    if (!path.isEmpty())
        m_eventPlayer->setSource(QUrl::fromLocalFile(path));
}

void SoundManager::preloadVictoryMusic()
{
    QString path = randomBgm(m_successBgmFiles);
    if (!path.isEmpty())
        m_eventPlayer->setSource(QUrl::fromLocalFile(path));
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
        m_currentBgmIndex = m_gameBgmFiles.indexOf(m_bgmPlayer->source().toLocalFile());
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