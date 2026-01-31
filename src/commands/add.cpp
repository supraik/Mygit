#include "add.h"
#include "../utils/hash.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

namespace fs = std::filesystem;

bool Add::execute(const std::string& filePath) {
    // Find repository root
    std::string repoPath;
    if (!findRepoRoot(repoPath)) {
        std::cerr << "fatal: not a mygit repository\n";
        return false;
    }
    
    // Check if file exists
    if (!fs::exists(filePath)) {
        std::cerr << "fatal: pathspec '" << filePath << "' did not match any files\n";
        return false;
    }
    
    // Read file content
    std::vector<char> content = readFile(filePath);
    if (content.empty() && fs::file_size(filePath) > 0) {
        std::cerr << "error: failed to read file '" << filePath << "'\n";
        return false;
    }
    
    // Compute hash
    std::string hash = Hash::blobHash(content);
    if (hash.empty()) {
        std::cerr << "error: failed to compute hash\n";
        return false;
    }
    
    // Store object
    if (!storeObject(repoPath, hash, content)) {
        std::cerr << "error: failed to store object\n";
        return false;
    }
    
    // Update index
    if (!updateIndex(repoPath, hash, filePath)) {
        std::cerr << "error: failed to update index\n";
        return false;
    }
    
    std::cout << "Added '" << filePath << "' (hash: " << hash << ")\n";
    return true;
}

bool Add::findRepoRoot(std::string& repoPath) {
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

std::vector<char> Add::readFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return {};
    }
    
    return std::vector<char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

bool Add::storeObject(const std::string& repoPath, const std::string& hash, const std::vector<char>& content) {
    // Create object path: .mygit/objects/ab/cdef...
    std::string dirPath = repoPath + "/objects/" + hash.substr(0, 2);
    std::string objPath = dirPath + "/" + hash.substr(2);
    
    // Check if object already exists
    if (fs::exists(objPath)) {
        return true; // Already stored
    }
    
    // Create directory
    try {
        fs::create_directories(dirPath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory: " << e.what() << "\n";
        return false;
    }
    
    // Write object (header + content)
    std::ofstream objFile(objPath, std::ios::binary);
    if (!objFile) {
        return false;
    }
    
    // Write git blob format: "blob <size>\0<content>"
    std::string header = "blob " + std::to_string(content.size()) + '\0';
    objFile.write(header.c_str(), header.size());
    objFile.write(content.data(), content.size());
    
    return objFile.good();
}

bool Add::updateIndex(const std::string& repoPath, const std::string& hash, const std::string& filePath) {
    std::string indexPath = repoPath + "/index";
    
    // Read existing index
    std::map<std::string, std::pair<std::string, std::string>> entries; // filename -> (mode, hash)
    
    if (fs::exists(indexPath)) {
        std::ifstream indexFile(indexPath);
        std::string line;
        while (std::getline(indexFile, line)) {
            std::istringstream iss(line);
            std::string mode, fileHash, fileName;
            if (iss >> mode >> fileHash) {
                std::getline(iss, fileName);
                // Trim leading space
                if (!fileName.empty() && fileName[0] == ' ') {
                    fileName = fileName.substr(1);
                }
                entries[fileName] = {mode, fileHash};
            }
        }
    }
    
    // Add/update entry
    entries[filePath] = {"100644", hash};
    
    // Write updated index
    std::ofstream indexFile(indexPath);
    if (!indexFile) {
        return false;
    }
    
    for (const auto& entry : entries) {
        indexFile << entry.second.first << " " << entry.second.second << " " << entry.first << "\n";
    }
    
    return true;
}
