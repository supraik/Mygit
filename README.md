# MyGit — A Git Clone Built From Scratch

> A fully functional version control system written in **C++17** with a **Go** remote server, replicating Git's core internals from the ground up.

---

## Table of Contents

- [What Is This?](#what-is-this)
- [Architecture](#architecture)
- [Repository Structure](#repository-structure)
- [Commands Reference](#commands-reference)
- [Internal Object Model](#internal-object-model)
- [Building the Client](#building-the-client)
- [Running the Server](#running-the-server)
- [Docker Deployment](#docker-deployment)
- [Push / Pull Workflow](#push--pull-workflow)
- [How It Works Internally](#how-it-works-internally)
- [Limitations vs Real Git](#limitations-vs-real-git)
- [Technologies Used](#technologies-used)

---

## What Is This?

MyGit is a **ground-up reimplementation of Git** — not a wrapper, not a library. Every object type (blob, tree, commit), every command, and every protocol detail is hand-coded:

- SHA1 content-addressable object storage
- Staged index (staging area)
- Commit history with parent-chain traversal
- Branch creation, deletion, and checkout
- Three-way merge with conflict detection
- Remote push/pull over HTTP using raw sockets
- A Go REST server that acts as the remote repository

---

## Architecture

```
┌─────────────────────────────────────┐      HTTP/TCP
│        mygit (C++ Client)           │◄────────────────►  ┌──────────────────┐
│                                     │                     │  Go HTTP Server  │
│  init / add / commit / log          │    POST /object/    │   (port 8080)    │
│  branch / checkout / merge          │    GET  /object/    │                  │
│  push / pull / remote               │    POST /ref/       │  Stores objects  │
│                                     │    GET  /refs       │  and branch refs │
└─────────────────────────────────────┘                     └──────────────────┘
              │
              ▼
         .mygit/
         ├── HEAD
         ├── config
         ├── index
         ├── objects/
         │   └── ab/cdef123...
         └── refs/heads/master
```

---

## Repository Structure

```
├── src/
│   ├── main.cpp               # CLI entry point — command dispatcher
│   ├── commands/
│   │   ├── init.cpp/.h        # Repository initialization
│   │   ├── add.cpp/.h         # Staging files (blob hashing + index update)
│   │   ├── rm.cpp/.h          # Removing files from index
│   │   ├── status.cpp/.h      # Show staged files
│   │   ├── ls-files.cpp/.h    # List all indexed files
│   │   ├── cat-file.cpp/.h    # Inspect objects by hash
│   │   ├── commit.cpp/.h      # Create tree + commit objects
│   │   ├── log.cpp/.h         # Traverse commit history
│   │   ├── branch.cpp/.h      # Create / delete branches
│   │   ├── checkout.cpp/.h    # Switch branches or restore commits
│   │   ├── merge.cpp/.h       # Fast-forward and three-way merge
│   │   ├── remote.cpp/.h      # Manage remote URLs in config
│   │   ├── push.cpp/.h        # Upload objects + update remote ref
│   │   └── pull.cpp/.h        # Fetch objects + update local ref
│   └── utils/
│       ├── hash.cpp/.h        # SHA1 via Windows CryptoAPI
├── server/
│   ├── main.go                # Go HTTP server (REST API for remote)
│   ├── go.mod                 # Go module definition
│   └── Dockerfile             # Container image for deployment
├── CMakeLists.txt             # CMake build configuration
└── build/                     # Out-of-source build artifacts
```

---

## Commands Reference

| Command | Description |
|---|---|
| `mygit init` | Initialize a new `.mygit` repository in the current directory |
| `mygit add <file>` | Hash file content, store as blob object, update index |
| `mygit rm <file>` | Remove a file entry from the staging index |
| `mygit status` | List all currently staged files |
| `mygit ls-files` | List all files tracked in the index |
| `mygit cat-file -p <hash>` | Pretty-print object content |
| `mygit cat-file -t <hash>` | Show object type (blob/tree/commit) |
| `mygit cat-file -s <hash>` | Show object size in bytes |
| `mygit commit -m "msg"` | Create tree + commit objects, advance HEAD |
| `mygit log` | Walk the parent chain and print commit history |
| `mygit branch <name>` | Create a new branch at current HEAD |
| `mygit branch -d <name>` | Delete a branch reference |
| `mygit checkout <name>` | Switch to a branch or detach HEAD to a commit hash |
| `mygit merge <branch>` | Merge target branch (fast-forward or three-way) |
| `mygit remote add <name> <url>` | Register a remote server URL in config |
| `mygit push <remote> [branch]` | Upload all objects + update remote branch ref |
| `mygit pull <remote> [branch]` | Fetch missing objects + update local branch ref |

---

## Internal Object Model

All data is stored as **immutable content-addressed objects** in `.mygit/objects/`:

### Blob (file content)
```
blob <size>\0<raw file bytes>
```

### Tree (directory snapshot)
```
tree <size>\0100644 blob <hash>\t<filename>\n...
```
Files are sorted alphabetically so the same directory always produces the same tree hash.

### Commit
```
commit <size>\0tree <tree-hash>
parent <parent-hash>
author <timestamp>

<commit message>
```

**Object addressing:** SHA1 hash of the full object bytes → `ab/cdef123...` (2-char dir + 38-char file).

---

## Building the Client

### Prerequisites
- CMake 3.10+
- Visual Studio or MSVC (Windows; uses `crypt32` and `ws2_32`)
- C++17-capable compiler

### Steps

```sh
# Configure (generates Visual Studio solution or Makefiles)
cmake -B build

# Build
cmake --build build

# Run
./build/Debug/mygit.exe init
```

Alternatively open `build/mygit.slnx` in Visual Studio and build from there.

---

## Running the Server

### Direct (Go installed)

```sh
cd server
go run main.go
# Listening on :8080
```

```sh
# Custom port
PORT=9000 go run main.go
```

### Available endpoints

| Method | Endpoint | Description |
|---|---|---|
| GET | `/health` | Liveness check — returns `OK` |
| GET | `/refs` | List all branches: `hash refs/heads/name\n` |
| GET | `/ref/{name}` | Get commit hash for branch `name` |
| POST | `/ref/{name}` | Update branch `name` to new commit hash (body) |
| GET | `/object/{hash}` | Download raw object bytes |
| POST | `/object/{hash}` | Upload raw object bytes |

---

## Docker Deployment

```sh
cd server
docker build -t mygit-server .
docker run -p 8080:8080 -v $(pwd)/repo:/app/.mygit mygit-server
```

The server auto-detects the `PORT` environment variable (for Heroku, Railway, Render, etc.).

---

## Push / Pull Workflow

```sh
# 1. Initialize and make commits
mygit init
mygit add main.cpp
mygit commit -m "initial commit"

# 2. Register the remote server
mygit remote add origin http://localhost:8080

# 3. Push: uploads all objects + updates remote branch pointer
mygit push origin master

# --- on another machine / clone ---

# 4. Pull: fetches all missing objects + updates local branch pointer
mygit pull origin master
```

**What push does internally:**
1. Reads remote URL from `.mygit/config`
2. BFS-traverses the commit DAG to collect all reachable objects
3. HTTP POST each object to `/object/<hash>` on the server
4. HTTP POST the branch hash to `/ref/<branch>` on the server

**What pull does internally:**
1. HTTP GET `/refs` to discover remote branch hashes
2. BFS from remote tip to find objects not yet stored locally
3. HTTP GET `/object/<hash>` for each missing object
4. Writes updated local branch ref

---

## How It Works Internally

### Hashing (SHA1)
Uses **Windows CryptoAPI** (`crypt32.dll`):
```cpp
CryptAcquireContext → CryptCreateHash(CALG_SHA1) → CryptHashData → CryptGetHashParam
```
The blob header `"blob <size>\0"` is prepended before hashing so the same raw content in different contexts produces different hashes (matching Git's behavior).

### Index (Staging Area)
Flat text file `.mygit/index`:
```
100644 <sha1-hash> <filepath>
```
Loaded into `std::map<string,string>` (sorted → deterministic tree hashes), modified, written back.

### Merge Algorithm
1. Collect ancestor sets for both branch tips via BFS
2. Find intersection → **merge base** (lowest common ancestor)
3. If merge base == current HEAD → **fast-forward** (just move pointer)
4. Otherwise → **three-way merge**: compare file sets of base, ours, theirs
5. Conflict: both sides modified the same file differently → write conflict markers

### Networking
Raw **Winsock2** TCP sockets (`ws2_32.dll`) — no HTTP library. Push/pull construct HTTP/1.1 request strings manually, send via `send()`, and parse the response from `recv()`.

---

## Limitations vs Real Git

| Feature | MyGit | Real Git |
|---|---|---|
| Object compression | ❌ Raw bytes | ✅ zlib/deflate |
| Pack files | ❌ Loose objects only | ✅ Delta-compressed packs |
| Subdirectory tracking | ❌ Flat root only | ✅ Nested trees |
| Authentication | ❌ None | ✅ SSH keys, HTTPS tokens |
| SHA-256 support | ❌ SHA1 only | ✅ sha256 object format |
| Platform | ❌ Windows only (WinCAPI) | ✅ Cross-platform |
| Rebase | ❌ | ✅ |
| Stash | ❌ | ✅ |
| Reflog | ❌ | ✅ |
| Hooks | ❌ | ✅ |
| .gitignore | ❌ | ✅ |
| Untracked file detection in status | ❌ | ✅ |

---

## Technologies Used

| Component | Technology | Details |
|---|---|---|
| Client | C++17 | CMake, std::filesystem, std::map |
| Hashing | Windows CryptoAPI | `crypt32.dll`, SHA1 |
| Networking (client) | Winsock2 | Raw TCP sockets, HTTP/1.1 |
| Server | Go (stdlib only) | `net/http`, `os`, `encoding/json` |
| Build | CMake 3.10+ | MSVC, out-of-source build |
| Containerization | Docker | Single-binary Go image |

---

*Built to understand Git internals from the ground up — every object, every hash, every network byte.*
