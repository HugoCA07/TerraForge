# 🌍 TerraForge

![C++](https://img.shields.io/badge/C++-17-blue.svg?style=flat&logo=c%2B%2B)
![SFML](https://img.shields.io/badge/SFML-2.6-green.svg?style=flat)
![CMake](https://img.shields.io/badge/CMake-Build-orange.svg?style=flat&logo=cmake)
![License](https://img.shields.io/badge/License-MIT-lightgrey.svg?style=flat)

**TerraForge** is a 2D sandbox adventure game and custom engine built from the ground up in C++ using the Simple and Fast Multimedia Library (SFML). The project focuses on organic procedural world generation, survival mechanics, and efficient real-time rendering.

![TerraForge Gameplay](assets/screenshot.png) 
*(Note for Dev: Replace `screenshot.png` in the assets folder with an actual cool screenshot of your game!)*

## 🚀 Core Tech & Features

*   **Procedural World Generation:** Infinite terrain generation using cellular automata for caves and noise algorithms for biomes (Forests, Deserts, Snow).
*   **Smart Autotiling:** Custom bitmasking system for seamless, organic transitions between blocks (Dirt, Stone) and background walls.
*   **Custom Physics Engine:** Hand-written AABB collision detection, gravity, friction, and kinetic movement for entities and particles.
*   **Dynamic Day/Night Cycle:** Ambient lighting system that interpolates global illumination and allows torches to cast dynamic light.
*   **Living Ecosystem:** Custom AI behaviors ranging from wandering peaceful mobs (Dodos) to aggressive Boss encounters (Alpha T-Rex) with complex combat states.
*   **Crafting & Inventory:** Fully functional drag-and-drop inventory UI, crafting tables, and real-time furnace smelting mechanics.

## 🎮 Controls

| Key/Input | Action |
| :--- | :--- |
| **A / D** | Move Left / Right |
| **Space** | Jump |
| **Left Click** | Attack / Mine (Hold) |
| **Right Click** | Build Block / Interact / Eat |
| **E** | Open/Close Inventory & UI |
| **Q** | Swap between Hotbar & Armor Wheel |
| **1 - 4** | Quick select hotbar slots |
| **F5 / F6** | Quick Save / Quick Load |

## 🛠️ Build Instructions

This project uses **CMake** and requires a modern C++ compiler (C++17 or higher).

1. **Clone the repository:**
   ```bash
   git clone [https://github.com/HugoCA07/TerraForge.git](https://github.com/HugoCA07/TerraForge.git)
   cd TerraForge

🗺️ Roadmap
[x] Basic movement, physics, and sprite rendering

[x] Tile-based building and mining system

[x] Procedural biomes, cave systems, and mineral generation

[x] Drag-and-drop Crafting & Inventory UI

[x] Smart Autotiling for organic terrain

[x] Basic AI and Boss encounters

[ ] Advanced Pathfinding (A*) for enemies

[ ] Biome-specific ambient music and soundscapes

[ ] Multiplayer support (UDP/TCP)

🤝 Contributing
Contributions are what make the open-source community an amazing place to learn and create.

Fork the Project.

Create your Feature Branch (git checkout -b feature/AmazingFeature).

Commit your changes (git commit -m 'Add some AmazingFeature').

Push to the Branch (git push origin feature/AmazingFeature).

Open a Pull Request.