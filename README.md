# Dungeon Escape 🎮

**An immersive C++ and SFML text-based adventure game where strategy, resource management, and critical choices determine your fate.**

Welcome to Dungeon Escape! This project is a complete, text-driven adventure game built with C++14 and the SFML multimedia library. It showcases a strong application of Object-Oriented Programming (OOP), custom data structures, and modern C++ features. The player awakens in a dungeon and must navigate a series of dangerous rooms, battle enemies, and make a crucial choice to find the key to their freedom.

---

## Table of Contents
* [Core Features ✨](#core-features)
* [Gameplay Mechanics & Flow ⚔️](#gameplay-mechanics--flow)
* [Technical Stack 💻](#technical-stack)
* [System Architecture & Design 🏗️](#system-architecture--design)
    * [Class Overview](#class-overview)
    * [Inheritance & Polymorphism](#inheritance--polymorphism)
    * [Data Structures & Algorithms](#data-structures--algorithms)
* [How to Build and Run ⚙️](#how-to-build-and-run)

---

## Core Features ✨
* **Modern SFML Interface:** A graphical window powered by SFML replaces a standard console, providing a more engaging user experience.
* **Animated Visuals:** The game features animated menu backgrounds, screen shake effects during combat, and damage flashes to enhance player immersion.
* **State-Based Game Management:** A robust state machine manages the game's flow between the main menu, name input screen, core gameplay, and the final game-over screen.
* **Interactive Dungeon Crawl:** Navigate a linear series of interconnected rooms, each with a unique description, background, and a potential encounter—be it a valuable item, a fearsome enemy, or a difficult choice.
* **Strategic Turn-Based Combat:** The combat system's outcome, especially against the final boss, is directly influenced by the player's preparation and item choices.
* **Critical Path Choices:** The player must make a single, crucial choice between two powerful items, which directly determines whether victory is possible.

---

## Gameplay Mechanics & Flow ⚔️

The game is a linear adventure that tests the player's resource management and strategic thinking.

### The Journey
The player progresses through a predefined sequence of rooms:
`Entrance` -> `Wizard's Sanctum` -> `Dragon's Lair` -> `Zombie's Crypt` -> `Chamber of Blades` -> `Room of Choice` -> `Monster's Den` -> `Boss Chamber` -> `Final Door`

Moving between rooms (forward or backward) costs **Move** points. If the player's **Health** or **Moves** drop to zero, the game is lost.

### The Final Boss: A Test of Preparation
In the `Boss Chamber`, the player faces the ultimate challenge. The outcome is determined by a key item:
* **Without the Ultimate Sword:** The boss is overwhelming, dealing a massive **75 HP** of damage.
* **With the Ultimate Sword:** Found in the `Chamber of Blades`, the sword's magic awakens, weakening the boss and reducing its damage to a more manageable **50 HP**.

### The Golden Key: The True Path to Victory 🔑
In the `Room of Choice`, the player must choose between a tempting **full Health Potion** and the **Golden Key**. While the potion offers immediate survival, **the Golden Key is essential to winning the game.**

To escape through the `Final Door`, the player must satisfy two conditions:
1.  Possess the **Golden Key**.
2.  Have **defeated the Final Boss**.

Failing to meet both conditions upon reaching the door results in being trapped forever—a loss. This design ensures the player must face every major challenge to earn their victory.

---

## Technical Stack 💻
* **Language:** **C++14**
* **Core Library:** **SFML 2.5.1**
    * `SFML/Graphics`: For all rendering (sprites, text, window).
    * `SFML/Window`: For window management and user input.
    * `SFML/System`: For clocks and vector math.
    * `SFML/Audio`: For background music.
* **C++ Standard Library:**
    * `<iostream>`, `<string>`, `<sstream>`
    * `<vector>`, `<map>`, `<deque>`
    * `<memory>` (for `std::unique_ptr`)
    * `<algorithm>` (for `std::sort`)
    * `<stdexcept>`

---

## System Architecture & Design 🏗️

The project is built on a foundation of clean, modular, and extensible object-oriented code.

### Class Overview

| Class           | Description                                                                                             |
| :-------------- | :------------------------------------------------------------------------------------------------------ |
| **Game** | The central engine. Manages the main game loop, screens, window, and global state.                      |
| **Player** | Represents the user's character, holding health, moves, and inventory.                                  |
| **Dungeon** | Manages the linked-list structure of `Room` objects that form the game world.                             |
| **Room** | A single node in the dungeon, containing a description, background, and an optional `Entity`.             |
| **Entity** | **Abstract base class** for any interactive object in a room (e.g., items, enemies).                      |
| **Item** | A concrete `Entity` representing a collectible item.                                                    |
| **Enemy** | A concrete `Entity` representing a hostile creature.                                                    |
| **Screen** | **Abstract base class** for a game state's UI (e.g., menu, gameplay). Defines the core interface.         |
| **GamePlayScreen**| The screen where the primary game logic and player interaction occur.                                  |
| **Inventory<T>**| A **custom template class** (dynamic array) to hold the player's items.                                 |
| **Stack<T>** | A **custom template class** (linked list) used to track the player's path for backtracking.             |
| **ResourceManager**| A static class for loading and caching assets (fonts, textures) to prevent redundant file I/O.        |

### Inheritance & Polymorphism

The project leverages polymorphism for flexible and decoupled game logic:
* **Entity Hierarchy:** A `Room` can hold a pointer to any `Entity` (`Item` or `Enemy`). This allows the game to handle interactions polymorphically via the virtual `interact()` method without the `Room` needing to know the object's specific type.
* **Screen Hierarchy:** The `Game` class manages a pointer to the current `Screen`. It can call `handleInput()`, `update()`, and `draw()` on the current screen without knowing if it's the `MenuScreen`, `GamePlayScreen`, or `GameOverScreen`.

### Data Structures & Algorithms

The project uses a mix of custom and standard library data structures for optimal performance and logic.

#### Custom Data Structures
* **`Stack<T>` (Linked List Implementation):** Used by the `Dungeon` to track the player's path. When the player moves, the previous room is pushed onto the stack, allowing for an efficient `backtrack()` (pop) operation.
* **`Inventory<T>` (Dynamic Array Implementation):** Used by the `Player` to store item names. It automatically resizes itself by doubling its capacity when full, demonstrating dynamic memory management.

#### Standard Template Library (STL)
* **`std::map`:** Used heavily in the `ResourceManager` to cache assets (mapping filenames to textures/fonts) and in the `Game` class to map `GameStateID` enums to their corresponding `Screen` objects.
* **`std::deque`:** Used in the `GamePlayScreen` to manage the `actionsLog`. A deque is ideal here for its efficiency in adding new messages to the front.
* **`std::sort`:** This algorithm is used by the `Inventory` class to provide an alphabetically sorted list of the player's items for a clean UI display.

---

## How to Build and Run ⚙️

This project is built on Windows using the **MinGW-w64** toolchain (`g++` and `mingw32-make`). It links to **SFML 2.5.1 statically**.

### Prerequisites
1.  **Git:** Required to clone the repository.
2.  **MinGW-w64:** You need the MinGW-w64 toolchain which provides `g++` and `mingw32-make`. Make sure its `bin` directory is added to your system's PATH.
3.  **SFML 2.5.1 (Static):** Download the official SFML 2.5.1 release that matches your version of MinGW (e.g., 32-bit or 64-bit). You will need the path to this folder for the setup.

### Setup & Build Steps

1.  **Clone the Repository**
    Open your terminal (like PowerShell or Command Prompt) and run:
    ```bash
    git clone [https://github.com/Fahad2129/Dungeon-Escape-31659-.git](https://github.com/Fahad2129/Dungeon-Escape-31659-.git)
    cd Dungeon-Escape-31659-
    ```

2.  **CRITICAL: Update the Makefile**
    The provided `Makefile` contains a hardcoded path to the SFML library. You **must** change this to match the location where you unzipped SFML on your computer.

    * Open the `Makefile` in a text editor.
    * Find these lines:
        ```makefile
        compile:
            g++ -c main.cpp -I"C:\\Users\\Fahad Azfar\\Documents\\libraries\\SFML-2.5.1\\include" -DSFML_STATIC

        link:
            g++ main.o -o main -L"C:\Users\Fahad Azfar\Documents\\libraries\\SFML-2.5.1\\lib" \
            ...
        ```
    * Replace both paths (`C:\\Users\\Fahad Azfar\\...`) with the correct path to your own SFML folder.
    * **Important:** Remember to use double backslashes (`\\`) for the path separators.

3.  **Compile the Project**
    In your terminal, from the project's root directory, run the `mingw32-make` command. This will compile the source code and link the executable.
    ```bash
    mingw32-make
    ```
    *(If you need to do a fresh build, you can clean up old files first by running `mingw32-make clean`)*

4.  **Run the Game**
    Once the build is successful, an executable named `main.exe` will be created in the directory. Run it with this command:
    ```bash
    .\main.exe
    ```
