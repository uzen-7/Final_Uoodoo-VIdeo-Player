#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "LibraryManager.h"

void printHelp() {
    std::cout << "UooDoo console demo\n";
    std::cout << "Commands:\n";
    std::cout << "  add <path>       Add a local MP4 file to the library\n";
    std::cout << "  list             Show all videos in the library\n";
    std::cout << "  favorite <name>  Toggle favorite state for a video\n";
    std::cout << "  remove <name>    Remove a video from the library\n";
    std::cout << "  help             Show this help text\n";
    std::cout << "  quit             Exit\n";
}

int main() {
    LibraryManager manager("library.txt");
    std::cout << "UooDoo: Video Loading and Playback System\n";
    printHelp();

    std::string command;
    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, command);

        if (command == "quit") {
            break;
        }
        if (command == "help") {
            printHelp();
            continue;
        }

        std::istringstream iss(command);
        std::string action;
        iss >> action;

        if (action == "add" && iss.good()) {
            std::string path;
            iss >> path;
            if (manager.addVideo(path)) {
                std::cout << "Added video to library\n";
            } else {
                std::cout << "Could not add video. Check the file path and type.\n";
            }
        } else if (action == "list") {
            const auto videos = manager.getVideos();
            if (videos.empty()) {
                std::cout << "No videos in the library yet.\n";
            } else {
                for (const auto& video : videos) {
                    std::cout << "- " << video.name << " [favorite=" << (video.favorite ? "yes" : "no") << "]\n";
                }
            }
        } else if (action == "favorite" && iss.good()) {
            std::string name;
            iss >> name;
            if (manager.toggleFavorite(name)) {
                std::cout << "Favorite state updated\n";
            } else {
                std::cout << "Video not found\n";
            }
        } else if (action == "remove" && iss.good()) {
            std::string name;
            iss >> name;
            if (manager.removeVideo(name)) {
                std::cout << "Video removed\n";
            } else {
                std::cout << "Video not found\n";
            }
        } else {
            std::cout << "Unknown command. Type 'help' to see available commands.\n";
        }
    }

    return 0;
}
