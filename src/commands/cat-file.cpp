#include "cat-file.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

bool CatFile::execute(const std::string& option, const std::string& hash) {
    // Find repository root
    std::string repoPath;
    if (!findRepoRoot(repoPath)) {
        std::cerr << "fatal: not a mygit repository\n";
        return false;
    }
    
    // Read the object
    std::string objectData = readObject(repoPath, hash);
    if (objectData.empty()) {
        std::cerr << "fatal: not a valid object name " << hash << "\n";
        return false;
    }
    
    // Parse object: extract type, size, and content
    std::string type;
    int size;
    std::string content;
    parseObject(objectData, type, size, content);
    
    // Handle different options
    if (option == "-p") {
        // Print content (pretty-print)
        std::cout << content;
    }
    else if (option == "-s") {
        // Show size
        std::cout << size << "\n";
    }
    else if (option == "-t") {
        // Show type
        std::cout << type << "\n";
    }
    else {
        std::cerr << "fatal: invalid option: " << option << "\n";
        std::cerr << "Usage: mygit cat-file (-p|-s|-t) <object>\n";
        return false;
    }
    
    return true;
}

bool CatFile::findRepoRoot(std::string& repoPath) {
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

std::string CatFile::readObject(const std::string& repoPath, const std::string& hash) {
    // Object path: .mygit/objects/ab/cdef123...
    if (hash.length() < 40) {
        std::cerr << "error: hash too short\n";
        return "";
    }
    
    std::string objDir = hash.substr(0, 2);     // First 2 chars
    std::string objFile = hash.substr(2);       // Remaining 38 chars
    std::string objPath = repoPath + "/objects/" + objDir + "/" + objFile;
    
    // Check if object exists
    if (!fs::exists(objPath)) {
        return "";
    }
    
    // Read the entire object file
    std::ifstream file(objPath, std::ios::binary);
    if (!file) {
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    return content;
}

void CatFile::parseObject(const std::string& objectData, 
                          std::string& type, 
                          int& size, 
                          std::string& content) {
    // Object format: "blob 12\0Hello World"
    //                 ↑    ↑  ↑  ↑
    //              type size \0 content
    
    // Find the null byte separator
    size_t nullPos = objectData.find('\0');
    if (nullPos == std::string::npos) {
        type = "unknown";
        size = 0;
        content = "";
        return;
    }
    
    // Header is everything before null byte: "blob 12"
    std::string header = objectData.substr(0, nullPos);
    
    // Content is everything after null byte
    content = objectData.substr(nullPos + 1);
    
    // Parse header: "blob 12" -> type="blob", size=12
    std::istringstream iss(header);
    iss >> type >> size;
}
