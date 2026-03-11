# MyGit - A Simple Git Implementation in C++

## Current Directory Structure

```
D:\Git\
├── .mygit/                     # MyGit repository metadata (test repo)
│   ├── objects/                # Object database
│   │   ├── 43/                 # First 2 chars of hash
│   │   │   └── 06b8f99...      # Remaining 38 chars (blob object)
│   │   └── ef/
│   │       └── e6aca67...      # Another blob object
│   ├── refs/                   # References (branches)
│   │   └── heads/              # Branch heads
│   ├── index                   # Staging area
│   ├── HEAD                    # Current branch pointer
│   └── config                  # Repository configuration
│
├── src/                        # Source code
│   ├── commands/               # Command implementations
│   │   ├── init.h              # Init command header
│   │   ├── init.cpp            # Create .mygit structure
│   │   ├── add.h               # Add command header
│   │   ├── add.cpp             # Stage files (hash & store)
│   │   ├── status.h            # Status command header
│   │   └── status.cpp          # Show working tree status
│   ├── utils/                  # Utility functions
│   │   ├── hash.h              # SHA-1 hashing header
│   │   └── hash.cpp            # SHA-1 implementation (Windows CryptoAPI)
│   └── main.cpp                # Entry point (command dispatcher)
│
├── CMakeLists.txt              # Build configuration
├── mygit.exe                   # Compiled executable
└── test.txt, file2.txt, ...    # Test files

```

## Implemented Commands

### 1. `mygit init`
- Creates `.mygit/` directory structure
- Initializes HEAD, config, refs, and objects directories

### 2. `mygit add <file>`
- Computes SHA-1 hash of file content
- Stores blob object in `.mygit/objects/<2-char>/<38-char>`
- Updates index (staging area)

### 3. `mygit status`
- Shows staged files (in index)
- Shows modified files (staged but changed)
- Shows untracked files (not in index)

---

## How to Implement `mygit commit -m "message"`

### Theory Behind Git Commit

A **commit** in Git is a snapshot of your project at a specific point in time. It contains:

1. **Tree object** - Directory structure and file references
2. **Commit object** - Metadata (author, timestamp, message, parent commit)

### Git Object Model

```
Commit Object
├── tree <tree-hash>           # Points to root tree
├── parent <parent-hash>       # Previous commit (if not first)
├── author <name> <timestamp>
├── committer <name> <timestamp>
└── message

Tree Object
├── blob <hash> <filename>     # Files in this directory
├── tree <hash> <dirname>      # Subdirectories
└── ...

Blob Object
└── <file-content>             # Actual file data
```

### Implementation Steps

#### Step 1: Create Tree Object
A tree object represents a directory snapshot. Format:
```
tree <size>\0
<mode> <filename>\0<hash-binary>
<mode> <filename>\0<hash-binary>
...
```

**Tasks:**
1. Read index file
2. For each staged file, create entry: `100644 <filename>\0<20-byte-binary-hash>`
3. Compute SHA-1 of entire tree content
4. Store tree object in `.mygit/objects/`

#### Step 2: Create Commit Object
A commit object contains metadata and points to the tree. Format:
```
commit <size>\0
tree <tree-hash>
parent <parent-commit-hash>    # (skip if first commit)
author <name> <email> <timestamp> <timezone>
committer <name> <email> <timestamp> <timezone>

<commit-message>
```

**Tasks:**
1. Get tree hash from Step 1
2. Read HEAD to find parent commit (if exists)
3. Get author info (can hardcode or use env variables)
4. Get timestamp (Unix epoch)
5. Construct commit content
6. Compute SHA-1 hash
7. Store commit object in `.mygit/objects/`

#### Step 3: Update References
Update branch pointer to new commit:
1. Read HEAD to find current branch (e.g., `ref: refs/heads/master`)
2. Write commit hash to `.mygit/refs/heads/master`

#### Step 4: Clear Index (Optional)
After commit, the staging area should remain (git keeps it), but you could optionally clear it.

### Implementation Files to Create

