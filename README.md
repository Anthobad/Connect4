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
gcc -Wall -Werror -o connect4 play.c gamelogic.c ui.c
./connect4
./connect4 --no-anim
```

## Controls

During your turn, you can:

- `1-7` → Drop a piece into the chosen column  
- `u` → Undo the last move  
- `r` → Redo a previously undone move  
- `q` → Quit the game  

After the game ends, you will be prompted to **play again** either input **y** or **n**.

---

## Contributing

Contributions are welcome! You can:

- Add new features (e.g., AI opponent)  
- Improve UI or animations  
- Fix bugs or optimize the code  

**How to contribute:**

1. Fork the repository.  
2. Create a new branch: `git checkout -b feature-name`.  
3. Make your changes.  
4. Commit your changes: `git commit -m "Add feature"`.  
5. Push to your branch: `git push origin feature-name`.  
6. Open a Pull Request on GitHub.
