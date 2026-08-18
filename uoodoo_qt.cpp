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
        resize(1150, 760);
        setMinimumSize(900, 600);

        libraryModel = new LibraryModel("library.json");

        QWidget* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);

        QHBoxLayout* headerLayout = new QHBoxLayout();

        QLabel* title = new QLabel("UooDoo");
        title->setObjectName("appTitle");
        headerLayout->addWidget(title);

        headerLayout->addStretch();

        QLabel* subtitle = new QLabel(
            "Interactive video library for local playback management"
        );
        subtitle->setObjectName("appSubtitle");
        headerLayout->addWidget(subtitle);

        rootLayout->addLayout(headerLayout);


        QHBoxLayout* toolbarLayout = new QHBoxLayout();

        searchBox = new QLineEdit();
        searchBox->setObjectName("search");
        searchBox->setPlaceholderText("Search videos...");
        toolbarLayout->addWidget(searchBox, 1);

        addButton = new QPushButton("Add Video");
        addButton->setObjectName("primary");
        toolbarLayout->addWidget(addButton);

        favoriteButton = new QPushButton("Favorite");
        toolbarLayout->addWidget(favoriteButton);

        removeButton = new QPushButton("Remove");
        toolbarLayout->addWidget(removeButton);

        rootLayout->addLayout(toolbarLayout);


        QHBoxLayout* bodyLayout = new QHBoxLayout();

        QVBoxLayout* leftLayout = new QVBoxLayout();

        QLabel* libraryLabel = new QLabel("Library");
        libraryLabel->setObjectName("sectionTitle");
        leftLayout->addWidget(libraryLabel);

        videoList = new QListWidget();
        videoList->setObjectName("videoList");
        leftLayout->addWidget(videoList, 1);

        bodyLayout->addLayout(leftLayout, 1);


        QVBoxLayout* rightLayout = new QVBoxLayout();

        QFrame* card = new QFrame();
        card->setObjectName("card");
        card->setStyleSheet(
            "QFrame#card { background: #111a2e; border: 1px solid #1e293b; border-radius: 16px; }"
        );

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        cardLayout->setSpacing(8);

        infoTitle = new QLabel("No video selected");
        infoTitle->setObjectName("videoTitle");
        cardLayout->addWidget(infoTitle);

        infoPath = new QLabel("Select a video to preview it here.");
        infoPath->setObjectName("muted");
        infoPath->setWordWrap(true);
        cardLayout->addWidget(infoPath);

        statusLabel = new QLabel("Ready to add a video");
        statusLabel->setObjectName("status");
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
        footer->setObjectName("muted");
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

        const QVector<VideoEntry>& items = libraryModel->all();
        for (const VideoEntry& video : items)
        {
            if (video.name == currentName)
            {
                videoPlayer->load(video.path);
                videoPlayer->play();
                return;
            }
        }
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
    app.setApplicationName("UooDoo");
    app.setApplicationDisplayName("UooDoo - Video Library");
    app.setStyleSheet(R"(
        QWidget {
            background: #0b1220;
            color: #e2e8f0;
            font-family: "Segoe UI", "Inter", Arial, sans-serif;
            font-size: 13px;
        }

        QWidget#centralWidget, QMainWindow {
            background: #0b1220;
        }

        QLabel {
            background: transparent;
        }

        QLabel#appTitle {
            font-size: 26px;
            font-weight: 700;
            color: #ffffff;
            letter-spacing: 1px;
        }

        QLabel#appSubtitle {
            color: #7dd3fc;
            font-size: 11px;
        }

        QLabel#sectionTitle {
            font-size: 15px;
            font-weight: 600;
            color: #f1f5f9;
        }

        QLabel#videoTitle {
            font-size: 18px;
            font-weight: 600;
            color: #ffffff;
        }

        QLabel#muted {
            color: #94a3b8;
            font-size: 11px;
        }

        QLabel#status {
            color: #fcd34d;
            font-size: 12px;
            font-weight: 600;
        }

        QFrame#card {
            background: #111a2e;
            border: 1px solid #1e293b;
            border-radius: 16px;
        }

        VideoPlayer {
            background: #0f172a;
            border-radius: 12px;
        }

        QWidget#playerControls {
            background: transparent;
        }

        QLineEdit#search {
            background: #0f172a;
            border: 1px solid #1e293b;
            border-radius: 10px;
            padding: 9px 14px;
            color: #f1f5f9;
            selection-background-color: #2563eb;
        }

        QLineEdit#search:focus {
            border: 1px solid #3b82f6;
        }

        QPushButton {
            background: #1e293b;
            color: #e2e8f0;
            border: 1px solid #334155;
            border-radius: 10px;
            padding: 9px 16px;
            font-weight: 600;
        }

        QPushButton:hover {
            background: #2b3a52;
        }

        QPushButton:pressed {
            background: #182333;
        }

        QPushButton#primary {
            background: #2563eb;
            border: 1px solid #3b82f6;
            color: #ffffff;
        }

        QPushButton#primary:hover {
            background: #1d4ed8;
        }

        QPushButton#primary:pressed {
            background: #1e40af;
        }

        QListWidget#videoList {
            background: #0f172a;
            border: 1px solid #1e293b;
            border-radius: 12px;
            padding: 6px;
            outline: none;
        }

        QListWidget#videoList::item {
            border-radius: 8px;
            padding: 8px 10px;
            margin: 2px;
            color: #cbd5e1;
        }

        QListWidget#videoList::item:hover {
            background: #1e293b;
            color: #f1f5f9;
        }

        QListWidget#videoList::item:selected {
            background: #1e3a8a;
            color: #ffffff;
        }

        QMenuBar {
            background: #0b1220;
            color: #cbd5e1;
        }

        QMenuBar::item:selected {
            background: #1e293b;
        }

        QMenu {
            background: #111a2e;
            border: 1px solid #1e293b;
            border-radius: 8px;
            padding: 4px;
        }

        QMenu::item {
            padding: 6px 22px;
            border-radius: 6px;
        }

        QMenu::item:selected {
            background: #1e3a8a;
            color: #ffffff;
        }

        QMessageBox, QInputDialog {
            background: #0f172a;
        }

        QProgressBar {
            background: #1e293b;
            border: none;
            border-radius: 6px;
            height: 10px;
            text-align: center;
            color: #cbd5e1;
        }

        QProgressBar::chunk {
            background: #22c55e;
            border-radius: 6px;
        }
    )");

    UooDooWindow window;
    window.show();

    return app.exec();
}