```cpp
// src/commands/commit.h
class Commit {
public:
    bool execute(const std::string& message);
private:
    std::string createTreeObject(const std::map<std::string, std::string>& indexFiles);
    std::string createCommitObject(const std::string& treeHash, const std::string& message);
    bool updateBranch(const std::string& commitHash);
    std::string getParentCommit();
};

// src/commands/commit.cpp
// - Read index
// - Create tree object
// - Create commit object
// - Update branch ref
```

### Helper Functions Needed

```cpp
// In utils/hash.h
static std::string treeHash(const std::string& treeContent);
static std::string commitHash(const std::string& commitContent);

// In utils/object.h (new file)
class Object {
public:
    static bool storeObject(const std::string& type, 
                           const std::string& content, 
                           std::string& outHash);
    static std::string readObject(const std::string& hash);
};
```

### Example Flow

```bash
# User has staged files
$ mygit status
Changes to be committed:
    new file:   file1.txt
    new file:   file2.txt

# User commits
$ mygit commit -m "Initial commit"

# What happens:
# 1. Read index: file1.txt (hash: abc123...), file2.txt (hash: def456...)
# 2. Create tree:
#    "tree 68\0100644 file1.txt\0<20-bytes>100644 file2.txt\0<20-bytes>"
#    Tree hash: 789abc...
# 3. Create commit:
#    "commit 180\0tree 789abc...\nauthor User <email> 1706745600 +0000\n..."
#    Commit hash: fedcba...
# 4. Update .mygit/refs/heads/master: fedcba...
# 5. Output: [master fedcba] Initial commit
```

### Data Structures

```cpp
// Tree entry
struct TreeEntry {
    std::string mode;      // "100644" for files
    std::string name;      // filename
    std::string hash;      // SHA-1 hash (40 hex chars)
};

// Commit info
struct CommitInfo {
    std::string tree;      // Tree object hash
    std::string parent;    // Parent commit hash (empty if first)
    std::string author;    // "Name <email> timestamp +0000"
    std::string message;   // Commit message
};
```

### Testing Plan

```bash
# Initialize repo
$ mygit init

# Add files
$ echo "Hello" > file1.txt
$ mygit add file1.txt

# Commit
$ mygit commit -m "First commit"
# Expected output: [master abc1234] First commit

# Verify
$ cat .mygit/refs/heads/master
# Should show commit hash

$ cat .mygit/HEAD
# Should show: ref: refs/heads/master

# Check objects created
$ ls .mygit/objects/
# Should see both tree and commit objects
```

### Next Steps After Commit

Once commit is implemented, you can add:
1. **`mygit log`** - Display commit history
2. **`mygit diff`** - Show changes between commits/working tree
3. **`mygit checkout`** - Switch branches or restore files
4. **`mygit branch`** - Create/list branches

---

## Building the Project

```bash
# Using g++ directly
g++ -std=c++17 src/main.cpp src/commands/*.cpp src/utils/*.cpp -o mygit.exe -I src -lcrypt32

# Or using CMake
mkdir build
cd build
cmake ..
cmake --build .
```

## Key Design Decisions

1. **SHA-1 Hashing**: Using Windows CryptoAPI (wincrypt.h) - platform-specific but simple
2. **Object Storage**: Uncompressed for simplicity (real git uses zlib)
3. **Index Format**: Simple text format (real git uses binary)
4. **No Compression**: Storing objects as plain text with headers

## Dependencies

- **C++17**: For `std::filesystem`
- **Windows CryptoAPI**: For SHA-1 hashing (crypt32.lib)
- **Standard Library**: fstream, iostream, sstream, map, vector

---

## Future Features Implementation Guide

### 4. `mygit log` - Display Commit History

#### Theory
Git log traverses the commit history by following parent pointers from HEAD backwards. Each commit points to its parent(s), forming a directed acyclic graph (DAG).

#### What It Does
- Reads current branch ref to get latest commit hash
- Reads commit object to get tree, parent, author, and message
- Follows parent chain recursively
- Displays commits in reverse chronological order

#### Implementation Steps

1. **Read HEAD** → Get current branch → Read branch ref → Get commit hash
2. **Parse commit object**:
   ```
   commit <size>\0
   tree <hash>
   parent <hash>      # May not exist (first commit)
   author ...
   committer ...
   
   <message>
   ```
