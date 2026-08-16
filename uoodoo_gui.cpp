#include <wx/wx.h>
#include <wx/listbox.h>
#include <wx/filedlg.h>
#include <wx/gauge.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;
using json = nlohmann::json;

// ==========================================
// Data Structures & Models
// ==========================================

struct VideoEntry {
    std::string name;
    std::string path;
    bool favorite = false;

    // JSON serialization macros from nlohmann/json
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VideoEntry, name, path, favorite)
};

class VideoLibrary {
private:
    fs::path storage_path;

public:
    std::vector<VideoEntry> videos;

    explicit VideoLibrary(const std::string& storage_file = "library.json")
        : storage_path(storage_file) {
        load();
    }

    void load() {
        if (!fs::exists(storage_path)) return;
        try {
            std::ifstream file(storage_path);
            json data;
            file >> data;
            videos = data.get<std::vector<VideoEntry>>();
        } catch (...) {
            videos.clear();
        }
    }

    void save() const {
        try {
            std::ofstream file(storage_path);
            json data = videos;
            file << data.dump(2);
        } catch (...) {
            // Handle write error if necessary
        }
    }

    bool add_video(const std::string& file_path) {
        fs::path path(file_path);
        if (!fs::exists(path) || !fs::is_regular_file(path)) return false;

        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        static const std::vector<std::string> valid_exts = {".mp4", ".mkv", ".avi", ".mov", ".wmv"};
        if (std::find(valid_exts.begin(), valid_exts.end(), ext) == valid_exts.end()) {
            return false;
        }

        std::string resolved_path = fs::canonical(path).string();
        for (const auto& item : videos) {
            if (item.path == resolved_path) return false;
        }

        videos.push_back({path.filename().string(), resolved_path, false});
        save();
        return true;
    }

    bool remove_video(const std::string& name) {
        size_t before = videos.size();
        videos.erase(std::remove_if(videos.begin(), videos.end(),
            [&name](const VideoEntry& v) { return v.name == name; }), videos.end());
        
        if (videos.size() != before) {
            save();
            return true;
        }
        return false;
    }

    bool toggle_favorite(const std::string& name) {
        for (auto& item : videos) {
            if (item.name == name) {
                item.favorite = !item.favorite;
                save();
                return True_or_False(true);
            }
        }
        return false;
    }

    std::vector<VideoEntry> search(std::string query) const {
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);
        std::vector<VideoEntry> result;
        
        for (const auto& item : videos) {
            std::string lower_name = item.name;
            std::string lower_path = item.path;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
            std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);

            if (query.empty() || lower_name.find(query) != std::string::npos || lower_path.find(query) != std::string::npos) {
                result.push_back(item);
            }
        }
        return result;
    }

private:
    bool True_or_False(bool val) { return val; } // Helper wrapper
};

// ==========================================
// GUI Application (wxWidgets)
// ==========================================

class UooDooFrame : public wxFrame {
private:
    VideoLibrary library;
    std::string current_selection;
    bool playing = false;

