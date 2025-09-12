# gltf_renderer

A simple OpenGL viewer for glTF (.gltf/.glb) models with camera controls, door interaction, and a procedural floor.

## Build

Requirements: g++, GLFW, GLEW, OpenGL 3.3, pthreads. On Debian/Ubuntu you might need packages like `libglfw3-dev` and `libglew-dev`.

```bash
make
```

## Run

Place your model files under `models/` (e.g., `models/TJAL.gltf` and `models/TJAL.bin`). Then:

```bash
make run
```

Controls:
- WASD: move
- Arrow keys: look around
- E: toggle nearest door
- T: cycle floor texture
- F11: toggle fullscreen
- Esc: quit

Note about large assets
- Don’t commit large binaries (`*.bin`, `*.glb`) to GitHub; they exceed the 100MB limit. Keep them locally under `models/` or use Git LFS if needed.