3. **Extract fields**: tree, parent, author, timestamp, message
4. **Display**: `[<hash>] <message>` or detailed format
5. **Follow parent**: Repeat for parent commit until no parent exists

#### Files to Create
```cpp
// src/commands/log.h
class Log {
public:
    bool execute(int limit = -1);  // -1 = unlimited
private:
    struct CommitData {
        std::string hash;
        std::string tree;
        std::string parent;
        std::string author;
        std::string timestamp;
        std::string message;
    };
    CommitData parseCommit(const std::string& hash);
    std::string readObject(const std::string& hash);
};
```

#### Output Format
```bash
$ mygit log
commit fedcba9876543210 
Author: User <user@email.com>
Date: Fri Jan 31 22:30:00 2026

    Second commit

commit abc1234567890def
Author: User <user@email.com>
Date: Fri Jan 31 22:00:00 2026

    Initial commit
```

---

### 5. `mygit diff` - Show Changes

#### Theory
Diff compares file content byte-by-byte and shows additions/deletions. Can compare:
- Working directory vs staged (index)
- Staged vs last commit
- Two commits

#### What It Does
- Compares file hashes between two states
- For changed files, performs line-by-line comparison
- Displays unified diff format: `+` for additions, `-` for deletions

#### Implementation Steps

1. **Diff working vs staged** (`mygit diff`):
   - Read index to get staged file hashes
   - Compute current file hashes
   - Compare and show differences

2. **Diff staged vs HEAD** (`mygit diff --cached`):
   - Read HEAD commit tree
   - Compare with index

3. **Line-by-line diff algorithm**:
   - Split files into lines
   - Use longest common subsequence (LCS) algorithm
   - Mark additions (+) and deletions (-)

#### Files to Create
```cpp
// src/utils/diff.h
class Diff {
public:
    struct Change {
        int lineNum;
        char type;  // '+' or '-'
        std::string content;
    };
    
    static std::vector<Change> compare(
        const std::vector<std::string>& oldLines,
        const std::vector<std::string>& newLines
    );
    
    static void printUnifiedDiff(
        const std::string& filename,
        const std::vector<Change>& changes
    );
};
```

#### Output Format
```diff
diff --git a/file.txt b/file.txt
--- a/file.txt
+++ b/file.txt
@@ -1,3 +1,4 @@
 Line 1
-Line 2
+Line 2 modified
+Line 3 added
 Line 4
```

---

### 6. `mygit branch` - Branch Management

#### Theory
Branches are lightweight pointers to commits. They're just files containing a commit hash in `.mygit/refs/heads/`. HEAD points to the current branch.

#### What It Does
- List branches: Read all files in `.mygit/refs/heads/`
- Create branch: Write commit hash to new file
- Delete branch: Remove file from `refs/heads/`
- Show current: Parse HEAD file

#### Implementation Steps

1. **List branches** (`mygit branch`):
   ```cpp
   for (auto& file : fs::directory_iterator(".mygit/refs/heads/")) {
       std::string branchName = file.path().filename();
       if (isCurrentBranch(branchName)) {
           std::cout << "* " << branchName << "\n";
       } else {
           std::cout << "  " << branchName << "\n";
       }
   }
   ```

2. **Create branch** (`mygit branch <name>`):
   - Get current HEAD commit hash
   - Write hash to `.mygit/refs/heads/<name>`

3. **Delete branch** (`mygit branch -d <name>`):
   - Check it's not current branch
   - Remove `.mygit/refs/heads/<name>`

#### Data Structure
```
.mygit/refs/heads/
├── master     → "abc123def456..."
├── feature    → "789abc012def..."
└── develop    → "fedcba987654..."

.mygit/HEAD    → "ref: refs/heads/master"
```

---

### 7. `mygit checkout` - Switch Branches/Restore Files

#### Theory
Checkout does two things:
1. **Switch branch**: Update HEAD and restore working tree
2. **Restore file**: Copy from commit/index to working directory

This is Git's most complex command - it modifies HEAD, index, and working directory.

