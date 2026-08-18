#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QFrame>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>
#include <QStandardPaths>
#include <QDebug>
#include <QSettings>
#include <QMenuBar>
#include <QInputDialog>

#include "src/VideoPlayer.h"


struct VideoEntry
{
    QString name;
    QString path;
    bool favorite = false;
};


class LibraryModel
{
public:
    LibraryModel(const QString& path = "library.json")
    {
        storagePath = path;
        load();
    }

    bool addVideo(const QString& filePath)
    {
        QFileInfo info(filePath);

        if (!info.exists() || !info.isFile())
            return false;

        QString extension = info.suffix().toLower();
        QStringList allowedExtensions;
        allowedExtensions << "mp4" << "mkv" << "avi" << "mov" << "wmv";

        if (!allowedExtensions.contains(extension))
            return false;

        QString fullPath = info.absoluteFilePath();

        for (const VideoEntry& video : videos)
        {
            if (video.path == fullPath)
                return false;
        }

        VideoEntry video;
        video.name = info.fileName();
        video.path = fullPath;
        video.favorite = false;

        videos.push_back(video);
        save();

        return true;
    }

    bool removeVideo(const QString& name)
    {
        int oldSize = videos.size();
        QVector<VideoEntry> newVideos;

        for (const VideoEntry& video : videos)
        {
            if (video.name != name)
                newVideos.push_back(video);
        }

        videos = newVideos;

        if (videos.size() != oldSize)
        {
            save();
            return true;
        }

        return false;
    }

    bool toggleFavorite(const QString& name)
    {
        for (VideoEntry& video : videos)
        {
            if (video.name == name)
            {
                video.favorite = !video.favorite;
                save();
                return true;
            }
        }

        return false;
    }

    QVector<VideoEntry> search(const QString& query) const
    {
        QVector<VideoEntry> result;
        QString searchText = query.trimmed().toLower();

        for (const VideoEntry& video : videos)
        {
            if (searchText.isEmpty() ||
                video.name.toLower().contains(searchText) ||
                video.path.toLower().contains(searchText))
            {
                result.push_back(video);
            }
        }

        return result;
    }

    const QVector<VideoEntry>& all() const
    {
        return videos;
    }

private:
    void load()
    {
        QFile file(storagePath);

        if (!file.exists())
            return;

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;

        QByteArray data = file.readAll();
        file.close();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError || !document.isArray())
            return;

        videos.clear();

        for (const QJsonValue& value : document.array())
        {
            if (!value.isObject())
                continue;

            QJsonObject object = value.toObject();

            VideoEntry video;
            video.name = object.value("name").toString();
            video.path = object.value("path").toString();
            video.favorite = object.value("favorite").toBool();

            videos.push_back(video);
        }
    }

    void save()
    {
        QFile file(storagePath);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;

        QJsonArray array;

        for (const VideoEntry& video : videos)
        {
            QJsonObject object;

            object["name"] = video.name;
            object["path"] = video.path;
            object["favorite"] = video.favorite;

            array.append(object);
        }

        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
        file.close();
    }

    QString storagePath;
    QVector<VideoEntry> videos;
};


class UooDooWindow : public QMainWindow
{
public:
    UooDooWindow()
    {
        setWindowTitle("UooDoo - Video Library");
        resize(1100, 720);

        setStyleSheet(R"(
            QWidget {
                background: #07111f;
                color: #f8fafc;
                font-family: Segoe UI, Arial;
            }

            QFrame, QLabel, QListWidget, QLineEdit, QPushButton, QProgressBar {
                background: transparent;
            }

            QListWidget {
                background: #0f172a;
                border: 1px solid #1e293b;
                border-radius: 10px;
                padding: 6px;
            }

            QLineEdit {
                background: #0f172a;
                border: 1px solid #1e293b;
                border-radius: 8px;
                padding: 8px;
                color: white;
            }

            QPushButton {
                background: #2563eb;
                border: 1px solid #3b82f6;
                border-radius: 8px;
                padding: 10px 16px;
            }

            QPushButton:hover {
                background: #1d4ed8;
            }

            QLabel {
                color: #f8fafc;
            }

            QProgressBar {
                border: 1px solid #1e293b;
                border-radius: 8px;
                text-align: center;
            }

            QProgressBar::chunk {
                background-color: #22c55e;
                border-radius: 8px;
            }
        )");

        libraryModel = new LibraryModel("library.json");

        QWidget* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);

        QHBoxLayout* headerLayout = new QHBoxLayout();

        QLabel* title = new QLabel("UooDoo");
        title->setStyleSheet("font-size: 24px; font-weight: 700;");
        headerLayout->addWidget(title);

