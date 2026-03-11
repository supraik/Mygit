#include "pull.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <set>
#include <queue>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

namespace fs = std::filesystem;

bool Pull::execute(const std::string& remoteName, const std::string& branchName) {
    // Get remote URL
    std::string remoteUrl = getRemoteUrl(remoteName);
    if (remoteUrl.empty()) {
        std::cerr << "Remote '" << remoteName << "' not found\n";
        std::cerr << "Use: mygit remote add <name> <url>\n";
        return false;
    }
    
    // Get current branch if not specified
    std::string pullBranch = branchName.empty() ? getCurrentBranch() : branchName;
    if (pullBranch.empty()) {
        std::cerr << "No branch to pull\n";
        return false;
    }
    
    std::cout << "Pulling " << pullBranch << " from " << remoteName << "...\n";
    
    // Fetch remote refs
    std::map<std::string, std::string> remoteRefs = fetchRemoteRefs(remoteUrl);
    if (remoteRefs.empty()) {
        std::cerr << "Failed to fetch remote refs\n";
        return false;
    }
    
    std::string remoteRefName = "refs/heads/" + pullBranch;
    if (remoteRefs.find(remoteRefName) == remoteRefs.end()) {
        std::cerr << "Remote branch '" << pullBranch << "' not found\n";
        return false;
    }
    
    std::string remoteHash = remoteRefs[remoteRefName];
    std::cout << "Remote " << pullBranch << " at " << remoteHash << "\n";
    
    // Iteratively fetch missing objects, discovering dependencies as we go
    std::set<std::string> visited;
    std::queue<std::string> toVisit;
    int fetched = 0;
    
    toVisit.push(remoteHash);
    visited.insert(remoteHash);
    
    while (!toVisit.empty()) {
        std::string hash = toVisit.front();
        toVisit.pop();
        
        if (!objectExists(hash)) {
            if (!fetchObject(remoteUrl, hash)) {
                std::cerr << "\nFailed to fetch object: " << hash << "\n";
                continue;
            }
            fetched++;
            std::cout << "\rFetching objects: " << fetched << std::flush;
        }
        
        // Now parse the object to discover dependencies
        std::string content = readObject(hash);
        if (content.empty()) continue;
        
        size_t nullPos = content.find('\0');
        if (nullPos == std::string::npos) continue;
        
        std::string header = content.substr(0, nullPos);
        std::string data = content.substr(nullPos + 1);
        
        std::istringstream headerStream(header);
        std::string type;
        headerStream >> type;
        
        if (type == "commit") {
            std::istringstream dataStream(data);
            std::string line;
            while (std::getline(dataStream, line)) {
                if (line.empty()) break;
                std::istringstream lineStream(line);
                std::string key, objHash;
                lineStream >> key >> objHash;
                if ((key == "tree" || key == "parent") && visited.find(objHash) == visited.end()) {
                    toVisit.push(objHash);
                    visited.insert(objHash);
                }
            }
        } else if (type == "tree") {
            std::istringstream treeStream(data);
            std::string treeLine;
            while (std::getline(treeStream, treeLine)) {
                if (treeLine.empty()) continue;
                size_t tabPos = treeLine.find('\t');
                if (tabPos == std::string::npos) continue;
                std::string meta = treeLine.substr(0, tabPos);
                std::istringstream metaStream(meta);
                std::string mode, objType, objHash;
                if (metaStream >> mode >> objType >> objHash) {
                    if (visited.find(objHash) == visited.end()) {
                        toVisit.push(objHash);
                        visited.insert(objHash);
                    }
                }
            }
        }
    }
    if (fetched > 0) std::cout << "\n";
    std::cout << "Fetched " << fetched << " objects\n";
    
    // Update local ref
    if (updateLocalRef(pullBranch, remoteHash)) {
        std::cout << "Successfully pulled " << remoteName << "/" << pullBranch << " -> " << pullBranch << "\n";
        return true;
    }
    
    std::cerr << "Failed to update local ref\n";
    return false;
}

std::string Pull::getRemoteUrl(const std::string& remoteName) {
    std::string configPath = ".mygit/config";
    std::ifstream configFile(configPath);
    if (!configFile) {
        return "";
    }
    
    std::string line;
    bool inRemoteSection = false;
    std::string currentRemote;
    
    while (std::getline(configFile, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        if (line[0] == '[' && line.back() == ']') {
            std::string section = line.substr(1, line.length() - 2);
            if (section.substr(0, 7) == "remote ") {
                currentRemote = section.substr(8, section.length() - 9);
                inRemoteSection = (currentRemote == remoteName);
            } else {
                inRemoteSection = false;
            }
        } else if (inRemoteSection) {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);
                
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                if (key == "url") {
                    return value;
                }
            }
        }
    }
    
    return "";
}