#### What It Does
- Read commit's tree object
- Extract all blob references
- Write blobs to working directory
- Update HEAD pointer
- Update index to match commit

#### Implementation Steps

1. **Checkout branch** (`mygit checkout <branch>`):
   ```
   a. Check if branch exists in refs/heads/
   b. Get commit hash from branch ref
   c. Read commit → get tree hash
   d. Read tree → get all blobs
   e. Write blobs to working directory
   f. Update HEAD: "ref: refs/heads/<branch>"
   g. Update index to match tree
   ```

2. **Parse tree object**:
   ```
   tree <size>\0
   100644 file1.txt\0<20-bytes-hash>
   100644 file2.txt\0<20-bytes-hash>
   ```
   - Binary format: mode (text), space, name (text), null, hash (20 raw bytes)

3. **Restore files**:
   - For each tree entry: read blob, write to working directory

#### Files to Create
```cpp
// src/commands/checkout.h
class Checkout {
public:
    bool execute(const std::string& target);
private:
    bool checkoutBranch(const std::string& branch);
    bool checkoutFile(const std::string& file);
    std::map<std::string, std::string> parseTree(const std::string& treeHash);
    bool restoreFiles(const std::map<std::string, std::string>& files);
};
```

---

### 8. `mygit reset` - Undo Changes

#### Theory
Reset moves the current branch pointer and optionally modifies index/working directory. Three modes:
- **--soft**: Move HEAD only
- **--mixed** (default): Move HEAD + update index
- **--hard**: Move HEAD + update index + update working directory

#### What It Does
- Moves branch pointer to specified commit
- Optionally resets index
- Optionally resets working directory

#### Implementation Steps

1. **Soft reset** (`mygit reset --soft <commit>`):
   - Update `.mygit/refs/heads/<current-branch>` with commit hash

2. **Mixed reset** (`mygit reset <commit>`):
   - Update branch ref
   - Read commit's tree
   - Rewrite index to match tree

3. **Hard reset** (`mygit reset --hard <commit>`):
   - Update branch ref
   - Update index
   - Restore working directory from commit

#### Safety Considerations
- Warn before hard reset (data loss)
- Keep copy of old HEAD in `.mygit/ORIG_HEAD`

---

### 9. `mygit merge` - Merge Branches

#### Theory
Merging combines changes from two branches. Types:
- **Fast-forward**: Current branch is ancestor of target, just move pointer
- **Three-way merge**: Find common ancestor, merge changes
- **Conflict**: Both branches modified same lines

#### What It Does
1. Find merge base (common ancestor commit)
2. Compare changes in both branches
3. Combine changes or report conflicts

#### Implementation Steps (Simplified)

1. **Fast-forward merge**:
   ```cpp
   if (isAncestor(currentCommit, targetCommit)) {
       // Just update branch pointer
       updateBranch(targetCommit);
   }
   ```

2. **Three-way merge**:
   ```
   a. Find common ancestor using graph traversal
   b. Get three trees: base, ours, theirs
   c. For each file:
      - If only changed in one branch: use that version
      - If changed in both: detect conflict
   d. Create merge commit with two parents
   ```

3. **Conflict handling**:
   ```
   <<<<<<< HEAD
   Our changes
   =======
   Their changes
   >>>>>>> feature-branch
   ```

---

### 10. `mygit clone` - Clone Repository

#### Theory
Clone copies entire repository to new location. For local clone:
- Copy `.mygit/` directory
- Setup remote tracking
- Checkout working directory

#### Implementation Steps

1. Create destination directory
2. Copy `.mygit/` recursively
3. Checkout HEAD to populate working directory
4. Setup `.mygit/config` with remote info

---

### 11. `mygit tag` - Create Tags

#### Theory
Tags are permanent pointers to commits (unlike branches which move). Two types:
- **Lightweight**: Just commit hash in `.mygit/refs/tags/<name>`
- **Annotated**: Tag object with metadata

#### Implementation

**Lightweight tag**:
```cpp
// Write commit hash to .mygit/refs/tags/<tagname>
std::ofstream tag(".mygit/refs/tags/" + tagName);
tag << getCurrentCommit();
```

