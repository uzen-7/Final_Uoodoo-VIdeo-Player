#include "LibraryManager.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
std::string trim(const std::string& input) {
    const auto begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(begin, end - begin + 1);
}
}  // namespace

LibraryManager::LibraryManager(const std::string& metadataPath)
    : metadataPath_(metadataPath) {
    load();
}

bool LibraryManager::addVideo(const std::string& filePath) {
    const std::filesystem::path path(filePath);
    if (!std::filesystem::exists(path)) {
        return false;
    }

    const std::string fileName = getFileName(filePath);
    if (std::any_of(videos_.begin(), videos_.end(), [&](const VideoEntry& entry) {
            return entry.name == fileName;
        })) {
        return false;
    }

    VideoEntry entry;
    entry.name = fileName;
    entry.path = std::filesystem::absolute(path).string();
    videos_.push_back(entry);
    save();
    return true;
}

bool LibraryManager::removeVideo(const std::string& fileName) {
    const auto it = std::find_if(videos_.begin(), videos_.end(), [&](const VideoEntry& entry) {
        return entry.name == fileName;
    });
    if (it == videos_.end()) {
        return false;
    }
    videos_.erase(it);
    save();
    return true;
}

std::optional<VideoEntry> LibraryManager::getVideo(const std::string& fileName) const {
    const auto it = std::find_if(videos_.begin(), videos_.end(), [&](const VideoEntry& entry) {
        return entry.name == fileName;
    });
    if (it == videos_.end()) {
        return std::nullopt;
    }
    return *it;
}

std::vector<VideoEntry> LibraryManager::getVideos() const {
    return videos_;
}

bool LibraryManager::toggleFavorite(const std::string& fileName) {
    auto it = std::find_if(videos_.begin(), videos_.end(), [&](VideoEntry& entry) {
        return entry.name == fileName;
    });
    if (it == videos_.end()) {
        return false;
    }
    it->favorite = !it->favorite;
    save();
    return true;
}

void LibraryManager::save() const {
    std::ofstream out(metadataPath_);
    if (!out) {
        throw std::runtime_error("Unable to open metadata file for writing");
    }
    for (const auto& video : videos_) {
        out << video.name << '\n' << video.path << '\n' << (video.favorite ? 1 : 0) << '\n';
    }
}

void LibraryManager::load() {
    if (!std::filesystem::exists(metadataPath_)) {
        return;
    }

    std::ifstream in(metadataPath_);
    if (!in) {
        return;
    }

    std::vector<VideoEntry> loaded;
    std::string line;
    while (std::getline(in, line)) {
        const std::string name = trim(line);
        if (name.empty()) {
            continue;
        }
        std::string path;
        std::string favoriteLine;
        if (!std::getline(in, path)) {
            break;
        }
        if (!std::getline(in, favoriteLine)) {
            break;
        }
        VideoEntry entry;
        entry.name = name;
        entry.path = trim(path);
        entry.favorite = trim(favoriteLine) == "1";
        loaded.push_back(entry);
    }
    videos_ = std::move(loaded);
}

std::string LibraryManager::getFileName(const std::string& filePath) {
    return std::filesystem::path(filePath).filename().string();
}
