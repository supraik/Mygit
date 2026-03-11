# MyGit Server - Deployment Guide

## Overview

This guide covers how to build, deploy, and use the MyGit server for remote repository hosting with push/pull capabilities.

## Architecture

The MyGit server consists of two main components:

1. **mygit-server** - HTTP server that hosts repositories
2. **mygit client** - Extended with push, pull, and remote commands

### Server Features

- HTTP REST API for Git operations
- Supports multiple clients
- Object storage and retrieval
- Reference (branch) management
- Multi-threaded request handling

## Building the Project

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler (Visual Studio 2019+, GCC 8+, Clang 7+)
- Windows OS (for current implementation)

### Build Steps

```bash
# Navigate to project directory
cd D:\Git

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build . --config Release

# Or use Visual Studio
cmake --build . --config Release --target ALL_BUILD
```

This will create two executables:
- `mygit.exe` - Client application
- `mygit-server.exe` - Server application

## Server Deployment

### Local Deployment (Development/Testing)

#### 1. Prepare Server Repository

```bash
# Create a directory for the server repository
mkdir D:\GitServer
cd D:\GitServer

# Initialize a bare repository
D:\Git\build\Release\mygit.exe init
```

#### 2. Start the Server

```bash
# Start on default port (8080)
D:\Git\build\Release\mygit-server.exe

# Or specify a custom port
D:\Git\build\Release\mygit-server.exe 9000
```

The server will display:
```
MyGit Server
============
Starting server on port 8080...
Git server started on port 8080
Waiting for connections...
```

#### 3. Keep Server Running

For production, you should:
- Run as a Windows Service
- Use a process manager (NSSM)
- Set up automatic restart on failure

### Network Deployment

#### Firewall Configuration

Allow incoming connections on your chosen port:

```powershell
# Allow port 8080
netsh advfirewall firewall add rule name="MyGit Server" dir=in action=allow protocol=TCP localport=8080
```

#### Find Your IP Address

```powershell
ipconfig
# Look for IPv4 Address
```

### Cloud Deployment (AWS/Azure/GCP)

#### AWS EC2 Example

1. Launch a Windows Server instance
2. Install build tools and compile the server
3. Configure Security Group to allow inbound traffic on port 8080
4. Run the server:

```bash
mygit-server.exe 8080
```

5. Use Elastic IP for stable connection
6. Server URL: `http://<elastic-ip>:8080`

#### Docker Deployment (Optional)

Create a `Dockerfile` (Linux-based):

```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git

# Copy source
COPY . /app
WORKDIR /app

# Build
RUN mkdir build && cd build && cmake .. && make

# Expose port
EXPOSE 8080

# Run server
CMD ["./build/mygit-server"]
```

Build and run:
```bash
docker build -t mygit-server .
docker run -p 8080:8080 -v /path/to/repo:/app/.mygit mygit-server
```

## Client Configuration

### Adding a Remote

```bash
# Add remote server
mygit remote add origin http://192.168.1.100:8080

# Or for cloud deployment
mygit remote add origin http://ec2-xx-xxx-xxx-xxx.compute.amazonaws.com:8080

# Verify
mygit remote -v
```

### Push to Server

```bash
# Push current branch to origin
mygit push origin main

# Push specific branch
mygit push origin feature-branch
```

Output:
```
Pushing main to origin...
Found 15 objects to push
Pushing objects: 15/15
Successfully pushed main -> origin/main
```

### Pull from Server

```bash
# Pull current branch
mygit pull origin main

# Pull specific branch
mygit pull origin feature-branch
```

Output:
```
Pulling main from origin...
Remote main at <commit-hash>
Fetching 8 objects...
Fetching objects: 8/8
Successfully pulled origin/main -> main
```

## Server Management

### Monitoring

The server logs all requests to stdout:

```
GET /refs
POST /object/a3f8b9c...
POST /ref/main
```

Redirect logs to file:

```bash
mygit-server.exe > server.log 2>&1
```

### Backup

Backup the server's `.mygit` directory regularly:

```bash
# Full backup
xcopy /E /I D:\GitServer\.mygit D:\Backups\mygit-$(date +%Y%m%d)

# Or use robocopy (Windows)
robocopy D:\GitServer\.mygit D:\Backups\mygit-%date% /MIR
```

### Multiple Repositories

To host multiple repositories, run multiple server instances:

```bash
# Terminal 1 - Project A
cd D:\Repos\ProjectA
mygit-server.exe 8081

# Terminal 2 - Project B
cd D:\Repos\ProjectB
mygit-server.exe 8082
```

Configure clients:
```bash
# In Project A clone
mygit remote add origin http://server:8081

# In Project B clone
mygit remote add origin http://server:8082
```

## Production Considerations

### 1. Security

**Current Implementation** (Development Only):
- No authentication
- No encryption (plain HTTP)

**For Production, Add:**

- **Authentication**: Implement token-based auth or OAuth
- **HTTPS**: Use TLS/SSL certificates
- **Authorization**: Role-based access control
- **Rate Limiting**: Prevent abuse

