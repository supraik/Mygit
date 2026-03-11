# Docker Hands-On Guide (Pressure-Driven)

> **Goal:** You will *use* Docker while you learn it. Each section has a short “Do it now” checkpoint. Don’t read ahead—run the commands first. If something fails, fix it before continuing.

---

## 1) What Docker is and Why It’s Used

**Docker** is a platform for packaging software so it runs the same way on any machine. It bundles your app, its dependencies, and runtime into a **Docker image**, and you execute that image as a **container**.

**Why it’s used:**
- **Consistency:** “It works on my machine” becomes “it works everywhere.”
- **Speed:** Start environments in seconds.
- **Isolation:** Dependencies don’t conflict with other apps.
- **Portability:** Deploy the same image to dev, staging, production.

**Do it now:**
1. Install Docker Desktop if you don’t have it.
2. Run:
   ```bash
   docker version
   ```
   You should see client and server details.

---

## 2) Core Docker Concepts and Functionalities

**Key ideas you must internalize:**
- **Image** = immutable blueprint (like a class).
- **Container** = running instance of an image (like an object).
- **Registry** = image storage (e.g., Docker Hub).
- **Dockerfile** = instructions to build an image.
- **Volume** = persistent data storage outside containers.
- **Network** = isolated virtual network for containers.

**Do it now:**
1. Pull a tiny image:
   ```bash
   docker pull hello-world
   ```
2. List images:
   ```bash
   docker images
   ```
3. Run it:
   ```bash
   docker run hello-world
   ```
4. List containers (including stopped):
   ```bash
   docker ps -a
   ```

---

## 3) How Docker Images Work

An **image** is a read-only stack of layers. Each layer is a filesystem change. When you build an image, Docker caches layers to speed up rebuilds.

### Minimal Image Example
Create a folder and add a file called `Dockerfile`:
```Dockerfile
FROM alpine:3.19
RUN echo "Hello from a Docker image" > /message.txt
CMD ["cat", "/message.txt"]
```

Build it:
```bash
docker build -t my-hello-image .
```

Run it:
```bash
docker run --rm my-hello-image
```

**Do it now:**
- Confirm the output is printed.
- Run `docker images` and make sure `my-hello-image` appears.

---

## 4) Containers vs Images (Understand This Clearly)

- **Image:** Static snapshot. Doesn’t change.
- **Container:** Running instance. Has a writable layer on top of the image.

If you delete a container, its writable changes are gone unless you used a **volume**.

**Do it now:**
1. Run a container in interactive mode:
   ```bash
   docker run -it --name temp-alpine alpine:3.19 sh
   ```
2. Inside it, create a file:
   ```sh
   echo "inside container" > /tmp/inside.txt
   exit
   ```
3. Start a *new* container and check if the file exists:
   ```bash
   docker run --rm alpine:3.19 sh -c "ls /tmp/inside.txt"
   ```
You should see **“No such file or directory”**—because the file belonged to the old container.

---

## 5) The Docker Workflow: From Image Creation to Container Execution

### Standard workflow
1. **Write a Dockerfile** (blueprint).
2. **Build the image** (`docker build`).
3. **Run the container** (`docker run`).
4. **Iterate** by editing source, rebuilding, re-running.
5. **Push image** to registry if you need to share/deploy.

**Do it now:**
1. Create a simple app folder:
   ```bash
   mkdir docker-demo
   cd docker-demo
   ```
2. Add `app.sh`:
   ```bash
   echo "echo Hello from the container" > app.sh
   ```
3. Add a Dockerfile:
   ```Dockerfile
   FROM alpine:3.19
   COPY app.sh /app.sh
   RUN chmod +x /app.sh
   CMD ["/app.sh"]
   ```
4. Build and run:
   ```bash
   docker build -t demo-app .
   docker run --rm demo-app
   ```

---

## 6) Practical Example: A Tiny Web Server

**Do it now:**
1. Run Nginx container:
   ```bash
   docker run --rm -d -p 8080:80 --name web nginx:alpine
   ```
2. Open http://localhost:8080 in your browser.
3. Stop it:
   ```bash
   docker stop web
   ```

**What you learned:**
- `-p 8080:80` maps **host:container** ports.
- Containers run in isolated networks by default.

---

## 7) Networking-Related Proof-of-Concept Workflow (Git + Docker)

You requested a **real workflow** to show networking tasks using Git while Docker is part of the setup. Here is a minimal, repeatable PoC.

### Scenario
You have a project in a Git repo. You want a clean, reproducible environment to: **fetch updates**, **pull code**, **push code**—all inside Docker so your host machine stays clean.

