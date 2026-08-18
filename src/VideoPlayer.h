#pragma once

#include <QWidget>

class QMediaPlayer;
class QVideoWidget;
class QPushButton;
class QLabel;
class QTimer;
class QPropertyAnimation;

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

    // expose video widget for shortcut parenting if needed
    QVideoWidget* videoWidget() const { return m_videoWidget; }

signals:
    void statusMessage(const QString& msg);
    void progressUpdated(int percent);

private slots:
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onStateChanged(int state);

signals:
    void positionChangedMS(qint64 pos);
    void durationChangedMS(qint64 dur);
private:
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

    void showControls();
    void hideControls();

private:
    QMediaPlayer* m_player;
    QVideoWidget* m_videoWidget;
    qint64 m_duration;
    QPushButton* m_playBtn;
    QPushButton* m_pauseBtn;
    QPushButton* m_stopBtn;
    QWidget* m_controls;
    QWidget* m_seekBar;
    QLabel* m_timeLabel;
    QTimer* m_hideTimer;
    QPropertyAnimation* m_hideAnimation;
    bool m_controlsVisible = true;
    int m_controlsHideDelayMs = 5000;
    // fullscreen overlay
    QWidget* m_fullscreenOverlay = nullptr;
    QWidget* m_overlaySeekBar = nullptr;
    QLabel* m_overlayTimeLabel = nullptr;
    QWidget* m_controlsOriginalParent = nullptr;
};
