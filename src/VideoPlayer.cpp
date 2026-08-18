#include "VideoPlayer.h"

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QBoxLayout>
#include <QUrl>
#include <QPushButton>
#include <QStyle>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QShortcut>
#include <QTime>
#include <QHBoxLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QWindow>

// Lightweight seek bar implemented as a local helper class.
class LocalSeekBar : public QWidget
{
public:
    explicit LocalSeekBar(QWidget* parent = nullptr)
        : QWidget(parent), m_percent(0), m_ticks(10), m_pressed(false) { setMinimumHeight(28); }

    void setPercent(int p) { m_percent = qBound(0, p, 100); update(); }
    int percent() const { return m_percent; }
    void setTicks(int t) { m_ticks = t; update(); }
    void setOnSeek(std::function<void(int)> cb) { m_onSeek = std::move(cb); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF track(6, height()/2.0 - 4, width()-12, 8);
        p.setBrush(QColor(60,60,60));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(track, 4, 4);

        // ticks
        if (m_ticks > 1) {
            p.setPen(QPen(QColor(200,200,200,120), 1));
            for (int i=0;i<=m_ticks;i++){
                qreal x = track.left() + (track.width() * i / (qreal)m_ticks);
                p.drawLine(QPointF(x, track.top()-6), QPointF(x, track.top()-2));
            }
        }

        // filled
        QRectF filled = track;
        filled.setWidth(track.width() * m_percent / 100.0);
        p.setBrush(QColor(34,197,94));
        p.drawRoundedRect(filled, 4, 4);

        // handle
        qreal hx = track.left() + track.width() * m_percent / 100.0;
        QPointF center(hx, track.center().y());
        p.setBrush(Qt::white);
        p.setPen(QPen(Qt::black, 0.5));
        p.drawEllipse(center, 8, 8);
    }

    void mousePressEvent(QMouseEvent* ev) override { m_pressed=true; handleMouse(ev->pos()); }
    void mouseMoveEvent(QMouseEvent* ev) override { if(m_pressed) handleMouse(ev->pos()); }
    void mouseReleaseEvent(QMouseEvent*) override { m_pressed=false; }

private:
    void handleMouse(const QPoint& pos)
    {
        QRectF track(6, height()/2.0 - 4, width()-12, 8);
        qreal pct = (pos.x()-track.left()) / track.width();
        int p = qBound(0, int(pct*100.0), 100);
        if (m_onSeek) m_onSeek(p);
        setPercent(p);
    }

    int m_percent;
    int m_ticks;
    bool m_pressed;
    std::function<void(int)> m_onSeek;
};