### PoC Plan (Hands-On)
**Step 1 — Create a working directory**
```bash
mkdir docker-git-poc
cd docker-git-poc
```

**Step 2 — Create a Dockerfile**
```Dockerfile
FROM alpine:3.19
RUN apk add --no-cache git openssh
WORKDIR /workspace
CMD ["sh"]
```

**Step 3 — Build the image**
```bash
docker build -t git-poc .
```

**Step 4 — Run an interactive container**
```bash
docker run -it --rm \
  -v ${PWD}:/workspace \
  git-poc
```

> This mounts your local folder into the container so you can use it as a workspace.

### Inside the container: Git networking tasks
> You must have a remote Git repo URL for these steps.

1. **Clone (first time) or fetch updates:**
   ```sh
   git clone <YOUR_REPO_URL>
   cd <YOUR_REPO_NAME>
   ```

2. **Fetch updates (network-only):**
   ```sh
   git fetch
   ```

3. **Pull updates (fetch + merge):**
   ```sh
   git pull
   ```

4. **Make a test change**
   ```sh
   echo "poc change" >> POC.txt
   git add POC.txt
   git commit -m "PoC: change from Docker container"
   ```

5. **Push updates:**
   ```sh
   git push
   ```

**Proof-of-Concept success criteria:**
- `git fetch` shows remote communication.
- `git pull` updates local branch.
- `git push` sends your commit to remote.

---

## 8) Your MyGit Project: Planning Pull, Push, and Fetch

### Current State of Your MyGit Implementation
Your project at `d:\Git\` is a custom Git implementation with these commands already working:
- ✅ `mygit init` — repository initialization
- ✅ `mygit add` — staging files
- ✅ `mygit status` — viewing staging area
- ✅ `mygit ls-files` — listing tracked files
- ✅ `mygit cat-file` — inspecting objects
- ✅ `mygit rm` — removing files

### Building Networking Commands: Pull, Push, Fetch

To add networking capabilities to MyGit, you'll need to implement three core commands that communicate with remote repositories.

#### **Step 1: Understanding the Architecture**

Each command has a specific role in the Git workflow:

| Command | What It Does | Network Activity | Local Changes |
|---------|--------------|------------------|---------------|
| **fetch** | Downloads objects and refs from remote | ✅ Yes | Updates `.mygit/refs/remotes/` only |
| **pull** | Fetch + merge into current branch | ✅ Yes | Updates working directory |
| **push** | Uploads local commits to remote | ✅ Yes | None (changes remote) |

#### **Step 2: Implementation Roadmap**

**Phase 1: Add Remote Management**
```cpp
// src/commands/remote.h and remote.cpp
class Remote {
public:
    bool execute(const std::string& action, const std::string& name, const std::string& url);
private:
    bool addRemote(const std::string& name, const std::string& url);
    bool listRemotes();
};
```

**What to build:**
- Store remote URLs in `.mygit/config`
- Command: `mygit remote add origin <url>`

**Phase 2: Implement Fetch**
```cpp
// src/commands/fetch.h and fetch.cpp
class Fetch {
public:
    bool execute(const std::string& remoteName);
private:
    bool connectToRemote(const std::string& url);
    bool downloadObjects();
    bool updateRemoteRefs();
};
```

**What to build:**
1. Parse remote URL from `.mygit/config`
2. Establish HTTP/SSH connection to remote
3. Request missing objects (use Git's smart protocol or dumb HTTP)
4. Download pack files or loose objects
5. Update `.mygit/refs/remotes/origin/main`

**Phase 3: Implement Pull**
```cpp
// src/commands/pull.h and pull.cpp
class Pull {
public:
    bool execute(const std::string& remoteName, const std::string& branch);
private:
    bool fetch(const std::string& remoteName);
    bool merge(const std::string& remoteBranch);
};
```

**What to build:**
1. Call your `Fetch` implementation
2. Implement merge logic (fast-forward or 3-way merge)
3. Update working directory and HEAD

**Phase 4: Implement Push**
```cpp
// src/commands/push.h and push.cpp
class Push {
public:
    bool execute(const std::string& remoteName, const std::string& branch);
private:
    bool uploadObjects();
    bool updateRemoteRefs();
};
```

**What to build:**
1. Find commits that remote doesn't have
2. Pack objects into a pack file
3. Upload to remote via HTTP POST or SSH
4. Update remote refs

#### **Step 3: Docker Integration for Testing**

**Why Docker is perfect for testing networking commands:**
- Spin up a local Git server in seconds
- Test without GitHub/GitLab
- Isolated environment for experimentation

**Create a test environment:**

1. **Dockerfile for Git server:**
```Dockerfile
FROM alpine:3.19
RUN apk add --no-cache git git-daemon openssh
RUN git config --global init.defaultBranch main

