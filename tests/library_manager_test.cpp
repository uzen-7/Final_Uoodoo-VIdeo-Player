#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "LibraryManager.h"

int main() {
    const std::filesystem::path temp_db = std::filesystem::temp_directory_path() / "uoodoo_test_library.txt";
    const std::filesystem::path temp_video = std::filesystem::temp_directory_path() / "sample.mp4";
    if (std::filesystem::exists(temp_db)) {
        std::filesystem::remove(temp_db);
    }
    if (std::filesystem::exists(temp_video)) {
        std::filesystem::remove(temp_video);
    }

    std::ofstream video_out(temp_video);
    video_out << "fake mp4 content";
    video_out.close();

    LibraryManager manager(temp_db.string());
    const bool added = manager.addVideo(temp_video.string());
    assert(added);
    assert(manager.getVideos().size() == 1);

    const auto favorite_result = manager.toggleFavorite("sample.mp4");
    assert(favorite_result);
    const auto video = manager.getVideo("sample.mp4");
    assert(video.has_value());
    assert(video->favorite);

    std::cout << "library manager tests passed" << std::endl;
    return 0;
}
