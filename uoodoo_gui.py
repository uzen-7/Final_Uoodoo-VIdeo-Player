#!/usr/bin/env python3
import json
import os
import shutil
import subprocess
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from pathlib import Path
from dataclasses import dataclass, asdict


@dataclass
class VideoEntry:
    name: str
    path: str
    favorite: bool = False


class VideoLibrary:
    def __init__(self, storage_path: str = "library.json"):
        self.storage_path = Path(storage_path)
        self.videos: list[VideoEntry] = []
        self.load()

    def load(self) -> None:
        if not self.storage_path.exists():
            return
        try:
            data = json.loads(self.storage_path.read_text(encoding="utf-8"))
            self.videos = [VideoEntry(**item) for item in data]
        except Exception:
            self.videos = []

    def save(self) -> None:
        self.storage_path.write_text(json.dumps([asdict(item) for item in self.videos], indent=2), encoding="utf-8")

    def add_video(self, file_path: str) -> bool:
        path = Path(file_path)
        if not path.exists():
            return False
        if not path.is_file():
            return False
        if path.suffix.lower() not in {".mp4", ".mkv", ".avi", ".mov", ".wmv"}:
            return False
        if any(item.path == str(path.resolve()) for item in self.videos):
            return False
        self.videos.append(VideoEntry(name=path.name, path=str(path.resolve()), favorite=False))
        self.save()
        return True

    def remove_video(self, name: str) -> bool:
        before = len(self.videos)
        self.videos = [item for item in self.videos if item.name != name]
        if len(self.videos) != before:
            self.save()
            return True
        return False

    def toggle_favorite(self, name: str) -> bool:
        for item in self.videos:
            if item.name == name:
                item.favorite = not item.favorite
                self.save()
                return True
        return False

    def search(self, query: str) -> list[VideoEntry]:
        query = query.lower()
        return [item for item in self.videos if query in item.name.lower() or query in item.path.lower()]


class UooDooApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("UooDoo - Video Library")
        self.root.geometry("1120x720")
        self.root.minsize(960, 640)
        self.root.configure(bg="#07111f")

        self.library = VideoLibrary("library.json")
        self.current_selection = None
        self.playing = False

        self.build_ui()
        self.refresh_view()

    def build_ui(self) -> None:
        style = ttk.Style(self.root)
        style.theme_use("clam")
        style.configure("TFrame", background="#07111f")
        style.configure("TLabel", background="#07111f", foreground="#f5f7fb")
        style.configure("TButton", background="#0f172a", foreground="#f5f7fb")
        style.map("TButton", background=[("active", "#1d4ed8")])

        header = ttk.Frame(self.root, padding=20)
        header.pack(fill="x")
        ttk.Label(header, text="UooDoo", font=("Segoe UI", 22, "bold")).pack(anchor="w")
        ttk.Label(header, text="Local video loading and playback workspace", font=("Segoe UI", 11), foreground="#7dd3fc").pack(anchor="w")

        controls = ttk.Frame(self.root, padding=(20, 0, 20, 10))
        controls.pack(fill="x")

        self.search_var = tk.StringVar()
        self.search_var.trace_add("write", lambda *_: self.refresh_view())
        search_entry = ttk.Entry(controls, textvariable=self.search_var, width=50)
        search_entry.pack(side="left")

        ttk.Button(controls, text="Add Video", command=self.add_video).pack(side="left", padx=(10, 0))
        ttk.Button(controls, text="Remove", command=self.remove_selected).pack(side="left", padx=(8, 0))
        ttk.Button(controls, text="Favorite", command=self.toggle_favorite).pack(side="left", padx=(8, 0))

        body = ttk.Frame(self.root, padding=(20, 5, 20, 20))
        body.pack(fill="both", expand=True)

        left = ttk.Frame(body)
        left.pack(side="left", fill="y", padx=(0, 15))
        ttk.Label(left, text="Library", font=("Segoe UI", 14, "bold")).pack(anchor="w")
        self.listbox = tk.Listbox(left, width=44, height=18, bg="#0f172a", fg="#f8fafc", borderwidth=0, highlightthickness=0)
        self.listbox.pack(fill="y", expand=True, pady=(8, 0))
        self.listbox.bind("<<ListboxSelect>>", self.on_select)

        right = ttk.Frame(body)
        right.pack(side="left", fill="both", expand=True)
        card = ttk.Frame(right, padding=20)
        card.pack(fill="both", expand=True)
        card.configure(style="Card.TFrame")

        self.info_title = ttk.Label(card, text="Select a video", font=("Segoe UI", 17, "bold"))
        self.info_title.pack(anchor="w")
        self.info_path = ttk.Label(card, text="Your local MP4 videos will appear here.", wraplength=560, foreground="#93c5fd")
        self.info_path.pack(anchor="w", pady=(8, 12))

        self.status_var = tk.StringVar(value="Ready to load a video")
        ttk.Label(card, textvariable=self.status_var, foreground="#fcd34d", font=("Segoe UI", 11, "bold")).pack(anchor="w")

        button_row = ttk.Frame(card)
        button_row.pack(anchor="w", pady=(20, 8))
        ttk.Button(button_row, text="Play", command=self.play_selected).pack(side="left")
        ttk.Button(button_row, text="Pause", command=self.pause_selected).pack(side="left", padx=(8, 0))
        ttk.Button(button_row, text="Stop", command=self.stop_selected).pack(side="left", padx=(8, 0))

        self.progress = ttk.Progressbar(card, orient="horizontal", length=520, mode="determinate")
        self.progress.pack(anchor="w", pady=(18, 6))
        ttk.Label(card, text="Playback controls", foreground="#94a3b8").pack(anchor="w")

        footer = ttk.Frame(self.root, padding=(20, 0, 20, 20))
        footer.pack(fill="x")
        ttk.Label(footer, text="Inspired by the UooDoo proposal: upload, play, search, favorites, and library management.", foreground="#64748b", wraplength=900).pack(anchor="w")

    def refresh_view(self) -> None:
        query = self.search_var.get().strip()
        items = self.library.search(query)
        self.listbox.delete(0, tk.END)
        for item in items:
            prefix = "★ " if item.favorite else "• "
            self.listbox.insert(tk.END, f"{prefix}{item.name}")
        if not items:
            self.info_title.configure(text="No videos found")
            self.info_path.configure(text="Add a local MP4 or other supported video file to start.")
            self.status_var.set("No media loaded")
            self.progress.stop()
            self.current_selection = None
            return
        if self.current_selection is None:
            self.listbox.selection_clear(0, tk.END)
            self.listbox.select_set(0)
            self.current_selection = items[0].name
            self.update_details(items[0])
        else:
            self.update_details(self.find_entry(self.current_selection))

    def on_select(self, _event=None) -> None:
        selection = self.listbox.curselection()
        if not selection:
            return
        index = selection[0]
        item = self.library.search(self.search_var.get().strip())[index]
        self.current_selection = item.name
        self.update_details(item)

    def update_details(self, item: VideoEntry | None) -> None:
        if not item:
            return
        self.info_title.configure(text=item.name)
        self.info_path.configure(text=item.path)
        self.status_var.set("Favorite" if item.favorite else "Ready to play")
        if item.favorite:
            self.progress.configure(value=100)
        else:
            self.progress.configure(value=20)

    def find_entry(self, name: str) -> VideoEntry | None:
        for item in self.library.videos:
            if item.name == name:
                return item
        return None

    def add_video(self) -> None:
        file_path = filedialog.askopenfilename(title="Choose a video", filetypes=[("Video files", "*.mp4 *.mkv *.avi *.mov *.wmv"), ("All files", "*.*")])
        if not file_path:
            return
        if self.library.add_video(file_path):
            self.refresh_view()
            self.status_var.set("Added video to the library")
        else:
            messagebox.showwarning("UooDoo", "The selected file could not be added.")

    def remove_selected(self) -> None:
        if not self.current_selection:
            return
        if self.library.remove_video(self.current_selection):
            self.current_selection = None
            self.refresh_view()
            self.status_var.set("Video removed")

    def toggle_favorite(self) -> None:
        if not self.current_selection:
            return
        if self.library.toggle_favorite(self.current_selection):
            self.refresh_view()
            self.status_var.set("Favorite state updated")

    def play_selected(self) -> None:
        entry = self.find_entry(self.current_selection) if self.current_selection else None
        if not entry:
            messagebox.showinfo("UooDoo", "Select a video first.")
            return
        player = self.detect_player()
        if player:
            try:
                subprocess.Popen([player, entry.path])
                self.playing = True
                self.status_var.set("Now playing")
            except Exception as exc:
                self.status_var.set(f"Playback failed: {exc}")
        else:
            self.status_var.set("No supported player found; opening is not available in this environment")

    def pause_selected(self) -> None:
        self.playing = not self.playing
        self.status_var.set("Paused" if not self.playing else "Playing")

    def stop_selected(self) -> None:
        self.playing = False
        self.status_var.set("Stopped")

    def detect_player(self) -> str | None:
        for candidate in ["mpv", "vlc", "ffplay", "xdg-open"]:
            if shutil.which(candidate):
                return candidate
        return None

    def run(self) -> None:
        self.root.mainloop()


def main() -> None:
    root = tk.Tk()
    app = UooDooApp(root)
    app.run()


if __name__ == "__main__":
    main()