Example HTTPS setup with a reverse proxy (nginx):

```nginx
server {
    listen 443 ssl;
    server_name git.yourdomain.com;
    
    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;
    
    location / {
        proxy_pass http://localhost:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

### 2. Performance

- **Concurrent Connections**: Server uses threading for multiple clients
- **Large Files**: Consider implementing Git LFS for large binary files
- **Compression**: Add gzip compression for network transfer

### 3. Reliability

- **Process Management**: Use NSSM (Non-Sucking Service Manager) on Windows
  
  ```bash
  # Install as Windows Service
  nssm install MyGitServer "D:\Git\build\Release\mygit-server.exe"
  nssm set MyGitServer AppDirectory "D:\GitServer"
  nssm start MyGitServer
  ```

- **Health Checks**: Add a `/health` endpoint
- **Auto-restart**: Configure automatic restart on failure

### 4. Monitoring & Logging

- Implement structured logging (JSON format)
- Set up log rotation
- Monitor server metrics:
  - Request count
  - Response times
  - Error rates
  - Disk usage

## API Reference

### Endpoints

#### GET /refs
Lists all references (branches)

**Response:**
```
<hash> refs/heads/main
<hash> refs/heads/develop
```

#### GET /object/{hash}
Retrieves an object by hash

**Response:** Binary object data

#### POST /object/{hash}
Uploads an object

**Body:** Binary object data

#### GET /ref/{branch}
Gets the hash of a branch

**Response:** `<commit-hash>`

#### POST /ref/{branch}
Updates a branch reference

**Body:** `<commit-hash>`

## Troubleshooting

### Server Won't Start

**Issue:** "Bind failed"
- **Solution:** Port already in use. Choose different port or kill existing process

```bash
# Find process using port 8080
netstat -ano | findstr :8080

# Kill process
taskkill /PID <PID> /F
```

### Client Can't Connect

**Issue:** "Failed to fetch remote refs"
- Check server is running
- Verify firewall rules
- Test connection: `curl http://server:8080/refs`
- Verify network connectivity: `ping server-ip`

### Push Fails

**Issue:** "Failed to update remote ref"
- Ensure server has write permissions
- Check server logs for errors
- Verify objects were pushed successfully

### Performance Issues

- Monitor server resource usage (CPU, Memory, Disk I/O)
- Check network bandwidth
- Consider implementing object caching
- Use compression for large transfers

## Example Workflow

### Complete Setup from Scratch

```bash
# 1. Build the project
cd D:\Git\build
cmake ..
cmake --build . --config Release

# 2. Set up server repository
mkdir D:\GitServer
cd D:\GitServer
..\build\Release\mygit.exe init

# 3. Start server (in separate terminal)
..\build\Release\mygit-server.exe 8080

# 4. Set up client repository
cd D:\Projects\MyProject
..\..\Git\build\Release\mygit.exe init
..\..\Git\build\Release\mygit.exe remote add origin http://localhost:8080

# 5. Make some commits
echo "Hello" > file.txt
..\..\Git\build\Release\mygit.exe add file.txt
..\..\Git\build\Release\mygit.exe commit -m "Initial commit"

# 6. Push to server
..\..\Git\build\Release\mygit.exe push origin main

# 7. Clone or pull from another location
cd D:\Projects\Clone
..\..\Git\build\Release\mygit.exe init
..\..\Git\build\Release\mygit.exe remote add origin http://localhost:8080
..\..\Git\build\Release\mygit.exe pull origin main
```

## Scaling

### Horizontal Scaling

- Run multiple server instances behind a load balancer
- Use shared storage (NFS, S3) for repositories
- Implement distributed caching

### Vertical Scaling

- Increase server resources (CPU, RAM)
- Use SSD storage for faster I/O
- Optimize object database structure

## Next Steps

### Recommended Enhancements

1. **Authentication & Authorization**
   - User management
   - Access control lists
   - API tokens

2. **Web Interface**
   - Repository browser
   - Commit history viewer
   - User management UI

3. **Advanced Features**
   - Git hooks support
   - Webhooks for CI/CD integration
   - Repository statistics

4. **Protocol Improvements**
   - Git smart HTTP protocol
   - Pack file support
   - Delta compression

## Support & Maintenance

### Regular Maintenance Tasks

- Monitor disk usage
- Rotate logs
- Update dependencies
- Security patches
- Backup verification

### Upgrading

1. Stop the server
2. Backup repositories
3. Build new version
4. Test in staging
5. Deploy to production
6. Restart server

---

## Quick Reference Commands

```bash
# Server
mygit-server [port]                    # Start server

# Client - Remote Management
mygit remote add <name> <url>          # Add remote
mygit remote remove <name>             # Remove remote
mygit remote -v                        # List remotes

# Client - Push/Pull
mygit push <remote> [branch]           # Push to remote
mygit pull <remote> [branch]           # Pull from remote
```

---

**Version:** 1.0  
**Last Updated:** February 2026  
**Author:** MyGit Development Team
