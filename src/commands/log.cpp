#include "log.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

bool Log::execute() {
    std::string repoPath;
    if (!findRepoRoot(repoPath)) {
        std::cerr << "fatal: not a mygit repository\n";
        return false;
    }
    
    // Get current commit from HEAD
    std::string currentCommit = getCurrentCommit(repoPath);
    if (currentCommit.empty()) {
        std::cout << "No commits yet\n";
        return true;
    }
    
    // Traverse commits from current to the first commit
    std::string commitHash = currentCommit;
    while (!commitHash.empty()) {
        CommitInfo info;
        if (!readCommitObject(repoPath, commitHash, info)) {
            std::cerr << "error: failed to read commit " << commitHash << "\n";
            return false;
        }
        
        displayCommit(info);
        
        // Move to parent commit
        commitHash = info.parent;
        
        // Print separator between commits
        if (!commitHash.empty()) {
            std::cout << "\n";
        }
    }
    
    return true;
}

bool Log::findRepoRoot(std::string& repoPath) {
    fs::path current = fs::current_path();
    
    while (true) {
        fs::path mygitPath = current / ".mygit";
        if (fs::exists(mygitPath) && fs::is_directory(mygitPath)) {
            repoPath = mygitPath.string();
            return true;
        }
        
        if (!current.has_parent_path() || current == current.parent_path()) {
            return false;
        }
        
        current = current.parent_path();
    }
}

std::string Log::getCurrentCommit(const std::string& repoPath) {
    std::string headPath = repoPath + "/HEAD";
    // Read HEAD file
    std::ifstream headFile(headPath);
    if (!headFile) {
        return ""; // No HEAD file
    }
    
    std::string line;
    std::getline(headFile, line);
    headFile.close();
    
    // Check if it's a symbolic reference (ref: refs/heads/master)
    if (line.substr(0, 5) == "ref: ") {
        // Extract the reference path
        std::string refPath = line.substr(5);
        
        // Remove trailing whitespace
        while (!refPath.empty() && (refPath.back() == '\n' || refPath.back() == '\r')) {
            refPath.pop_back();
        }
        
        // Read the reference file
        std::string refFilePath = repoPath + "/" + refPath;
        
        if (!fs::exists(refFilePath)) {
            // No commits yet
            return "";
        }
        
        std::ifstream refFile(refFilePath);
        if (!refFile) 
        {
            return "";
        }
        
        std::string commitHash;
        std::getline(refFile, commitHash);
        
        // Remove trailing whitespace
        while (!commitHash.empty() && (commitHash.back() == '\n' || commitHash.back() == '\r')) {
            commitHash.pop_back();
        }
        
        return commitHash;
    } else {
        // Direct hash (detached HEAD)
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        return line;
    }
}

std::string Log::readObjectContent(const std::string& repoPath, const std::string& hash) {
    if (hash.size() < 3) {
        return "";
    }
    
    // Object path: .mygit/objects/ab/cdef123...
    std::string objPath = repoPath + "/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    
    if (!fs::exists(objPath)) {
        return "";
    }
    
    std::ifstream objFile(objPath, std::ios::binary);
    if (!objFile) {
        return "";
    }
    
    // Read entire file
    std::string content((std::istreambuf_iterator<char>(objFile)),
                        std::istreambuf_iterator<char>());
    
    return content;
}

bool Log::readCommitObject(const std::string& repoPath, const std::string& commitHash, CommitInfo& info) {
    // Read object content
    std::string content = readObjectContent(repoPath, commitHash);
    if (content.empty()) {
        return false;
    }
    
    // Parse header: "commit <size>\0"
    size_t nullPos = content.find('\0');
    if (nullPos == std::string::npos) {
        return false;
    }
    
    std::string header = content.substr(0, nullPos);
    if (header.substr(0, 7) != "commit ") {
        return false;
    }
    

    // Get commit body (after null byte)
    std::string body = content.substr(nullPos + 1);
    
    
    // Parse commit body
    info.hash = commitHash;
    info.parent = "";  // Default to empty
    
    std::istringstream stream(body);
    std::string line;
    bool inMessage = false;
    std::ostringstream messageStream;
    
    while (std::getline(stream, line)) {
        // Remove carriage return if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (inMessage) {
            // Everything after the blank line is the message
            messageStream << line << "\n";
        } else if (line.empty()) {
            // Blank line indicates start of message
            inMessage = true;
        } else if (line.substr(0, 5) == "tree ") {
            info.tree = line.substr(5);
        } else if (line.substr(0, 7) == "parent ") {
            // Only use the first parent for log traversal (skip merge parents)
            if (info.parent.empty()) {
                info.parent = line.substr(7);
            }
        } else if (line.substr(0, 7) == "author ") {
            info.author = line.substr(7);
            // Extract timestamp from author line
            size_t lastSpace = info.author.rfind(' ');
            if (lastSpace != std::string::npos) {
                size_t secondLastSpace = info.author.rfind(' ', lastSpace - 1);
                if (secondLastSpace != std::string::npos) {
                    info.timestamp = info.author.substr(secondLastSpace + 1, lastSpace - secondLastSpace - 1);
                }
            }
        } else if (line.substr(0, 10) == "committer ") {
            info.committer = line.substr(10);
        }
    }
    
    info.message = messageStream.str();
    // Remove trailing newline from message
    while (!info.message.empty() && info.message.back() == '\n') {
        info.message.pop_back();
    }
    
    return true;
}

void Log::displayCommit(const CommitInfo& info) {
    std::cout << "commit " << info.hash << "\n";
    std::cout << "Author: " << info.author << "\n";
    
    // Convert timestamp to readable format if available
    if (!info.timestamp.empty()) {
        try {
            std::time_t timestamp = std::stoll(info.timestamp);
            std::tm* timeinfo = std::localtime(&timestamp);
            char buffer[80];
            std::strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", timeinfo);
            std::cout << "Date:   " << buffer << "\n";
        } catch (...) {
            // If timestamp parsing fails, just skip it
        }
    }
    
    std::cout << "\n";
    std::cout << "    " << info.message << "\n";
}
