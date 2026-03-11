# MyGit Server - Quick Start Guide

## 🚀 Quick Start (5 Minutes)

### Step 1: Build the Project

```bash
cd D:\Git
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

This creates:
- `mygit.exe` - Git client
- `mygit-server.exe` - Git server

### Step 2: Start the Server

Open a **new terminal** and run:

```bash
# Create server repository directory
mkdir D:\GitServer
cd D:\GitServer

# Initialize repository
D:\Git\build\Release\mygit.exe init

# Start server on port 8080
D:\Git\build\Release\mygit-server.exe
```

You should see:
```
MyGit Server
============
Starting server on port 8080...
Git server started on port 8080
Waiting for connections...
```

**Keep this terminal open!**

### Step 3: Set Up Client Repository

Open a **different terminal**:

```bash
# Create your project directory
mkdir D:\MyProject
cd D:\MyProject

# Initialize repository
D:\Git\build\Release\mygit.exe init

# Add remote server (using localhost for testing)
D:\Git\build\Release\mygit.exe remote add origin http://localhost:8080

# Verify remote was added
D:\Git\build\Release\mygit.exe remote -v
```

### Step 4: Make Some Commits

```bash
# Create a file
echo "Hello World" > README.md

# Stage it
D:\Git\build\Release\mygit.exe add README.md

# Commit it
D:\Git\build\Release\mygit.exe commit -m "Initial commit"

# Check status
D:\Git\build\Release\mygit.exe log
```

### Step 5: Push to Server

```bash
D:\Git\build\Release\mygit.exe push origin main
```

Output:
```
Pushing main to origin...
Found 3 objects to push
Pushing objects: 3/3
Successfully pushed main -> origin/main
```

### Step 6: Clone/Pull from Another Location

Open a **third terminal**:

```bash
# Create clone directory
mkdir D:\MyProjectClone
cd D:\MyProjectClone

# Initialize and configure
D:\Git\build\Release\mygit.exe init
D:\Git\build\Release\mygit.exe remote add origin http://localhost:8080

# Pull from server
D:\Git\build\Release\mygit.exe pull origin main

# Verify files
dir
type README.md
```

## 🎯 Common Commands

### Remote Management
```bash
# Add a remote
mygit remote add <name> <url>

# List remotes
mygit remote -v

# Remove a remote
mygit remote remove <name>
```

### Push/Pull
```bash
# Push current branch
mygit push origin main

# Pull current branch
mygit pull origin main

# Push specific branch
mygit push origin feature-branch
```

## 🌐 Network Setup (Connect from Other Machines)

### Find Your IP Address
```powershell
ipconfig
# Look for IPv4 Address, e.g., 192.168.1.100
```

### Allow Firewall Access
```powershell
netsh advfirewall firewall add rule name="MyGit Server" dir=in action=allow protocol=TCP localport=8080
```

### Connect from Another Computer
```bash
# On another machine on the same network
mygit remote add origin http://192.168.1.100:8080
mygit push origin main
```

## 📁 Recommended Directory Structure

```
D:\GitServer\           # Server repository (where server runs)
  ├── .mygit\
  └── mygit-server.exe

D:\MyProject\           # Your project
  ├── .mygit\
  └── your-files...

D:\OtherClone\          # Another clone
  ├── .mygit\
  └── your-files...
```

## 🔧 Troubleshooting

### "Bind failed" Error
**Problem:** Port 8080 already in use

**Solution:**
```bash
# Find what's using port 8080
netstat -ano | findstr :8080

# Kill that process or use a different port
mygit-server.exe 9000
```

### "Remote 'origin' not found"
**Problem:** Remote not configured

**Solution:**
```bash
mygit remote add origin http://localhost:8080
```

### Can't Connect from Another Machine
**Problem:** Firewall blocking

**Solution:**
1. Check Windows Firewall settings
2. Verify server is running
3. Test with `curl http://SERVER_IP:8080/refs`

### Push/Pull Shows No Objects
**Problem:** No commits made yet

**Solution:**
```bash
# Make sure you have committed something
mygit add .
mygit commit -m "First commit"
mygit push origin main
```

## 📚 Next Steps

- Read [SERVER_DEPLOYMENT.md](SERVER_DEPLOYMENT.md) for production deployment
- Set up continuous integration
- Add webhooks for automated deployments
- Implement authentication for security

## 🔐 Security Warning

**⚠️ Current version is for development/testing only!**

For production use, you need:
- HTTPS/TLS encryption
- User authentication
- Access control
- Rate limiting

See [SERVER_DEPLOYMENT.md](SERVER_DEPLOYMENT.md) for production setup.

## 💡 Tips

1. **Add mygit to PATH** for easier access:
   ```powershell
   $env:PATH += ";D:\Git\build\Release"
   ```

2. **Create aliases** (PowerShell):
   ```powershell
   Set-Alias mg mygit.exe
   Set-Alias mgs mygit-server.exe
   ```

3. **Run server as background service** (production):
   - Use NSSM (Non-Sucking Service Manager)
   - Or Task Scheduler on Windows

4. **Multiple servers** for multiple projects:
   ```bash
   # Terminal 1
   cd D:\ProjectA && mygit-server 8081
   
   # Terminal 2
   cd D:\ProjectB && mygit-server 8082
   ```

## 🎓 Example Workflow

```bash
# DAY 1: Set up and push your work
mygit init
mygit remote add origin http://server:8080
echo "Code here" > app.cpp
mygit add app.cpp
mygit commit -m "Initial version"
mygit push origin main

# DAY 2: Continue from another machine
cd D:\Work
mygit init
mygit remote add origin http://server:8080
mygit pull origin main
# Edit files...
mygit add .
mygit commit -m "Added features"
mygit push origin main

# DAY 3: Back to first machine
mygit pull origin main
# Pulls the changes from DAY 2
```

---

**Need Help?** Check [SERVER_DEPLOYMENT.md](SERVER_DEPLOYMENT.md) for detailed documentation.
