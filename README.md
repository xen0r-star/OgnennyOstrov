# Ognenny Ostrov (Огненный остров)

A tactical 2D top-down survival / Battle Royale game developed in C++ using the SFML library.

## 📖 About the Game

Deep in the isolated forests of Siberia, guards at the **Ognenny Ostrov** prison organize clandestine fights to the death. Four prisoners are dropped into an abandoned military camp: the last survivor earns their freedom.

You face 3 AI-controlled bots with survival instincts while avoiding armed guard patrols and managing constant pressure on the map through periodic position reveals.

## 🛠️ Tech Stack

* **Language:** C++
* **Graphics & Inputs:** [SFML 2.5.1](https://www.sfml-dev.org/) (handles windowing, sprites, tilemaps, camera, and mouse/keyboard inputs).
* **Compiler:** MinGW 8.x

## 🚀 Building & Running

```bash
# Clone the repository
git clone https://github.com/xen0r-star/OgnennyOstrov.git
cd OgnennyOstrov

# Build instructions (CMake - requires MinGW 8.x)
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```