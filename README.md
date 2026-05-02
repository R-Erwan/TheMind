# TheMind — Multiplayer Card Game Server

A multithreaded TCP server for the cooperative card game *The Mind*, written in C. Includes a dedicated client, an AI robot player, and automated PDF statistics reports.

## Overview

*The Mind* is a cooperative card game where players must play numbered cards (1–99) in ascending order **without communicating**. This implementation runs as a networked server supporting up to four concurrent players, with lobby management, round progression, and per-game statistics exported as PDF documents.

The project is structured as three independent executables:

| Component | Description |
|---|---|
| `TheMindServeur` | Game server — manages lobby, game state, and client connections |
| `TheMindClient` | Interactive terminal client with a help system and auto-download |
| `TheMindRobot` | Autonomous AI player, spawned as a separate process |

## Architecture

```
TheMind-main/
├── src/                     # Server
│   ├── main.c               # Entry point, connection & download threads
│   ├── Game.{h,c}           # Game state machine
│   ├── playersRessources.{h,c}  # Player list & thread-safe access
│   ├── queue.{h,c}          # Sorted card queue
│   ├── statsManager.{h,c}   # Statistics collection & PDF generation
│   └── utils.{h,c}
├── TheMindClient/src/        # Client
├── TheMindRobot/src/         # Robot AI
├── ressources/main.tex       # LaTeX template for stats PDFs
├── CMakeLists.txt
├── Dockerfile
└── install.sh
```

**Server design:**

- One thread per connected client, synchronized via `pthread` read-write locks and mutexes
- A dedicated download thread on `port+1` for file transfers, separated from game traffic
- Signal handler for graceful shutdown on `SIGINT`
- Robot players are spawned via `fork()` + `execl()` as independent processes

**Game states:** `LOBBY_STATE` → `GAME_STATE` → `PLAY_STATE` → (stats & rankings)

## Tech Stack

| Concern | Tool |
|---|---|
| Language | C (C17) |
| Build system | CMake ≥ 3.25 |
| Networking | POSIX TCP sockets |
| Concurrency | `pthread` (threads, mutexes, rwlocks) |
| Statistics | LaTeX + gnuplot → PDF |
| Containerization | Docker (debian:stable-slim) |

## Getting Started

### Prerequisites

Debian/Ubuntu:
```bash
sudo apt install gcc cmake gnuplot texlive texlive-latex-extra
```

### Build & Install

```bash
./install.sh
```

> On Windows, run `dos2unix install.sh` first if the script fails due to line-ending issues.

This compiles all three executables and places them under `TheMind/bin/`.

### Run the Server

```bash
./TheMind/bin/server/TheMindServeur <port> <backlog>
# Example:
./TheMind/bin/server/TheMindServeur 4242 10
```

The server listens for player connections on `<port>` and for file download requests on `<port+1>`.

### Connect a Client

Using the provided client (recommended):
```bash
./TheMind/bin/client/TheMindClient 127.0.0.1 4242
```

Or with netcat for quick testing:
```bash
nc localhost 4242
```

### Run with Docker

```bash
# Build
docker build -t themind-server:latest .

# Run (exposes both game and download ports)
docker run -e PORT=4242 -p 4242:4242 -p 4243:4243 themind-server:latest
```

## Player Commands

| Command | Alias | Description |
|---|---|---|
| `ready` | `r` | Mark yourself ready in the lobby |
| `unready` | `u` | Cancel ready status |
| `start` | `sg` | Start the game or the next round |
| `stop` | — | End the current game |
| `quit` | — | Disconnect |
| `add robot` | — | Add an AI player to the lobby |
| `1`–`99` | — | Play a card |

## Statistics & PDF Reports

At the end of each game the server generates a PDF report (via LaTeX + gnuplot) containing per-round statistics and a global ranking. The filename is broadcast to all players.

**Download via shell:**
```bash
echo "getfile 2024-12-11-23_30_41.pdf" | nc localhost 4243 > stats.pdf
```

**Download via client:** the PDF is fetched automatically and saved to a local `pdf/` directory.

A leaderboard is also printed in-terminal, ranking all recorded games by player count and maximum round reached:

```
Rank  Players  MaxRound  Names   Date
1     2        8         Alice Bob   2024-12-11
2     2        6         Alice Bob   2024-12-10
...
```
