#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct VideoEntry {
    std::string name;
    std::string path;
    bool favorite = false;
};

class LibraryManager {
public:
    explicit LibraryManager(const std::string& metadataPath = "library.txt");

    bool addVideo(const std::string& filePath);
    bool removeVideo(const std::string& fileName);
    std::optional<VideoEntry> getVideo(const std::string& fileName) const;
    std::vector<VideoEntry> getVideos() const;
    bool toggleFavorite(const std::string& fileName);
    void save() const;
    void load();

private:
    std::filesystem::path metadataPath_;
    std::vector<VideoEntry> videos_;

    static std::string getFileName(const std::string& filePath);
};
