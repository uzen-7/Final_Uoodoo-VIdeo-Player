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

struct VideoEntry {
    QString name;
    QString path;
    bool favorite = false;
};

class LibraryModel {
public:
    LibraryModel(const QString& path = "library.json") : storagePath(path) {
        load();
    }

    bool addVideo(const QString& filePath) {
        QFileInfo info(filePath);
        if (!info.exists() || !info.isFile()) return false;
        QString ext = info.suffix().toLower();
        QStringList allowed = {"mp4", "mkv", "avi", "mov", "wmv"};
        if (!allowed.contains(ext)) return false;
        for (const auto& item : videos) {
            if (item.path == info.absoluteFilePath()) return false;
        }
        videos.push_back({info.fileName(), info.absoluteFilePath(), false});
        save();
        return true;
    }

    bool removeVideo(const QString& name) {
        const auto before = videos.size();
        QVector<VideoEntry> filtered;
        for (const auto& item : videos) {
            if (item.name != name) {
                filtered.push_back(item);
            }
        }
        videos = filtered;
        if (videos.size() != before) {
            save();
            return true;
        }
        return false;
    }

    bool toggleFavorite(const QString& name) {
        for (auto& item : videos) {
            if (item.name == name) {
                item.favorite = !item.favorite;
                save();
                return true;
            }
        }
        return false;
    }

    QVector<VideoEntry> search(const QString& query) const {
        QVector<VideoEntry> result;
        QString q = query.trimmed().toLower();
        for (const auto& item : videos) {
            if (q.isEmpty() || item.name.toLower().contains(q) || item.path.toLower().contains(q)) {
                result.push_back(item);
            }
        }
        return result;
    }

    const QVector<VideoEntry>& all() const { return videos; }

private:
    void load() {
        QFile file(storagePath);
        if (!file.exists()) return;
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        QByteArray data = file.readAll();
        file.close();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError || !doc.isArray()) return;
        videos.clear();
        for (const QJsonValue& value : doc.array()) {
            if (!value.isObject()) continue;
            QJsonObject obj = value.toObject();
            VideoEntry entry;
            entry.name = obj.value("name").toString();
            entry.path = obj.value("path").toString();
            entry.favorite = obj.value("favorite").toBool();
            videos.push_back(entry);
        }
    }

    void save() {
        QFile file(storagePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QJsonArray array;
        for (const auto& item : videos) {
            QJsonObject obj;
            obj["name"] = item.name;
            obj["path"] = item.path;
            obj["favorite"] = item.favorite;
            array.append(obj);
        }
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
        file.close();
    }

    QString storagePath;
    QVector<VideoEntry> videos;
};

