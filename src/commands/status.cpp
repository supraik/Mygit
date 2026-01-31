#include "status.h"
#include "../utils/hash.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>

namespace fs = std::filesystem;

bool Status::execute() {
    // Find repository root
    std::string repoPath;
    if (!findRepoRoot(repoPath)) {
        std::cerr << "fatal: not a mygit repository\n";
        return false;
    }
    
    // Read index (staged files)
    std::map<std::string, std::string> indexFiles = readIndex(repoPath);
    
    // Get working directory files
    std::vector<std::string> workingFiles = getWorkingDirFiles();
    
    // Track which files we've processed
    std::set<std::string> processedFiles;
    
    // Find staged and modified files
    std::vector<std::string> stagedFiles;
    std::vector<std::string> modifiedFiles;
    
    for (const auto& entry : indexFiles) {
        const std::string& fileName = entry.first;
        const std::string& indexHash = entry.second;
        
        processedFiles.insert(fileName);
        
        if (!fs::exists(fileName)) {
            // File deleted in working directory
            modifiedFiles.push_back(fileName + " (deleted)");
        } else {
            // Check if file is modified
            std::string currentHash = computeFileHash(fileName);
            if (currentHash != indexHash) {
                modifiedFiles.push_back(fileName);
            }
        }
        
        stagedFiles.push_back(fileName);
    }
    
    // Find untracked files
    std::vector<std::string> untrackedFiles;
    for (const auto& file : workingFiles) {
        if (processedFiles.find(file) == processedFiles.end()) {
            untrackedFiles.push_back(file);
        }
    }
    
    // Display status
    std::cout << "On branch master\n\n";
    
    if (!stagedFiles.empty()) {
        std::cout << "Changes to be committed:\n";
        std::cout << "  (use \"mygit reset <file>...\" to unstage)\n\n";
        for (const auto& file : stagedFiles) {
            std::cout << "\tnew file:   " << file << "\n";
        }
        std::cout << "\n";
    }
    
    if (!modifiedFiles.empty()) {
        std::cout << "Changes not staged for commit:\n";
        std::cout << "  (use \"mygit add <file>...\" to update what will be committed)\n\n";
        for (const auto& file : modifiedFiles) {
            std::cout << "\tmodified:   " << file << "\n";
        }
        std::cout << "\n";
    }
    
    if (!untrackedFiles.empty()) {
        std::cout << "Untracked files:\n";
        std::cout << "  (use \"mygit add <file>...\" to include in what will be committed)\n\n";
        for (const auto& file : untrackedFiles) {
            std::cout << "\t" << file << "\n";
        }
        std::cout << "\n";
    }
    
    if (stagedFiles.empty() && modifiedFiles.empty() && untrackedFiles.empty()) {
        std::cout << "nothing to commit, working tree clean\n";
    }
    
    return true;
}

bool Status::findRepoRoot(std::string& repoPath) {
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

std::map<std::string, std::string> Status::readIndex(const std::string& repoPath) {
    std::map<std::string, std::string> indexFiles;
    std::string indexPath = repoPath + "/index";
    
    if (!fs::exists(indexPath)) {
        return indexFiles;
    }
    
    std::ifstream indexFile(indexPath);
    std::string line;
    
    while (std::getline(indexFile, line)) {
        std::istringstream iss(line);
        std::string mode, hash, fileName;
        
        if (iss >> mode >> hash) {
            std::getline(iss, fileName);
            // Trim leading space
            if (!fileName.empty() && fileName[0] == ' ') {
                fileName = fileName.substr(1);
            }
            indexFiles[fileName] = hash;
        }
    }
    
    return indexFiles;
}

std::vector<std::string> Status::getWorkingDirFiles() {
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file()) {
                std::string fileName = entry.path().filename().string();
                // Skip hidden files and mygit executable
                if (fileName[0] != '.' && fileName != "mygit.exe") {
                    files.push_back(fileName);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << "\n";
    }
    
    return files;
}

std::string Status::computeFileHash(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "";
    }
    
    std::vector<char> content{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };
    
    return Hash::blobHash(content);
}
