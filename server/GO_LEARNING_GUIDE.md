# MyGit Server - Go Learning Guide

Welcome! This guide will teach you Go while building a real Git server.

## Table of Contents
1. [Why Go?](#why-go)
2. [Installing Go](#installing-go)
3. [Your First Steps](#your-first-steps)
4. [Understanding the Server Code](#understanding-the-server-code)
5. [Go Concepts Cheat Sheet](#go-concepts-cheat-sheet)
6. [Running the Server](#running-the-server)
7. [Deploying to the Cloud](#deploying-to-the-cloud)

---

## Why Go?

Go was created by Google to solve real-world problems:

| Problem | Go's Solution |
|---------|---------------|
| C++ compiles slowly | Go compiles in seconds |
| Python is slow at runtime | Go is fast (compiled to native code) |
| JavaScript has no types | Go has static typing |
| Java is verbose | Go is minimal and readable |
| Deployment is complex | Go compiles to a single binary |

**For your Git server, Go is perfect because:**
- Built-in HTTP server (no external dependencies!)
- Single binary deployment (no runtime needed)
- Cross-compiles to Linux easily from Windows
- Handles concurrent connections efficiently

---

## Installing Go

### Windows
1. Download from https://go.dev/dl/
2. Run the installer
3. Open a new terminal and verify:
   ```powershell
   go version
   ```

### Alternative: Using Scoop
```powershell
scoop install go
```

---

## Your First Steps

### 1. Navigate to the server directory
```powershell
cd d:\Git\server
```

### 2. Run the server (Go compiles and runs automatically!)
```powershell
go run main.go
```

### 3. Build a binary
```powershell
go build -o mygit-server.exe
```

### 4. Cross-compile for Linux (for deployment!)
```powershell
$env:GOOS="linux"; $env:GOARCH="amd64"; go build -o mygit-server
```

---

## Understanding the Server Code

Open `main.go` and read it top to bottom. It's heavily commented to teach you Go!

### Key Concepts (in order of appearance):

#### 1. Packages and Imports
```go
package main    // This is an executable (not a library)

import (
    "fmt"       // Standard library for printing
    "net/http"  // Built-in HTTP server!
)
```

#### 2. Functions
```go
func functionName(param1 string, param2 int) returnType {
    // body
}

// Multiple return values (unique to Go!)
func divide(a, b int) (int, error) {
    if b == 0 {
        return 0, errors.New("division by zero")
    }
    return a / b, nil
}
```

#### 3. Variables
```go
// Full declaration
var name string = "hello"

// Short declaration (preferred)
name := "hello"

// Constants
const maxSize = 100
```

#### 4. Error Handling
```go
// Go doesn't have exceptions! Errors are values.
result, err := someFunction()
if err != nil {
    // Handle error
    return err
}
// Use result
```

#### 5. Maps (Dictionaries)
```go
// Create a map
refs := make(map[string]string)

// Add items
refs["main"] = "abc123"

// Get items
hash := refs["main"]

// Check if key exists
hash, exists := refs["main"]
if exists {
    // key was found
}
```

#### 6. Slices (Dynamic Arrays)
```go
// Create a slice
numbers := []int{1, 2, 3}

// Append
numbers = append(numbers, 4)

// Slice syntax
first := numbers[0]      // First element
last := numbers[len(numbers)-1]  // Last element
subset := numbers[1:3]   // Elements 1 and 2
```

---

## Go Concepts Cheat Sheet

### Coming from C++

| C++ | Go |
|-----|-----|
| `#include <iostream>` | `import "fmt"` |
| `std::cout <<` | `fmt.Println()` |
| `int main()` | `func main()` |
| `new Object()` | `&Object{}` or `new(Object)` |
| `nullptr` | `nil` |
| `class` | `struct` + methods |
| `this->` | No `this`, use receiver name |
| `try/catch` | Return `error`, check `if err != nil` |
| `vector<int>` | `[]int` (slice) |
| `map<string,int>` | `map[string]int` |
| `auto` | `:=` (type inference) |

### Unique Go Features

```go
// Multiple return values
func swap(a, b int) (int, int) {
    return b, a
}

// Named return values
func split(sum int) (x, y int) {
    x = sum * 4 / 9
    y = sum - x
    return  // Returns x and y
}

// Defer (runs when function exits)
func readFile(path string) {
    file, _ := os.Open(path)
    defer file.Close()  // Guaranteed to run!
    // ... use file
}

// Goroutines (lightweight threads)
go func() {
    // This runs concurrently
}()

// Channels (communication between goroutines)
ch := make(chan int)
go func() { ch <- 42 }()  // Send
value := <-ch              // Receive
```

---

## Running the Server

### Development
```powershell
cd d:\Git\server
go run main.go
```

### Production Build
```powershell
go build -o mygit-server.exe
./mygit-server.exe
```

### Testing the Endpoints
```powershell
# Health check
curl http://localhost:8080/health

# List refs
curl http://localhost:8080/refs

# Get specific ref
curl http://localhost:8080/ref/main
```

---

## Deploying to the Cloud

### Option 1: Railway (Easiest)
1. Push your `server/` folder to GitHub
2. Go to https://railway.app
3. Connect your repo
4. Railway auto-detects Go and deploys!

### Option 2: Fly.io
```powershell
# Install flyctl
# Then:
cd d:\Git\server
fly launch
fly deploy
```

### Option 3: Docker + Any Cloud
Create a Dockerfile in `server/`:
```dockerfile
FROM golang:1.21-alpine AS builder
WORKDIR /app
COPY . .
RUN go build -o mygit-server

FROM alpine:latest
WORKDIR /app
COPY --from=builder /app/mygit-server .
EXPOSE 8080
CMD ["./mygit-server"]
```

Build and run:
```powershell
docker build -t mygit-server .
docker run -p 8080:8080 mygit-server
```

---

## Next Steps

1. **Read `main.go`** - Every line is commented to teach you
2. **Modify something** - Add a new endpoint
3. **Run and test** - Use `go run main.go` and curl
4. **Build and deploy** - Cross-compile for Linux and deploy

### Recommended Learning Path
1. Complete the [Go Tour](https://go.dev/tour/) (official, interactive)
2. Read [Go by Example](https://gobyexample.com/) (practical examples)
3. Build something! (You're already doing this!)

---

## Quick Reference

### Common Commands
```powershell
go run main.go      # Compile and run
go build            # Build binary
go fmt              # Format code (do this often!)
go mod tidy         # Clean up dependencies
go test             # Run tests
```

### Keyboard Shortcuts (if using VS Code)
- Install the Go extension
- `Ctrl+Shift+P` → "Go: Install/Update Tools"
- Format on save is automatic!

---

Good luck on your Go journey! The best way to learn is by doing, and you're already building a real server!
