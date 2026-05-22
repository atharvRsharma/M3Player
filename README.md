![CI](https://github.com/atharvRsharma/M3Player/actions/workflows/ci.yml/badge.svg)
<br></br>

<div align="center">
  <img src="icons/rdme.svg" width="80px"/>
  <h1>M3Player</h1>
  <p>A desktop media player built for people who open too many files at once.</p>
</div>

A desktop media player that lets you open multiple files side by side in a grid. Videos, audio, images, and PDFs all in one window.

## What it does

Opens media in a tiled grid layout that auto-arranges as you add files. Double-click anything to fullscreen it, double-click again to go back. You can select multiple tiles with Ctrl+click and control them together.

Drag and drop files or folders straight onto the window. Dropping a folder adds everything inside it.

**Supported formats**
- Video: mp4, mkv, avi, mov, webm, m3u8
- Audio: mp3, m4a, flac, ogg, wav
- Images: png, jpg, jpeg, webp, gif
- PDF
- Comic Book: cbz

Online video works too via the link button, it runs yt-dlp under the hood so anything yt-dlp supports should work. However, it is extremely janky and should be avoided, may very well be taken out in future releases.

## Controls

**General**

|Key|Action|
|-|-|
|`Space`|Play / pause|
|`F`|Toggle fullscreen|
|`Escape`|Exit fullscreen|
|`R`|Replay from start|
|`M`|Mute / unmute|
|`Delete`|Close selected tiles|
|`Ctrl+A`|Select all (press again to deselect all)|
|Arrow keys|Navigate between tiles|
|`+` / `-`|Zoom in / out (fullscreen)|
|Right-click|Context menu|

**Video / Audio (fullscreen)**

|Key|Action|
|-|-|
|`←` / `→`|Seek ±5 seconds|
|`↑` / `↓`|Volume up / down|
|`Ctrl` + arrow keys|Switch fullscreened tile without exiting|

**PDF (fullscreen)**

|Key|Action|
|-|-|
|`↑` / `↓`|Scroll|
|`←` / `→`|Previous / next page|
|`Ctrl+F`|Find in document|
|`=`|Toggle sidebar (bookmarks / thumbnails)|

Drag to select and copy text. Links are clickable.

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

**Comic Books**
- Basic .cbz file opening along with zoom in/out functionalities, nothing special
- Only loads one page at a time as of now, navigate with arrow keys

## Roadmap

* [ ] Workspaces — save and restore open file collections with timestamps, pages, and layout
* [ ] Recent files list
* [ ] Resizable tiles via mouse drag on borders
* [ ] Nested slots — group media by format, directory, or manual config
* [ ] Radio stream support
* [ ] Metadata loaders (Plex/Kodi/Jellyfin style)
* [ ] Additional containers: `.cbr`, `.cbx`, `.txt`
* [ ] UI overhaul
* [ ] Code consolidation pass
* [more as needs arise]

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