VideoPlayer::VideoPlayer(QWidget* parent)
    : QWidget(parent), m_player(new QMediaPlayer(this)), m_videoWidget(new QVideoWidget(this)), m_duration(0)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4,4,4,4);

    layout->addWidget(m_videoWidget, 1);

    // Controls container inside player
    QWidget* controls = new QWidget(this);
    QVBoxLayout* controlsLayout = new QVBoxLayout(controls);
    controlsLayout->setContentsMargins(6,6,6,6);

    LocalSeekBar* seekBar = new LocalSeekBar(this);
    seekBar->setTicks(10);
    controlsLayout->addWidget(seekBar);

    QHBoxLayout* bottomRow = new QHBoxLayout();
    QLabel* timeLabel = new QLabel("00:00 / 00:00", this);
    bottomRow->addWidget(timeLabel);
    bottomRow->addStretch();

    // playback control buttons (centralized in player)
    m_playBtn = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), "Start", this);
    m_pauseBtn = new QPushButton(style()->standardIcon(QStyle::SP_MediaPause), QString(), this);
    m_stopBtn = new QPushButton(style()->standardIcon(QStyle::SP_MediaStop), QString(), this);
    QPushButton* prevBtn = new QPushButton(style()->standardIcon(QStyle::SP_ArrowBack), QString(), this);
    QPushButton* nextBtn = new QPushButton(style()->standardIcon(QStyle::SP_ArrowForward), QString(), this);
    QPushButton* fsBtn = new QPushButton(style()->standardIcon(QStyle::SP_TitleBarMaxButton), QString(), this);

    bottomRow->addWidget(prevBtn);
    bottomRow->addWidget(m_playBtn);
    bottomRow->addWidget(m_pauseBtn);
    bottomRow->addWidget(m_stopBtn);
    bottomRow->addWidget(nextBtn);
    bottomRow->addWidget(fsBtn);
    controlsLayout->addLayout(bottomRow);

    // keep pointers to important widgets for runtime updates
    m_controls = controls;
    m_seekBar = seekBar;
    m_timeLabel = timeLabel;

    layout->addWidget(controls);

    m_player->setVideoOutput(m_videoWidget);

    connect(m_player, &QMediaPlayer::positionChanged, this, &VideoPlayer::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &VideoPlayer::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state){ onStateChanged(static_cast<int>(state)); });

    // seek callback
    seekBar->setOnSeek([this](int percent){ if (m_duration>0) seek((m_duration*percent)/100); });

    // update time label when position/duration change
    connect(this, &VideoPlayer::positionChangedMS, this, [this](qint64 pos){ if (m_timeLabel){ QTime cur = QTime::fromMSecsSinceStartOfDay(int(pos)); QTime total = QTime::fromMSecsSinceStartOfDay(int(m_duration)); QString text = cur.toString("mm:ss") + " / " + total.toString("mm:ss"); m_timeLabel->setText(text); } });

    // buttons
    connect(prevBtn, &QPushButton::clicked, this, [this](){ emit statusMessage("prev-request"); });
    connect(nextBtn, &QPushButton::clicked, this, [this](){ emit statusMessage("next-request"); });
    connect(fsBtn, &QPushButton::clicked, this, &VideoPlayer::toggleFullScreen);

    // Start button: restart from beginning then play
    connect(m_playBtn, &QPushButton::clicked, this, [this](){ if (m_duration>0) seek(0); play(); });

    // Pause button acts as a toggle: pause if playing, resume if paused
    connect(m_pauseBtn, &QPushButton::clicked, this, [this](){ if (m_player->playbackState() == QMediaPlayer::PlayingState) pause(); else play(); });

    connect(m_stopBtn, &QPushButton::clicked, this, &VideoPlayer::stop);

    // shortcuts on this widget so they work in fullscreen when the video widget is focused

    QShortcut* space = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(space, &QShortcut::activated, this, [this](){ if (m_player->playbackState()==QMediaPlayer::PlayingState) pause(); else play(); });
    QShortcut* fkey = new QShortcut(QKeySequence(Qt::Key_F), this);
    connect(fkey, &QShortcut::activated, this, &VideoPlayer::toggleFullScreen);

    // Next / Previous keyboard shortcuts
    QShortcut* nextKey = new QShortcut(QKeySequence(Qt::Key_N), this);
    connect(nextKey, &QShortcut::activated, this, [this](){ emit statusMessage("next-request"); });
    QShortcut* prevKey = new QShortcut(QKeySequence(Qt::Key_P), this);
    connect(prevKey, &QShortcut::activated, this, [this](){ emit statusMessage("prev-request"); });

    // Seek shortcuts: left/right arrows seek by 5 seconds
    QShortcut* seekLeft = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(seekLeft, &QShortcut::activated, this, [this](){ qint64 pos = qMax<qint64>(0, m_player->position() - 5000); seek(pos); });
    QShortcut* seekRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(seekRight, &QShortcut::activated, this, [this](){ qint64 pos = qMin<qint64>(m_duration, m_player->position() + 5000); seek(pos); });

    m_videoWidget->installEventFilter(this);
    // enable mouse move events
    m_videoWidget->setMouseTracking(true);
    this->setMouseTracking(true);

    // hide timer + animation for fullscreen overlay
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(m_controlsHideDelayMs);
    connect(m_hideTimer, &QTimer::timeout, this, &VideoPlayer::hideControls);

    m_hideAnimation = new QPropertyAnimation(m_controls);
    m_hideAnimation->setPropertyName("windowOpacity");
    m_hideAnimation->setDuration(300);
    m_controlsVisible = true;
    m_controlsOriginalParent = m_controls->parentWidget();
}

VideoPlayer::~VideoPlayer()
{
    if (m_player)
        m_player->stop();
}

void VideoPlayer::load(const QString& path)
{
    stop();
    QUrl url = QUrl::fromLocalFile(path);
    m_player->setSource(url);
    emit statusMessage(QString("Loaded: %1").arg(path));
}

void VideoPlayer::play()
{
    if (!m_player->source().isEmpty())
    {
        m_player->play();
        emit statusMessage("Playing");
    }
}

void VideoPlayer::pause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
    {
        m_player->pause();
        emit statusMessage("Paused");
    }
}

