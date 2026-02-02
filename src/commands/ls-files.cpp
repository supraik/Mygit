#include "ls-files.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

bool LsFiles::execute() {
    // Find repository root
    std::string repoPath;
    if (!findRepoRoot(repoPath)) {
        std::cerr << "fatal: not a mygit repository\n";
        return false;
    }
    
    // Read index
    std::map<std::string, std::string> indexFiles = readIndex(repoPath);
    
    if (indexFiles.empty()) {
        // No files staged, silent success
        return true;
    }
    
    // Print all staged files
    for (const auto& entry : indexFiles) {
        std::cout << entry.first << "\n";
    }
    
    return true;
}

bool LsFiles::findRepoRoot(std::string& repoPath) {
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

std::map<std::string, std::string> LsFiles::readIndex(const std::string& repoPath) {
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
