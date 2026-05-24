# FFGUI++

## History

FFGUI++ started as a side project. At the time, I was building FFmpeg and wanted an actually decent gui to go along with it. I simply could not find one; so I made one.

## Description

FFGUI++ is my rendition of a good gui for [FFmpeg](https://www.ffmpeg.org/). It is based around [this](https://github.com/Jsoeph192/FFmpeg) fork of FFmpeg which has full libdvdcss support and more comming soon.

## Usage

#### Installer

Download the latest installer from the [releases page](https://github.com/Jsoeph192/FFGUI/releases) and install it. Then run the app from start.

#### Linux

I have only tested this on Ubuntu, there are no gauranties that it will work elsewhere. Download the latest .AppImage file and run these commands:
```Bash
chmod +x <latest-file-name>.AppImage
./<latest-file-name>.AppImage
```
*Note that Whisper models are not bundled with either release version, download recommended models from [here](https://huggingface.co/codester2835/ffgui-models/tree/main) or more from [here](https://huggingface.co/ggerganov/whisper.cpp/tree/main)*


### Encoders

There is a wide range of audio and video encoders, all of which are from FFmpeg.

### Batch

To run many files at a time, simply select them in the *Single File* tab and, if the desired output is different from the input, change the file location.

### DVD

The *DVD* tab supports reading, writing, and ripping encrypted DVDs.  **Do not use this other than for personal purposes only to comply with the DMCA (Digital Melenium Copyright Act)**

### Playback

The *Playback* tab offers support to play almost any file.

### Probe

The *Probe* tab has the ability to get the advanced metadata from any file.

### Youtube Downloader

The *Youtube Downloader* tab offers a wide range of supported websites (found [here](https://github.com/yt-dlp/yt-dlp/blob/master/supportedsites.md)). [YT-DLP](https://github.com/yt-dlp/yt-dlp/) is used.

### Whisper

The *Whisper Transcription* tab offers support to get the text from an audio file. This is only available if you enable whisper transcription in the installer or download the models from [this link](https://huggingface.co/ggerganov/whisper.cpp/tree/main)

## Build-your-own

**Ensure you have [msys2 and mingw64](https://github.com/msys2/msys2-installer/releases/) installed on your system. You will also need the [Pac-Man](https://github.com/msys2/msys2-pacman) package management system installed and up-to-date. Git is recommended**

Windows:
```CMD
git clone https://github.com/Jsoeph192/FFGUI.git FFGUI++
cd FFGUI++
build.bat
```
Or Ubuntu:
```Bash
git clone https://github.com/Jsoeph192/FFGUI.git FFGUI++
cd FFGUI++
chmod +x linux-build.sh
./linux-build.sh
```
## License

FFGUI++ is licensed under the MIT License. *See [Licence](https://github.com/Jsoeph192/FFGUI/blob/main/LICENSE).*

## Miscellaneous

Please be sure to report any issues immediately. If you like this project, feel free to give it a star ⭐.
