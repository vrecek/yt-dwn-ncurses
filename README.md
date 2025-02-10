# TUI YT video downloader

## Requirements
- Linux  
- Terminal that support colors  
- ncurses  
- yt-dlp


## Features
- Download by:  
    - URL
    - File
- Easily editable config files


## Installation
Make sure you have:
- `ncurses` installed on your system, on Linux, there's a high chance it is installed automatically as it is a depedency for various programs
- [`yt-dlp`](https://github.com/yt-dlp/yt-dlp) accessible from your $PATH variable
- Terminal emulator of your choice that support colors

### 1. Compile and set it up yourself
```bash
git clone https://github.com/vrecek/yt-dwn-ncurses.git
cd yt-dwn-ncurses
gcc main.c utils.c -o yt-downloader -O0 -lncurses -lm
mv yt-downloader ~/.local/bin
```
Now you can use it by calling `yt-downloader`.
> [!IMPORTANT]
> Make sure `~/.local/bin` directory is in your $PATH.  
> Example for bash/zsh: In your `.bashrc` or `.zshrc` put the line: `PATH="$PATH:/home/$USER/.local/bin"`  

## Config
Config file is stored in `~/.config/ytdownloader/config.conf`. If it does not exist, it will be automatically created upon the launch.

- ### output_path [path]
Absolute path to store the downloaded files. (default: /home/$USER/Downloads)

- ### type [0|1]
Type of a downloaded file: 0=audio-only, 1=audio+video. (default: 1)

- ### audio_ext [string]
Audio-only (type 0) file extension. (default: mp3)

- ### from_file [path|0]
Absolute path of a file to download from. (default: 0)  
*Syntax:*
```
ID1 filename1
ID2 filename2
...
```
Each line should be separated be a newline, and should have two words separated by a space: **video ID**, and **output filename**

- ### cookies [path|0]
Absolute path of a verified youtube account cookie file. (default: 0)  
It allows you to download age-restricted videos  
File must be in a Netscape cookie file format. Depending on your browser, you can extract it using an extension, like `cookies.txt` (on firefox)  
> [!CAUTION]
> Be cautious when downloading extensions, if some aren't malicious now, doesn't mean they won't be in the future

## Usage
- ### Download from url  
Download video by the URL or by the video ID. Optionally, specify a file name.  
- ### Download from file  
Download video(s) from the specified file. You can also `Edit videos` without restarting the program, if you do so, you must `Update videos`  
- ### Read config  
Read your `config.conf` file. You can also `Edit config` without restarting the program, if you do so, you must `Update config`  
