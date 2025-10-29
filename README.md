# If the Moon Were Only One Pixel: 3D Interactive Solar System

![Demo of the interactive 3D Solar System](pics/demo2.png)

An interactive 3D visualization of our solar system at **true scale** — where 1 pixel equals the Moon's diameter (3,474.8 km).

Inspired by [Josh Worth's "If the Moon Were Only 1 Pixel"](https://joshworth.com/dev/pixelspace/pixelspace_solarsystem.html), this project extends the concept into fully interactive 3D space. Navigate freely, travel at light speed, and truly *experience* the vast emptiness between planets.

---

## Features

- **True astronomical scale** - All distances and sizes proportionally accurate
- **Free 3D navigation** - Explore from any angle
- **Light-speed travel** - Press C to travel at 299,792 km/s (still takes 8 minutes from Sun to Earth!)
- **Multiple speed modes** - Normal, Fast (Shift), and Light Speed
- **Planet information** - View distances, sizes, and travel times
- **Real-time rendering** - Smooth 60+ FPS

---

## Controls

| Key | Action |
|-----|--------|
| `W` `A` `S` `D` | Move forward/left/backward/right |
| `Mouse` | Look around |
| `Scroll` | Zoom in/out |
| `Shift` | Fast speed mode (hold) |
| `C` | Toggle light-speed mode |
| `ESC` | Exit |

---

## Building from Source

**Dependencies:** GLFW, GLM, OpenGL 3.3+

```bash
make
./build/main
```

Built and tested on Linux with Nix. ImGui and GLAD are included in the project.

---

## Quick Start

1. Launch the program
2. Use `W` to move forward (you start near the Sun)
3. Press `C` to enable light-speed mode
4. Navigate to Earth (about 43,000 pixels away)
5. Watch the timer — even at light speed, it takes 8+ minutes!

**Tip:** The further you travel, the more you'll appreciate the vast emptiness of space.

---

## The Scale

| Object | Distance from Sun | Travel Time at Light Speed |
|--------|------------------|---------------------------|
| Mercury | 57.9 million km | 3.2 minutes |
| Earth | 149.6 million km | 8.3 minutes |
| Mars | 227.9 million km | 12.7 minutes |
| Jupiter | 778.5 million km | 43 minutes |
| Neptune | 4.5 billion km | 4+ hours |

Remember: 1 pixel = 3,474.8 km (the Moon's diameter)

---

## What's Next

This is version 1.0. More features and improvements are planned for future releases.

---

## Acknowledgments

Inspired by Josh Worth's ["If the Moon Were Only 1 Pixel"](https://joshworth.com/dev/pixelspace/pixelspace_solarsystem.html)

Built with: OpenGL, GLFW, GLM, Dear ImGui

---

## License

MIT License - See [LICENSE](LICENSE) file

## Author

See [AUTHORS.md](AUTHORS.md) for credits

**Version 1.0**