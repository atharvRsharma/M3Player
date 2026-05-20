# M3Player

A desktop media player that lets you open multiple files side by side in a grid. Videos, audio, images, and PDFs all in one window.

## What it does

Opens media in a tiled grid layout that auto-arranges as you add files. Double-click anything to fullscreen it, double-click again to go back. You can select multiple tiles with Ctrl+click and control them together.

Drag and drop files or folders straight onto the window. Dropping a folder adds everything inside it.

**Supported formats**
- Video: mp4, mkv, avi, mov, webm, m3u8
- Audio: mp3, m4a, flac, ogg, wav
- Images: png, jpg, jpeg, webp, gif
- PDF

Online video works too via the link button, it runs yt-dlp under the hood so anything yt-dlp supports should work. However, it is extremely janky and should be avoided, may very well be taken out in future releases.

## Controls

**General**
- `Space` — play/pause
- `F` — fullscreen/exit fullscreen
- `Escape` — exit fullscreen
- `R` — replay from start
- `M` — mute/unmute
- `Delete` — close selected
- `Ctrl+A` — select all (run again to deselect all)
- Arrow keys — navigate between tiles
- `+` / `-` — zoom in/out (fullscreen)
- Right-click — close menu

**Video/Audio (fullscreen)**
- `←` / `→` — seek forward/backward 5 seconds
- `↑` / `↓` — volume up/down
- `Ctrl` + arrow keys — switch between fullscreened tiles without exiting

**PDF (fullscreen)**
- `↑` / `↓` — scroll
- `←` / `→` — previous/next page
- `Ctrl+F` — find in document
- `=` button — toggle side panel (bookmarks/thumbnails)
- Drag to select and copy text
- Links are clickable

## Features (fully and halfway implemented)

**Video**
- Subtitle track selection (built-in and external .srt, drop an .srt file onto a video). A button to select it through a menu is going to be added very soon.
- Audio and video track switching
- Settings panel (C key or the menu icon) shows track selectors in fullscreen
- TODO:
    - Add playback speed controls.
    - Add more VLC video suite features.

**Audio**
- Album art, title, artist pulled from metadata
- Embedded lyrics show in the settings panel if the file has them (USLT tag)
- TODO: 
    - Add user provided lyric support
    - Implement a .lrc generator and finagle it into a karaoke feature

**PDF**
- Bookmark tree and page thumbnail sidebar
- Text search with next/prev result navigation
- Text selection and copy (highly janky)
- Link detection with hover highlight
- Zoom selector with presets
- Cleans cached (loaded) pages every 3 seconds to be memory efficient
- TODO: 
    - Add proper text selection with an I-beam cursor to accompany it
    - Implement page jumping through page thumbnail interaction

**Images**
- Drag to pan, `+`/`-` to zoom
- TODO:
    - Perhaps add editing features, basic cropping and image manipulation accomodations

**TODO OVERALL**
- Add workspaces, saved collections of all files opened at the (if valid) timestamp/page/configuration to be opened at will without manually importing all the files again
- Add recent file list, ease of access
- Add more ways to zoom in/out for PDFs and images
- Allow slots to be resized by mouse drag on the borders
- Clean up UI, revitalise it
- Add radio support
- Add metadata loaders, more akin to Plex/Kodi/Jellyfin
- Add more containers to be supported (.cbx, .cbr, .txt, maybe .docx)
- Clean up files, lot of code can be consolidated or split up further to avoid spaghetti-fication
- Nested slots, open multiple slots within slots, group/ungroup media files based on format, parent directory, or user configuration
- [more as per needs]

## Dependencies

- Qt 6 (Multimedia, MultimediaWidgets, Pdf, Concurrent)
- TagLib (audio metadata)
- yt-dlp (optional, for streaming URLs, needs to be on PATH as `yt-dlp.exe`), not recommended for use

## Building

Standard CMake/qmake Qt project. Open in Qt Creator and build, or:

```bash
cmake -B build
cmake --build build
```

Make sure TagLib is findable by CMake (`TAGLIB_ROOT` or pkg-config).