        headerLayout->addStretch();

        QLabel* subtitle = new QLabel(
            "Interactive video library for local playback management"
        );
        subtitle->setStyleSheet("color: #7dd3fc; font-size: 11px;");
        headerLayout->addWidget(subtitle);

        rootLayout->addLayout(headerLayout);


        QHBoxLayout* toolbarLayout = new QHBoxLayout();

        searchBox = new QLineEdit();
        searchBox->setPlaceholderText("Search videos...");
        toolbarLayout->addWidget(searchBox, 1);

        addButton = new QPushButton("Add Video");
        toolbarLayout->addWidget(addButton);

        favoriteButton = new QPushButton("Favorite");
        toolbarLayout->addWidget(favoriteButton);

        removeButton = new QPushButton("Remove");
        toolbarLayout->addWidget(removeButton);

        rootLayout->addLayout(toolbarLayout);


        QHBoxLayout* bodyLayout = new QHBoxLayout();

        QVBoxLayout* leftLayout = new QVBoxLayout();

        QLabel* libraryLabel = new QLabel("Library");
        libraryLabel->setStyleSheet("font-size: 16px; font-weight: 600;");
        leftLayout->addWidget(libraryLabel);

        videoList = new QListWidget();
        leftLayout->addWidget(videoList, 1);

        bodyLayout->addLayout(leftLayout, 1);


        QVBoxLayout* rightLayout = new QVBoxLayout();

        QFrame* card = new QFrame();
        card->setStyleSheet(
            "background: #0f172a; "
            "border: 1px solid #1e293b; "
            "border-radius: 14px; "
            "padding: 12px;"
        );

        QVBoxLayout* cardLayout = new QVBoxLayout(card);

        infoTitle = new QLabel("No video selected");
        infoTitle->setStyleSheet("font-size: 18px; font-weight: 600;");
        cardLayout->addWidget(infoTitle);

        infoPath = new QLabel("Select a video to preview it here.");
        infoPath->setStyleSheet(
            "color: #93c5fd; font-size: 11px; margin-bottom: 10px;"
        );
        infoPath->setWordWrap(true);
        cardLayout->addWidget(infoPath);

        statusLabel = new QLabel("Ready to add a video");
        statusLabel->setStyleSheet(
            "color: #fcd34d; font-size: 11px; font-weight: 600;"
        );
        cardLayout->addWidget(statusLabel);


        // playback controls are embedded within the VideoPlayer widget now

        // Embedded video player widget
        videoPlayer = new VideoPlayer(this);
        videoPlayer->setMinimumHeight(300);
        cardLayout->addWidget(videoPlayer, 1);


        // progress is shown inside the embedded player now

        cardLayout->addStretch();

        rightLayout->addWidget(card, 1);
        bodyLayout->addLayout(rightLayout, 2);

        rootLayout->addLayout(bodyLayout, 1);


        QLabel* footer = new QLabel(
            "Inspired by the UooDoo proposal: upload, play, search, "
            "favorites, library management, and playback controls."
        );
        footer->setStyleSheet("color: #64748b; font-size: 10px;");
        footer->setWordWrap(true);
        rootLayout->addWidget(footer);


        connect(addButton, &QPushButton::clicked,
                this, &UooDooWindow::addVideo);

        connect(favoriteButton, &QPushButton::clicked,
                this, &UooDooWindow::toggleFavorite);

        connect(removeButton, &QPushButton::clicked,
                this, &UooDooWindow::removeVideo);

        // play/pause/stop are handled by the embedded player now

        connect(searchBox, &QLineEdit::textChanged,
                this, &UooDooWindow::refreshList);

        connect(videoList, &QListWidget::itemClicked,
                this, &UooDooWindow::selectVideo);

        // Wire player signals. VideoPlayer emits status messages.
        connect(videoPlayer, &VideoPlayer::statusMessage,
                this, [this](const QString& msg){
                    if (msg == "next-request") { nextSelected(); return; }
                    if (msg == "prev-request") { prevSelected(); return; }
                    statusLabel->setText(msg);
                });


        QMenuBar* menuBarPtr = menuBar();
        QMenu* settingsMenu = menuBarPtr->addMenu("Settings");
        QAction* setPlayer = settingsMenu->addAction("Set Player...");

        connect(setPlayer, &QAction::triggered,
                this, &UooDooWindow::openSettings);

        refreshList();
    }