class UooDooWindow : public QMainWindow {
public:
    UooDooWindow() {
        setWindowTitle("UooDoo - Video Library");
        resize(1100, 720);
        setStyleSheet(R"(
            QWidget { background: #07111f; color: #f8fafc; font-family: Segoe UI, Arial; }
            QFrame, QLabel, QListWidget, QLineEdit, QPushButton, QProgressBar { background: transparent; }
            QListWidget { background: #0f172a; border: 1px solid #1e293b; border-radius: 10px; padding: 6px; }
            QLineEdit { background: #0f172a; border: 1px solid #1e293b; border-radius: 8px; padding: 8px; color: white; }
            QPushButton { background: #2563eb; border: 1px solid #3b82f6; border-radius: 8px; padding: 10px 16px; }
            QPushButton:hover { background: #1d4ed8; }
            QLabel { color: #f8fafc; }
            QProgressBar { border: 1px solid #1e293b; border-radius: 8px; text-align: center; }
            QProgressBar::chunk { background-color: #22c55e; border-radius: 8px; }
        )");

        libraryModel = new LibraryModel("library.json");
        QWidget* central = new QWidget(this);
        setCentralWidget(central);
        QVBoxLayout* root = new QVBoxLayout(central);

        QHBoxLayout* header = new QHBoxLayout();
        QLabel* title = new QLabel("UooDoo");
        title->setStyleSheet("font-size: 24px; font-weight: 700;");
        header->addWidget(title);
        header->addStretch();
        QLabel* subtitle = new QLabel("Interactive video library for local playback management");
        subtitle->setStyleSheet("color: #7dd3fc; font-size: 11px;");
        header->addWidget(subtitle);
        root->addLayout(header);

        QHBoxLayout* toolbar = new QHBoxLayout();
        searchBox = new QLineEdit();
        searchBox->setPlaceholderText("Search videos...");
        toolbar->addWidget(searchBox, 1);
        addButton = new QPushButton("Add Video");
        toolbar->addWidget(addButton);
        favoriteButton = new QPushButton("Favorite");
        toolbar->addWidget(favoriteButton);
        removeButton = new QPushButton("Remove");
        toolbar->addWidget(removeButton);
        root->addLayout(toolbar);

        QHBoxLayout* body = new QHBoxLayout();
        QVBoxLayout* left = new QVBoxLayout();
        QLabel* libraryLabel = new QLabel("Library");
        libraryLabel->setStyleSheet("font-size: 16px; font-weight: 600;");
        left->addWidget(libraryLabel);
        videoList = new QListWidget();
        left->addWidget(videoList, 1);
        body->addLayout(left, 1);

        QVBoxLayout* right = new QVBoxLayout();
        QFrame* card = new QFrame();
        card->setStyleSheet("background: #0f172a; border: 1px solid #1e293b; border-radius: 14px; padding: 12px;");
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        infoTitle = new QLabel("No video selected");
        infoTitle->setStyleSheet("font-size: 18px; font-weight: 600;");
        cardLayout->addWidget(infoTitle);
        infoPath = new QLabel("Select a video to preview it here.");
        infoPath->setStyleSheet("color: #93c5fd; font-size: 11px; margin-bottom: 10px;");
        infoPath->setWordWrap(true);
        cardLayout->addWidget(infoPath);
        statusLabel = new QLabel("Ready to add a video");
        statusLabel->setStyleSheet("color: #fcd34d; font-size: 11px; font-weight: 600;");
        cardLayout->addWidget(statusLabel);

        QHBoxLayout* controlRow = new QHBoxLayout();
        playButton = new QPushButton("Play");
        controlRow->addWidget(playButton);
        pauseButton = new QPushButton("Pause");
        controlRow->addWidget(pauseButton);
        stopButton = new QPushButton("Stop");
        controlRow->addWidget(stopButton);
        cardLayout->addLayout(controlRow);

        progressBar = new QProgressBar();
        progressBar->setRange(0, 100);
        progressBar->setValue(20);
        cardLayout->addWidget(progressBar);
        cardLayout->addStretch();
        right->addWidget(card, 1);
        body->addLayout(right, 2);
        root->addLayout(body, 1);

        QLabel* footer = new QLabel("Inspired by the UooDoo proposal: upload, play, search, favorites, library management, and playback controls.");
        footer->setStyleSheet("color: #64748b; font-size: 10px;");
        footer->setWordWrap(true);
        root->addWidget(footer);

        connect(addButton, &QPushButton::clicked, this, &UooDooWindow::addVideo);
        connect(favoriteButton, &QPushButton::clicked, this, &UooDooWindow::toggleFavorite);
        connect(removeButton, &QPushButton::clicked, this, &UooDooWindow::removeVideo);
        connect(playButton, &QPushButton::clicked, this, &UooDooWindow::playSelected);
        connect(pauseButton, &QPushButton::clicked, this, &UooDooWindow::pauseSelected);
        connect(stopButton, &QPushButton::clicked, this, &UooDooWindow::stopSelected);
        connect(searchBox, &QLineEdit::textChanged, this, &UooDooWindow::refreshList);
        connect(videoList, &QListWidget::itemClicked, this, &UooDooWindow::selectVideo);

        refreshList();
    }

private slots:
    void addVideo() {
        QString filePath = QFileDialog::getOpenFileName(this, "Choose a video", QDir::homePath(), "Video files (*.mp4 *.mkv *.avi *.mov *.wmv)");
        if (filePath.isEmpty()) return;
        if (libraryModel->addVideo(filePath)) {
            refreshList();
            statusLabel->setText("Video added to the library");
        } else {
            QMessageBox::warning(this, "UooDoo", "The selected file could not be added.");
        }
    }

    void removeVideo() {
        if (currentName.isEmpty()) return;
        if (libraryModel->removeVideo(currentName)) {
            currentName.clear();
            refreshList();
            statusLabel->setText("Video removed");
        }
    }

    void toggleFavorite() {
        if (currentName.isEmpty()) return;
        if (libraryModel->toggleFavorite(currentName)) {
            refreshList();
            statusLabel->setText("Favorite state updated");
        }
    }

    void playSelected() {
        if (currentName.isEmpty()) return;
        const auto items = libraryModel->all();
        for (const auto& item : items) {
            if (item.name == currentName) {
                QProcess::startDetached("xdg-open", {item.path});
                statusLabel->setText("Launching playback");
                return;
            }
        }
    }

    void pauseSelected() {
        statusLabel->setText("Paused");
    }

    void stopSelected() {
        statusLabel->setText("Stopped");
    }

    void refreshList() {
        videoList->clear();
        const auto results = libraryModel->search(searchBox->text());
        for (const auto& item : results) {
            QString prefix = item.favorite ? "★ " : "• ";
            QListWidgetItem* row = new QListWidgetItem(prefix + item.name);
            row->setData(Qt::UserRole, item.name);
            videoList->addItem(row);
        }
        if (!results.isEmpty()) {
            if (currentName.isEmpty()) {
                currentName = results.first().name;
            }
            updateDetails(currentName);
        } else {
            currentName.clear();
            infoTitle->setText("No videos found");
            infoPath->setText("Add a local video to start your library.");
            statusLabel->setText("No media loaded");
            progressBar->setValue(0);
        }
    }

    void selectVideo(QListWidgetItem* item) {
        currentName = item->data(Qt::UserRole).toString();
        updateDetails(currentName);
    }

    void updateDetails(const QString& name) {
        const auto items = libraryModel->all();
        for (const auto& item : items) {
            if (item.name == name) {
                infoTitle->setText(item.name);
                infoPath->setText(item.path);
                statusLabel->setText(item.favorite ? "Favorite video" : "Ready to play");
                progressBar->setValue(item.favorite ? 100 : 20);
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
    QProgressBar* progressBar;
    QPushButton* addButton;
    QPushButton* favoriteButton;
    QPushButton* removeButton;
    QPushButton* playButton;
    QPushButton* pauseButton;
    QPushButton* stopButton;
    QString currentName;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    UooDooWindow window;
    window.show();
    return app.exec();
}
