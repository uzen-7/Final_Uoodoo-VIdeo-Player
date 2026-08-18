#pragma once

#include <QWidget>

class QMediaPlayer;
class QVideoWidget;
class QAudioOutput;
class QPushButton;
class QLabel;
class QBoxLayout;

class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPlayer(QWidget* parent = nullptr);
    ~VideoPlayer();

    void load(const QString& path);
    void play();
    void pause();
    void stop();
    void seek(qint64 positionMs);
    void toggleFullScreen();

    QVideoWidget* videoWidget() const { return m_videoWidget; }

signals:
    void statusMessage(const QString& msg);
    void progressUpdated(int percent);
    void positionChangedMS(qint64 pos);
    void durationChangedMS(qint64 dur);

private slots:
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onStateChanged(int state);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QMediaPlayer* m_player;
    QAudioOutput* m_audioOutput;
    QVideoWidget* m_videoWidget;
    qint64 m_duration = 0;
    QPushButton* m_playBtn;
    QPushButton* m_pauseBtn;
    QPushButton* m_stopBtn;
    QWidget* m_controls;
    QWidget* m_seekBar;
    QLabel* m_timeLabel;
    QBoxLayout* m_layoutBackup = nullptr;
    int m_layoutIndex = -1;
    bool m_pendingPlay = false;
};