**Annotated tag object**:
```
tag <size>\0
object <commit-hash>
type commit
tag <tag-name>
tagger <name> <email> <timestamp>

<tag-message>
```

---

## Advanced Features

### 12. `mygit stash` - Temporarily Save Changes

- Save working directory changes to a special commit
- Store in `.mygit/refs/stash`
- Restore later with `mygit stash pop`

### 13. `mygit rebase` - Rewrite History

- Move commits to new base
- Replay commits one by one
- More complex than merge

### 14. `mygit remote` - Remote Repository Management

- Store remote URLs in config
- Track remote branches
- Prepare for push/pull

### 15. `mygit push/pull` - Network Operations

- Serialize objects
- Transfer over HTTP/SSH
- Update remote refs

---

## Object Storage Optimization

### Compression (zlib)

Currently storing objects uncompressed. To optimize:

```cpp
#include <zlib.h>

std::string compress(const std::string& data) {
    uLongf compressedSize = compressBound(data.size());
    std::vector<Byte> compressed(compressedSize);
    
    compress2(compressed.data(), &compressedSize,
              (const Bytef*)data.c_str(), data.size(),
              Z_DEFAULT_COMPRESSION);
    
    return std::string(compressed.begin(), 
                      compressed.begin() + compressedSize);
}
```

### Pack Files

Git combines many objects into packfiles for efficiency:
- Delta compression between similar objects
- `.mygit/objects/pack/` directory
- Index file for quick lookups

---

## Implementation Priorities

### Phase 1: Core Functionality (Current)
- ✅ init, add, status
- ⏳ commit

### Phase 2: Version Control Basics
- log (view history)
- diff (compare changes)
- checkout (restore files)

### Phase 3: Branching
- branch (create/list/delete)
- checkout (switch branches)
- merge (simple fast-forward)

### Phase 4: History Management
- reset (undo commits)
- tag (mark releases)
- Full merge with conflict detection

### Phase 5: Collaboration
- clone (local)
- remote (add/remove)
- push/pull (network)

### Phase 6: Optimization
- Object compression
- Packfiles
- Delta compression

---

## Testing Strategy

### Unit Tests
```cpp
// Test SHA-1 hashing
assert(Hash::sha1("test") == "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3");

// Test blob creation
std::vector<char> data = {'h', 'e', 'l', 'l', 'o'};
assert(Hash::blobHash(data) == "5d308e1d060b0c387d452cf4747f89ecb134c53");
```

### Integration Tests
```bash
#!/bin/bash
# Test basic workflow
mygit init
echo "test" > file.txt
mygit add file.txt
mygit commit -m "Test"
mygit log | grep "Test"
```

---

## Code Organization Best Practices

### Utility Classes

```cpp
// src/utils/repository.h - Common repo operations
class Repository {
public:
    static std::string findRepoRoot();
    static std::string getCurrentBranch();
    static std::string getCommitHash(const std::string& ref);
};

// src/utils/object.h - Object database operations
class ObjectDB {
public:
    static std::string readObject(const std::string& hash);
    static bool writeObject(const std::string& type, 
                           const std::string& content,
                           std::string& outHash);
    static bool objectExists(const std::string& hash);
};

// src/utils/index.h - Index file operations
class Index {
public:
    struct Entry {
        std::string mode;
        std::string hash;
        std::string path;
    };
    
    static std::vector<Entry> read();
    static bool write(const std::vector<Entry>& entries);
    static bool add(const std::string& path, const std::string& hash);
};
```

---

## References

- [Git Internals - Git Objects](https://git-scm.com/book/en/v2/Git-Internals-Git-Objects)
- [Git Internals - Plumbing and Porcelain](https://git-scm.com/book/en/v2/Git-Internals-Plumbing-and-Porcelain)
- [Git Internals - Transfer Protocols](https://git-scm.com/book/en/v2/Git-Internals-Transfer-Protocols)
- [Building Git - James Coglan](https://shop.jcoglan.com/building-git/)
- [Write yourself a Git - Thibault Polge](https://wyag.thb.lt/)