private slots:
    void addVideo()
    {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Choose a video",
            QDir::homePath(),
            "Video files (*.mp4 *.mkv *.avi *.mov *.wmv)"
        );

        if (filePath.isEmpty())
            return;

        if (libraryModel->addVideo(filePath))
        {
            refreshList();
            statusLabel->setText("Video added to the library");
        }
        else
        {
            QMessageBox::warning(
                this,
                "UooDoo",
                "The selected file could not be added."
            );
        }
    }


    void removeVideo()
    {
        if (currentName.isEmpty())
            return;

        if (libraryModel->removeVideo(currentName))
        {
            currentName.clear();
            refreshList();
            statusLabel->setText("Video removed");
        }
    }


    void toggleFavorite()
    {
        if (currentName.isEmpty())
            return;

        if (libraryModel->toggleFavorite(currentName))
        {
            refreshList();
            statusLabel->setText("Favorite state updated");
        }
    }


    void playSelected()
    {
        if (currentName.isEmpty())
            return;

        const QVector<VideoEntry>& items = libraryModel->all();

        for (const VideoEntry& video : items)
        {
            if (video.name != currentName)
                continue;

            // Load and play the selected video using the embedded player
            videoPlayer->load(video.path);
            videoPlayer->play();
            return;
        }
    }


    void openSettings()
    {
        QSettings settings("UooDoo", "UooDooApp");

        QString currentPlayer =
            settings.value("player").toString();

        bool ok = false;

        QString text = QInputDialog::getText(
            this,
            "Set Player",
            "Player command (e.g. vlc --play-and-exit):",
            QLineEdit::Normal,
            currentPlayer,
            &ok
        );

        if (ok)
        {
            settings.setValue("player", text.trimmed());

            statusLabel->setText(
                QString("Player saved: %1").arg(text)
            );
        }
    }


    void pauseSelected()
    {
        if (videoPlayer)
        {
            videoPlayer->pause();
        }
    }


    void stopSelected()
    {
        if (videoPlayer)
        {
            videoPlayer->stop();
        }
    }

    void nextSelected()
    {
        const QVector<VideoEntry>& items = libraryModel->all();
        if (items.isEmpty() || currentName.isEmpty())
            return;

        int idx = -1;
        for (int i=0;i<items.size();++i) if (items[i].name == currentName) { idx = i; break; }
        if (idx < 0) return;
        int next = (idx + 1) % items.size();
        currentName = items[next].name;
        updateDetails(currentName);
        videoPlayer->load(items[next].path);
        videoPlayer->play();
    }

    void prevSelected()
    {
        const QVector<VideoEntry>& items = libraryModel->all();
        if (items.isEmpty() || currentName.isEmpty())
            return;

        int idx = -1;
        for (int i=0;i<items.size();++i) if (items[i].name == currentName) { idx = i; break; }
        if (idx < 0) return;
        int prev = (idx - 1 + items.size()) % items.size();
        currentName = items[prev].name;
        updateDetails(currentName);
        videoPlayer->load(items[prev].path);
        videoPlayer->play();
    }


    void refreshList()
    {
        videoList->clear();

        QVector<VideoEntry> results =
            libraryModel->search(searchBox->text());

        for (const VideoEntry& video : results)
        {
            QString prefix;

            if (video.favorite)
                prefix = "★ ";
            else
                prefix = "• ";

            QListWidgetItem* item =
                new QListWidgetItem(prefix + video.name);

            item->setData(Qt::UserRole, video.name);
            videoList->addItem(item);
        }


        if (!results.isEmpty())
        {
            if (currentName.isEmpty())
                currentName = results.first().name;

            updateDetails(currentName);
        }
        else
        {
            currentName.clear();

            infoTitle->setText("No videos found");
            infoPath->setText(
                "Add a local video to start your library."
            );
            statusLabel->setText("No media loaded");
        }
    }


    void selectVideo(QListWidgetItem* item)
    {
        currentName = item->data(Qt::UserRole).toString();
        updateDetails(currentName);
    }


    void updateDetails(const QString& name)
    {
        const QVector<VideoEntry>& items = libraryModel->all();

        for (const VideoEntry& video : items)
        {
            if (video.name == name)
            {
                infoTitle->setText(video.name);
                infoPath->setText(video.path);

                if (video.favorite)
                {
                    statusLabel->setText("Favorite video");
                }
                else
                {
                    statusLabel->setText("Ready to play");
                }

                return;
            }
        }
    }


private:
    LibraryModel* libraryModel;

    QLineEdit* searchBox;
    QListWidget* videoList;

    QLabel* infoTitle;
    QLabel* infoPath;
    QLabel* statusLabel;


    QPushButton* addButton;
    QPushButton* favoriteButton;
    QPushButton* removeButton;

    // playback handled in `VideoPlayer` controls

    VideoPlayer* videoPlayer;

    QString currentName;
};


int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    UooDooWindow window;
    window.show();

    return app.exec();
}
