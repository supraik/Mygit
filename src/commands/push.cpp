#include "push.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <set>
#include <queue>
#include <vector>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

namespace fs = std::filesystem;

bool Push::execute(const std::string& remoteName, const std::string& branchName) {
    // Get remote URL
    std::string remoteUrl = getRemoteUrl(remoteName);
    if (remoteUrl.empty()) {
        std::cerr << "Remote '" << remoteName << "' not found\n";
        std::cerr << "Use: mygit remote add <name> <url>\n";
        return false;
    }
    
    // Get current branch if not specified
    std::string pushBranch = branchName.empty() ? getCurrentBranch() : branchName;
    if (pushBranch.empty()) {
        std::cerr << "No branch to push\n";
        return false;
    }
    
    // Get branch hash
    std::string branchHash = getBranchHash(pushBranch);
    if (branchHash.empty()) {
        std::cerr << "Branch '" << pushBranch << "' not found\n";
        return false;
    }
    
    std::cout << "Pushing " << pushBranch << " to " << remoteName << "...\n";
    
    // Get all objects that need to be pushed
    std::vector<std::string> objects = getAllReferencedObjects(branchHash);
    std::cout << "Found " << objects.size() << " objects to push\n";
    
    // Push all objects
    int pushed = 0;
    for (const auto& hash : objects) {
        if (sendObject(remoteUrl, hash)) {
            pushed++;
            std::cout << "\rPushing objects: " << pushed << "/" << objects.size() << std::flush;
        }
    }
    std::cout << "\n";
    
    // Update remote ref
    if (updateRemoteRef(remoteUrl, pushBranch, branchHash)) {
        std::cout << "Successfully pushed " << pushBranch << " -> " << remoteName << "/" << pushBranch << "\n";
        return true;
    }
    
    std::cerr << "Failed to update remote ref\n";
    return false;
}

std::string Push::getRemoteUrl(const std::string& remoteName) {
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
            // Section header
            std::string section = line.substr(1, line.length() - 2);
            if (section.substr(0, 7) == "remote ") {
                currentRemote = section.substr(8, section.length() - 9); // Remove quotes
                inRemoteSection = (currentRemote == remoteName);
            } else {
                inRemoteSection = false;
            }
        } else if (inRemoteSection) {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);
                
                // Trim key and value
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

std::string Push::getCurrentBranch() {
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

std::string Push::getBranchHash(const std::string& branchName) {
    std::string refPath = ".mygit/refs/heads/" + branchName;
    std::ifstream refFile(refPath);
    if (refFile) {
        std::string hash;
        std::getline(refFile, hash);
        while (!hash.empty() && (hash.back() == '\n' || hash.back() == '\r')) {
            hash.pop_back();
        }
        if (!hash.empty()) return hash;
    }

    // Fall back: if HEAD is detached and we're pushing the current branch, use HEAD
    std::ifstream headFile(".mygit/HEAD");
    if (!headFile) return "";
    std::string head;
    std::getline(headFile, head);
    while (!head.empty() && (head.back() == '\n' || head.back() == '\r')) {
        head.pop_back();
    }
    // HEAD is detached (raw hash) — use it and also repair the branch ref
    if (head.substr(0, 5) != "ref: ") {
        std::ofstream repair(refPath);
        if (repair) repair << head << "\n";
        return head;
    }
    return "";
}

std::vector<std::string> Push::getAllReferencedObjects(const std::string& commitHash) {
    std::set<std::string> visited;
    std::queue<std::string> toVisit;
    std::vector<std::string> result;
    
    toVisit.push(commitHash);
    visited.insert(commitHash);
    
    while (!toVisit.empty()) {
        std::string hash = toVisit.front();
        toVisit.pop();
        result.push_back(hash);
        
        // Read object
        std::string content = readObject(hash);
        if (content.empty()) continue;
        
        // Parse object type
        size_t nullPos = content.find('\0');
        if (nullPos == std::string::npos) continue;
        
        std::string header = content.substr(0, nullPos);
        std::string data = content.substr(nullPos + 1);
        
        std::istringstream headerStream(header);
        std::string type;
        headerStream >> type;
        
        if (type == "commit") {
            // Parse commit to find tree and parent
            std::istringstream dataStream(data);
            std::string line;
            
            while (std::getline(dataStream, line)) {
                if (line.empty()) break;
                
                std::istringstream lineStream(line);
                std::string key, hash;
                lineStream >> key >> hash;
                
                if ((key == "tree" || key == "parent") && visited.find(hash) == visited.end()) {
                    toVisit.push(hash);
                    visited.insert(hash);
                }
            }
        } else if (type == "tree") {
            // Parse text-format tree entries: "mode type hash\tfilename\n"
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
    
    return result;
}

bool Push::sendObject(const std::string& url, const std::string& hash) {
    std::string content = readObject(hash);
    if (content.empty()) {
        return false;
    }
    
    std::string endpoint = url + "/object/" + hash;
    std::string response = httpPost(endpoint, content);
    
    return !response.empty();
}

bool Push::updateRemoteRef(const std::string& url, const std::string& refName, const std::string& hash) {
    std::string endpoint = url + "/ref/" + refName;
    std::string response = httpPost(endpoint, hash);
    
    return !response.empty();
}

std::string Push::readObject(const std::string& hash) {
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

std::string Push::httpPost(const std::string& url, const std::string& data) {
    // Parse URL
    std::string host, path;
    bool isHttps = false;
    INTERNET_PORT port = 80;

    size_t protoPos = url.find("://");
    if (protoPos != std::string::npos) {
        std::string scheme = url.substr(0, protoPos);
        isHttps = (scheme == "https");
        port = isHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    }
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
        port = (INTERNET_PORT)std::stoi(host.substr(portPos + 1));
        host = host.substr(0, portPos);
    }

    std::wstring wHost(host.begin(), host.end());
    std::wstring wPath(path.begin(), path.end());

    HINTERNET hSession = WinHttpOpen(L"mygit/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    // Explicitly enable TLS 1.2 for compatibility with modern servers
    DWORD dwProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &dwProtocols, sizeof(dwProtocols));

    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wPath.c_str(),
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    if (isHttps) {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                         SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    }

    std::wstring contentTypeHeader = L"Content-Type: application/octet-stream\r\n";
    BOOL bResult = WinHttpSendRequest(hRequest,
        contentTypeHeader.c_str(), (DWORD)-1L,
        (LPVOID)data.data(), (DWORD)data.size(), (DWORD)data.size(), 0);
    if (!bResult) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    if (!WinHttpReceiveResponse(hRequest, NULL)) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    std::string response;
    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
            response.append(buffer.data(), bytesRead);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}
