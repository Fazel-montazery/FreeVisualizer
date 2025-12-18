<div align="center">
<img src="icon.png" alt="fv" width="256">
</div>

**FreeVisualizer is A cute, tiny, bloat-free music visualizer for average music enjoyer**
## Dependencies
On all distros, you need a GPU driver that supports at least OpenGL 3.3
debian-based distros:
```bash
sudo apt install libsndfile1-dev libasound2-dev libglfw3-dev
```
fedora:
```bash
sudo dnf install libsndfile-devel alsa-lib-devel glfw-devel
```
arch:
```bash
sudo pacman -S libsndfile alsa-lib glfw
```
## Build
for building from source:
```bash
git clone https://github.com/Fazel-montazery/FreeVisualizer.git
cd FreeVisualizer
make
sudo make install # installing FreeVisualizer
```
you can change the default install directory (/usr/local):
```bash
sudo make INSDIR=/path/to/install install
```
if you wanna uninstall:
```bash
sudo make uninstall
```
## Usage
```bash
fv [OPTIONS] <mp3 file>
```
## Hotkeys
```
**Q/Esc**               (Exit)
**F**                   (Toggle fullscreen)
**Space**               (Pause/Resume)
**Right/Left**          (seek 2s)
**Up/Down**             (seek 5s)
**]/[**                 (Change Amplitude scale stronger/weaker)
```
### Options
```
-h, --help				Print help
-s, --scene				Which scene(shader) to use
-l, --ls                List scenes
-S, --yt-search         Search youtube and return 10 results
-d, --yt-dl             Download the audio of a YouTube video by title
-f, --fullscreen        Start in fullscreen mode
-p, --path              Use the shader at path
-t, --test              No audio mode, just for testing out the shaders
-c, --color             Use Custom colors in the string: "#FF0000 #0000FF #00FF00 $FFFFFF" (max 4)
-r, --random            Random color pallet 
-v, --sub               Provide a [.srt] subtitle file
```
