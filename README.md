# Connect 4 - Console Game in C

A two-player Connect 4 game implemented in C, featuring:

- Undo/redo functionality  
- Animated piece drops  
- Colored board output in the terminal  
- Cross-platform support (Linux, Windows with `gcc`)  

---

## Table of Contents

- [Gameplay](#gameplay)  
- [Features](#features)  
- [Requirements](#requirements)  
- [Compilation](#compilation)  
- [Running the Game](#running-the-game)  
- [Controls](#controls)  
- [Contributing](#contributing)  
- [License](#license)  

---

## Gameplay

Players take turns dropping pieces into a 7-column, 6-row grid. The first player to connect **4 pieces in a row, column, or diagonal** wins. If the board fills up without a winner, the game ends in a draw.

---

## Features

- Undo and redo previous moves  
- Optional animated piece drops  
- Color-coded board for better visualization  
- Console-based, lightweight, and fast  

---

## Requirements

- GCC (tested on Alpine Linux and standard Linux)  
- POSIX-compliant terminal for colored output  

---

## Compilation

Clone the repository:

```bash
git clone https://github.com/your-username/connect4.git
cd connect4