void VideoPlayer::stop()
{
    if (m_player->playbackState() != QMediaPlayer::StoppedState)
    {
        m_player->stop();
        emit statusMessage("Stopped");
    }
}

void VideoPlayer::seek(qint64 positionMs)
{
    if (m_player)
        m_player->setPosition(positionMs);
}

void VideoPlayer::onPositionChanged(qint64 pos)
{
    if (m_duration > 0)
    {
        int percent = static_cast<int>((pos * 100) / m_duration);
        emit progressUpdated(percent);
        emit positionChangedMS(pos);

        // update seek bar UI directly when available
        if (m_seekBar)
        {
            // LocalSeekBar is a plain QWidget-derived local class; use dynamic_cast
            LocalSeekBar* s = dynamic_cast<LocalSeekBar*>(m_seekBar);
            if (s) s->setPercent(percent);
        }

        // If in fullscreen and playing, ensure controls auto-hide
        if (m_videoWidget && m_videoWidget->isFullScreen())
        {
            if (m_player->playbackState() == QMediaPlayer::PlayingState)
                m_hideTimer->start();
        }
    }
}

void VideoPlayer::onDurationChanged(qint64 dur)
{
    m_duration = dur;
    emit durationChangedMS(dur);
}

void VideoPlayer::onStateChanged(int state)
{
    // Update pause button icon depending on playback state
    auto s = static_cast<QMediaPlayer::PlaybackState>(state);
    if (m_pauseBtn)
    {
        if (s == QMediaPlayer::PlayingState)
            m_pauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        else
            m_pauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void VideoPlayer::toggleFullScreen()
{
    if (m_videoWidget)
    {
        bool fs = !m_videoWidget->isFullScreen();
        m_videoWidget->setFullScreen(fs);

        if (fs)
        {
            // when entering full screen, ensure controls visible and start hide timer
            // reparent controls into the video widget so they overlay fullscreen window
            if (m_controls && m_videoWidget)
            {
                m_controlsOriginalParent = m_controls->parentWidget();
                m_controls->setParent(m_videoWidget);
                m_controls->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
                m_controls->setAttribute(Qt::WA_TranslucentBackground, true);
                m_controls->show();
                // position at bottom
                QRect vr = m_videoWidget->rect();
                int h = m_controls->sizeHint().height();
                m_controls->setGeometry(0, vr.height() - h - 10, vr.width(), h);
            }

            showControls();
            if (m_hideTimer) m_hideTimer->start();
        }
        else
        {
            // leaving fullscreen: show controls permanently
            // restore parent and layout
            if (m_controls)
            {
                m_controls->setParent(this);
                m_controls->setWindowFlags(Qt::Widget);
                m_controls->show();
                if (this->layout()) this->layout()->addWidget(m_controls);
            }
            showControls();
            if (m_hideTimer) m_hideTimer->stop();
        }
    }
}

bool VideoPlayer::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_videoWidget)
    {
        if (event->type() == QEvent::MouseButtonDblClick)
        {
            toggleFullScreen();
            return true;
        }

        if (event->type() == QEvent::MouseMove)
        {
            // show controls on mouse movement
            showControls();
            if (m_hideTimer && m_videoWidget->isFullScreen())
                m_hideTimer->start();
        }
        if (event->type() == QEvent::Resize)
        {
            // reposition overlay controls when the video widget resizes in fullscreen
            if (m_videoWidget->isFullScreen() && m_controls)
            {
                QRect vr = m_videoWidget->rect();
                int h = m_controls->sizeHint().height();
                m_controls->setGeometry(0, vr.height() - h - 10, vr.width(), h);
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void VideoPlayer::showControls()
{
    if (!m_controls) return;
    if (m_hideAnimation && m_hideAnimation->state() == QPropertyAnimation::Running)
        m_hideAnimation->stop();
    m_controls->setWindowOpacity(1.0);
    m_controls->show();
    m_controlsVisible = true;
}

void VideoPlayer::hideControls()
{
    if (!m_controls) return;
    if (!m_videoWidget->isFullScreen()) return;
    if (m_hideAnimation)
    {
        m_hideAnimation->stop();
        m_hideAnimation->setStartValue(1.0);
        m_hideAnimation->setEndValue(0.0);
        m_hideAnimation->start();
        m_controlsVisible = false;
    }
    else
    {
        m_controls->hide();
        m_controlsVisible = false;
    }
}