    // Controls
    wxTextCtrl* search_entry;
    wxListBox* listbox;
    wxStaticText* info_title;
    wxStaticText* info_path;
    wxStaticText* status_label;
    wxGauge* progress_bar;

public:
    UooDooFrame() : wxFrame(nullptr, wxID_ANY, "UooDoo - Video Library", wxDefaultPosition, wxSize(1120, 720)) {
        SetMinSize(wxSize(960, 640));
        SetBackgroundColour(wxColour("#07111f"));

        build_ui();
        refresh_view();
    }

private:
    void build_ui() {
        auto* main_sizer = new wxBoxSizer(wxVERTICAL);

        // Header Panel
        auto* header = new wxPanel(this);
        header->SetBackgroundColour(wxColour("#07111f"));
        auto* header_sizer = new wxBoxSizer(wxVERTICAL);

        auto* title = new wxStaticText(header, wxID_ANY, "UooDoo");
        title->SetFont(wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        title->SetForegroundColour(wxColour("#FFFFFF"));

        auto* subtitle = new wxStaticText(header, wxID_ANY, "Local video loading and playback workspace");
        subtitle->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        subtitle->SetForegroundColour(wxColour("#7dd3fc"));

        header_sizer->Add(title, 0, wxALIGN_LEFT);
        header_sizer->Add(subtitle, 0, wxALIGN_LEFT);
        header->SetSizer(header_sizer);
        main_sizer->Add(header, 0, wxEXPAND | wxALL, 20);

        // Controls Panel
        auto* controls = new wxPanel(this);
        controls->SetBackgroundColour(wxColour("#07111f"));
        auto* controls_sizer = new wxBoxSizer(wxHORIZONTAL);

        search_entry = new wxTextCtrl(controls, wxID_ANY, "", wxDefaultPosition, wxSize(350, -1));
        search_entry->Bind(wxEVT_TEXT, &UooDooFrame::on_search, this);

        auto* btn_add = new wxButton(controls, wxID_ANY, "Add Video");
        auto* btn_remove = new wxButton(controls, wxID_ANY, "Remove");
        auto* btn_fav = new wxButton(controls, wxID_ANY, "Favorite");

        btn_add->Bind(wxEVT_BUTTON, &UooDooFrame::on_add_video, this);
        btn_remove->Bind(wxEVT_BUTTON, &UooDooFrame::on_remove_selected, this);
        btn_fav->Bind(wxEVT_BUTTON, &UooDooFrame::on_toggle_favorite, this);

        controls_sizer->Add(search_entry, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
        controls_sizer->Add(btn_add, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        controls_sizer->Add(btn_remove, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        controls_sizer->Add(btn_fav, 0, wxALIGN_CENTER_VERTICAL);
        controls->SetSizer(controls_sizer);
        main_sizer->Add(controls, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

        // Body Panel
        auto* body = new wxPanel(this);
        body->SetBackgroundColour(wxColour("#07111f"));
        auto* body_sizer = new wxBoxSizer(wxHORIZONTAL);

        // Left Side: Library List
        auto* left_panel = new wxPanel(body);
        left_panel->SetBackgroundColour(wxColour("#07111f"));
        auto* left_sizer = new wxBoxSizer(wxVERTICAL);

        auto* lib_label = new wxStaticText(left_panel, wxID_ANY, "Library");
        lib_label->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        lib_label->SetForegroundColour(wxColour("#FFFFFF"));

        listbox = new wxListBox(left_panel, wxID_ANY, wxDefaultPosition, wxSize(300, -1));
        listbox->SetBackgroundColour(wxColour("#0f172a"));
        listbox->SetForegroundColour(wxColour("#f8fafc"));
        listbox->Bind(wxEVT_LISTBOX, &UooDooFrame::on_select, this);

        left_sizer->Add(lib_label, 0, wxALIGN_LEFT | wxBOTTOM, 8);
        left_sizer->Add(listbox, 1, wxEXPAND);
        left_panel->SetSizer(left_sizer);

        // Right Side: Details Card
        auto* card = new wxPanel(body);
        card->SetBackgroundColour(wxColour("#0f172a"));
        auto* card_sizer = new wxBoxSizer(wxVERTICAL);

        info_title = new wxStaticText(card, wxID_ANY, "Select a video");
        info_title->SetFont(wxFont(17, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        info_title->SetForegroundColour(wxColour("#FFFFFF"));

        info_path = new wxStaticText(card, wxID_ANY, "Your local MP4 videos will appear here.", wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
        info_path->SetForegroundColour(wxColour("#93c5fd"));

        status_label = new wxStaticText(card, wxID_ANY, "Ready to load a video");
        status_label->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        status_label->SetForegroundColour(wxColour("#fcd34d"));

        auto* button_row = new wxBoxSizer(wxHORIZONTAL);
        auto* btn_play = new wxButton(card, wxID_ANY, "Play");
        auto* btn_pause = new wxButton(card, wxID_ANY, "Pause");
        auto* btn_stop = new wxButton(card, wxID_ANY, "Stop");

        btn_play->Bind(wxEVT_BUTTON, &UooDooFrame::on_play, this);
        btn_pause->Bind(wxEVT_BUTTON, &UooDooFrame::on_pause, this);
        btn_stop->Bind(wxEVT_BUTTON, &UooDooFrame::on_stop, this);

        button_row->Add(btn_play, 0, wxRIGHT, 8);
        button_row->Add(btn_pause, 0, wxRIGHT, 8);
        button_row->Add(btn_stop, 0);

        progress_bar = new wxGauge(card, wxID_ANY, 100, wxDefaultPosition, wxSize(400, 15));

        auto* ctrl_label = new wxStaticText(card, wxID_ANY, "Playback controls");
        ctrl_label->SetForegroundColour(wxColour("#94a3b8"));

        card_sizer->Add(info_title, 0, wxALIGN_LEFT);
        card_sizer->Add(info_path, 0, wxALIGN_LEFT | wxTOP | wxBOTTOM, 10);
        card_sizer->Add(status_label, 0, wxALIGN_LEFT | wxBOTTOM, 15);
        card_sizer->Add(button_row, 0, wxALIGN_LEFT | wxBOTTOM, 20);
        card_sizer->Add(progress_bar, 0, wxALIGN_LEFT | wxBOTTOM, 6);
        card_sizer->Add(ctrl_label, 0, wxALIGN_LEFT);
        card->SetSizer(card_sizer);

        body_sizer->Add(left_panel, 0, wxEXPAND | wxRIGHT, 15);
        body_sizer->Add(card, 1, wxEXPAND);
        body->SetSizer(body_sizer);
        main_sizer->Add(body, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

        // Footer Panel
        auto* footer = new wxPanel(this);
        footer->SetBackgroundColour(wxColour("#07111f"));
        auto* footer_sizer = new wxBoxSizer(wxVERTICAL);
        auto* footer_label = new wxStaticText(footer, wxID_ANY, "Inspired by the UooDoo proposal: upload, play, search, favorites, and library management.");
        footer_label->SetForegroundColour(wxColour("#64748b"));
        footer_sizer->Add(footer_label, 0, wxALIGN_LEFT);
        footer->SetSizer(footer_sizer);
        main_sizer->Add(footer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

        SetSizer(main_sizer);
    }

    void refresh_view() {
        std::string query = search_entry->GetValue().ToStdString();
        auto items = library.search(query);
        listbox->Clear();

        for (const auto& item : items) {
            std::string prefix = item.favorite ? "★ " : "• ";
            listbox->Append(prefix + item.name);
        }

        if (items.empty()) {
            info_title->SetLabel("No videos found");
            info_path->SetLabel("Add a local MP4 or other supported video file to start.");
            status_label->SetLabel("No media loaded");
            progress_bar->SetValue(0);
            current_selection = "";
            return;
        }

        if (current_selection.empty()) {
            listbox->SetSelection(0);
            current_selection = items[0].name;
            update_details(&items[0]);
        } else {
            auto entry = find_entry(current_selection);
            update_details(entry);
        }
    }

    void update_details(const VideoEntry* item) {
        if (!item) return;
        info_title->SetLabel(item->name);
        info_path->SetLabel(item->path);
        status_label->SetLabel(item->favorite ? "Favorite" : "Ready to play");
        progress_bar->SetValue(item->favorite ? 100 : 20);
    }

    const VideoEntry* find_entry(const std::string& name) const {
        for (const auto& item : library.videos) {
            if (item.name == name) return &item;
        }
        return nullptr;
    }

    std::string detect_player() const {
#ifdef _WIN32
        return "start";
#else
        for (const auto& candidate : {"mpv", "vlc", "ffplay", "xdg-open"}) {
            std::string cmd = std::string("which ") + candidate + " > /dev/null 2>&1";
            if (std::system(cmd.c_str()) == 0) {
                return candidate;
            }
        }
        return "";
#endif
    }

    // Event Handlers
    void on_search(wxCommandEvent&) {
        refresh_view();
    }

    void on_select(wxCommandEvent&) {
        int sel = listbox->GetSelection();
        if (sel == wxNOT_FOUND) return;

        auto items = library.search(search_entry->GetValue().ToStdString());
        if (sel < items.size()) {
            current_selection = items[sel].name;
            update_details(&items[sel]);
        }
    }

    void on_add_video(wxCommandEvent&) {
        wxFileDialog openFileDialog(this, "Choose a video", "", "",
            "Video files (*.mp4;*.mkv;*.avi;*.mov;*.wmv)|*.mp4;*.mkv;*.avi;*.mov;*.wmv|All files (*.*)|*.*",
            wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        if (openFileDialog.ShowModal() == wxID_CANCEL) return;

        std::string path = openFileDialog.GetPath().ToStdString();
        if (library.add_video(path)) {
            refresh_view();
            status_label->SetLabel("Added video to the library");
        } else {
            wxMessageBox("The selected file could not be added.", "UooDoo", wxICON_WARNING);
        }
    }

    void on_remove_selected(wxCommandEvent&) {
        if (current_selection.empty()) return;
        if (library.remove_video(current_selection)) {
            current_selection = "";
            refresh_view();
            status_label->SetLabel("Video removed");
        }
    }

    void on_toggle_favorite(wxCommandEvent&) {
        if (current_selection.empty()) return;
        if (library.toggle_favorite(current_selection)) {
            refresh_view();
            status_label->SetLabel("Favorite state updated");
        }
    }

    void on_play(wxCommandEvent&) {
        const auto* entry = find_entry(current_selection);
        if (!entry) {
            wxMessageBox("Select a video first.", "UooDoo", wxICON_INFORMATION);
            return;
        }

        std::string player = detect_player();
        if (!player.empty()) {
            std::string command;
#ifdef _WIN32
            command = "start \"\" \"" + entry->path + "\"";
#else
            command = player + " \"" + entry->path + "\" &";
#endif
            std::system(command.c_str());
            playing = true;
            status_label->SetLabel("Now playing");
        } else {
            status_label->SetLabel("No supported player found; opening is not available in this environment");
        }
    }

    void on_pause(wxCommandEvent&) {
        playing = !playing;
        status_label->SetLabel(playing ? "Playing" : "Paused");
    }

    void on_stop(wxCommandEvent&) {
        playing = false;
        status_label->SetLabel("Stopped");
    }
};

// ==========================================
// wxWidgets Entry Point
// ==========================================

class UooDooApp : public wxApp {
public:
    virtual bool OnInit() {
        auto* frame = new UooDooFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(UooDooApp);