std::string Pull::getCurrentBranch() {
    std::ifstream headFile(".mygit/HEAD");
    if (!headFile) {
        return "";
    }
    
    std::string head;
    std::getline(headFile, head);
    
    // Remove trailing whitespace
    while (!head.empty() && (head.back() == '\n' || head.back() == '\r')) {
        head.pop_back();
    }
    
    if (head.substr(0, 16) == "ref: refs/heads/") {
        return head.substr(16);
    }
    
    return "";
}

std::map<std::string, std::string> Pull::fetchRemoteRefs(const std::string& url) {
    std::map<std::string, std::string> refs;
    
    std::string endpoint = url + "/refs";
    std::string response = httpGet(endpoint);
    
    if (response.empty()) {
        return refs;
    }
    
    // Parse response (format: "hash refname\n")
    std::istringstream iss(response);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        
        std::istringstream lineStream(line);
        std::string hash, refName;
        lineStream >> hash >> refName;
        
        if (!hash.empty() && !refName.empty()) {
            refs[refName] = hash;
        }
    }
    
    return refs;
}

bool Pull::fetchObject(const std::string& url, const std::string& hash) {
    if (hash.length() < 2) return false;
    
    std::string endpoint = url + "/object/" + hash;
    
    std::string dir = hash.substr(0, 2);
    std::string file = hash.substr(2);
    std::string dirPath = ".mygit/objects/" + dir;
    std::string filePath = dirPath + "/" + file;
    
    // Create directory if needed
    fs::create_directories(dirPath);
    
    return httpGetToFile(endpoint, filePath);
}

bool Pull::updateLocalRef(const std::string& branchName, const std::string& hash) {
    std::string refPath = ".mygit/refs/heads/" + branchName;
    
    std::ofstream refFile(refPath);
    if (!refFile) {
        return false;
    }
    
    refFile << hash;
    return true;
}

std::string Pull::httpGet(const std::string& url) {
    // Parse URL
    std::string host, path;
    int port = 8080;
    
    size_t protoPos = url.find("://");
    std::string urlWithoutProto = (protoPos != std::string::npos) ? url.substr(protoPos + 3) : url;
    
    size_t pathPos = urlWithoutProto.find('/');
    if (pathPos != std::string::npos) {
        host = urlWithoutProto.substr(0, pathPos);
        path = urlWithoutProto.substr(pathPos);
    } else {
        host = urlWithoutProto;
        path = "/";
    }
    
    size_t portPos = host.find(':');
    if (portPos != std::string::npos) {
        port = std::stoi(host.substr(portPos + 1));
        host = host.substr(0, portPos);
    }
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return "";
    }
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return "";
    }
    
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0 || !result) {
        closesocket(sock);
        WSACleanup();
        return "";
    }
    
    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        freeaddrinfo(result);
        closesocket(sock);
        WSACleanup();
        return "";
    }
    freeaddrinfo(result);
    
    // Send HTTP GET request
    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: " << host << "\r\n";
    request << "Connection: close\r\n";
    request << "\r\n";
    
    std::string requestStr = request.str();
    send(sock, requestStr.c_str(), requestStr.length(), 0);
    
    // Receive response (binary-safe)
    char buffer[4096];
    std::string response;
    int bytesReceived;
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytesReceived);
    }
    
    closesocket(sock);
    WSACleanup();
    
    // Extract body from HTTP response
    size_t bodyPos = response.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        return response.substr(bodyPos + 4);
    }
    
    return "";
}

bool Pull::httpGetToFile(const std::string& url, const std::string& filePath) {
    std::string content = httpGet(url);
    if (content.empty()) {
        return false;
    }
    
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile) {
        return false;
    }
    
    outFile << content;
    return true;
}

bool Pull::objectExists(const std::string& hash) {
    if (hash.length() < 2) return false;
    
    std::string dir = hash.substr(0, 2);
    std::string file = hash.substr(2);
    std::string path = ".mygit/objects/" + dir + "/" + file;
    
    return fs::exists(path);
}

std::string Pull::readObject(const std::string& hash) {
    if (hash.length() < 2) return "";
    
    std::string dir = hash.substr(0, 2);
    std::string file = hash.substr(2);
    std::string path = ".mygit/objects/" + dir + "/" + file;
    
    std::ifstream objFile(path, std::ios::binary);
    if (!objFile) {
        return "";
    }
    
    std::ostringstream oss;
    oss << objFile.rdbuf();
    return oss.str();
}
