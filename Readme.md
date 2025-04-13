<div align="center">
<img src="icon.png" alt="fv" width="256">
</div>

**FreeVisualizer is A cute, tiny, bloat-free music visualizer for average music enjoyer**
## Dependencies
On all distros, you need a GPU driver that supports at least OpenGL 3.3
debian-based distros:
```bash
sudo apt-get install -y [to be edited]
```
fedora:
```bash
sudo dnf install -y [to be edited]
```
arch:
```bash
sudo pacman -S [to be edited]
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
### Options
```
-h, --help				Print help
-s, --scene				Which scene(shader) to use
-l, --ls                List scenes
-S, --yt-search         Search youtube and return 10 results
-d, --yt-dl             Download the audio of a YouTube video by title
-f, --fullscreen        Start in fullscreen mode
```