# Create a bare repo for testing
RUN mkdir -p /git-repos && \
    cd /git-repos && \
    git init --bare test-repo.git

EXPOSE 9418
CMD ["git", "daemon", "--reuseaddr", "--base-path=/git-repos", "--export-all", "/git-repos"]
```

2. **Build and run the Git server:**
```bash
cd d:\Git
docker build -t mygit-test-server -f Dockerfile.gitserver .
docker run -d -p 9418:9418 --name gitserver mygit-test-server
```

3. **Test your MyGit networking commands:**
```bash
# In your project directory
.\mygit.exe remote add origin git://localhost:9418/test-repo.git
.\mygit.exe fetch origin
.\mygit.exe pull origin main
.\mygit.exe push origin main
```

#### **Step 4: Networking Libraries You'll Need**

For C++ networking in your MyGit implementation:

**Option A: Use libcurl (Recommended)**
```cpp
#include <curl/curl.h>

// HTTP transport for Git smart protocol
bool Fetch::downloadFromHttp(const std::string& url) {
    CURL* curl = curl_easy_init();
    // Set up GET request to <url>/info/refs?service=git-upload-pack
    // Parse response and download pack file
}
```

**Option B: Use libssh2 (For SSH transport)**
```cpp
#include <libssh2.h>

// SSH transport
bool Fetch::downloadFromSsh(const std::string& url) {
    // Connect via SSH
    // Execute git-upload-pack command
}
```

**Add to CMakeLists.txt:**
```cmake
find_package(CURL REQUIRED)
target_link_libraries(mygit PRIVATE CURL::libcurl)
```

#### **Step 5: Proof-of-Concept Workflow**

**Complete workflow demonstration:**

```bash
# 1. Initialize a test repo
.\mygit.exe init

# 2. Add and commit a file
echo "test content" > test.txt
.\mygit.exe add test.txt
.\mygit.exe commit -m "Initial commit"  # (You'll need to implement commit first)

# 3. Set up remote
.\mygit.exe remote add origin git://localhost:9418/test-repo.git

# 4. Push to remote
.\mygit.exe push origin main

# 5. Make a change remotely (simulate another developer)
# ... (use Docker container to modify remote repo)

# 6. Fetch updates
.\mygit.exe fetch origin

# 7. Pull and merge
.\mygit.exe pull origin main
```

#### **Step 6: Implementation Order (Recommended)**

1. **Week 1:** Implement `mygit commit` (prerequisite for push/pull)
2. **Week 2:** Implement `mygit remote add/list`
3. **Week 3:** Implement basic `mygit fetch` (read-only, HTTP protocol)
4. **Week 4:** Implement `mygit pull` (fetch + merge)
5. **Week 5:** Implement `mygit push` (write operation)
6. **Week 6:** Add Docker-based integration tests

#### **Key Files to Create**

```
src/
├── commands/
│   ├── commit.h/cpp       # Create commits (needed before push/pull)
│   ├── remote.h/cpp       # Manage remotes
│   ├── fetch.h/cpp        # Download from remote
│   ├── pull.h/cpp         # Fetch + merge
│   └── push.h/cpp         # Upload to remote
├── network/
│   ├── http_transport.h/cpp   # HTTP protocol
│   ├── ssh_transport.h/cpp    # SSH protocol
│   └── protocol.h/cpp         # Git wire protocol
└── utils/
    ├── packfile.h/cpp     # Pack/unpack objects
    └── merge.h/cpp        # Merge algorithms
```

---

## 9) Key Mental Model (Summarize This)

1. **Images are blueprints** (read-only, layered, shareable).
2. **Containers are processes** (running instances of images).
3. **Dockerfile → Image → Container** is the core flow.
4. **Volumes** preserve data outside containers.
5. **Networks** connect containers and the host.

---

## 9) Quick Commands Cheat Sheet

```bash
# images & containers
docker images
docker ps
docker ps -a

# build & run
docker build -t <name> .
docker run --rm <name>

# interactive shell
docker run -it --rm alpine:3.19 sh

# networking
docker run -d -p 8080:80 nginx:alpine

# cleanup
docker rm <container>
docker rmi <image>
```

---

## Next Step (Do It Now)
- Pick one of the hands-on sections and rerun it **without copying** the commands. If you can type it from memory, you’re starting to understand Docker.
