#include "rm.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

bool Rm::execute(const std::string& filePath) {
   
    std::string repoPath;
    if(!findRepoRoot(repoPath)) {
        std::cerr << "fatal: not a mygit repository\n";
        return false;
    }

    if(!fs::exists(filePath)) {
        std::cerr << "fatal: pathspec '" << filePath << "' did not match any files\n";
        return false;
    }

   if(!updateIndex(repoPath, filePath)) {
        std::cerr << "error: failed to update index\n";
        return false;
    }

    if(!fs::remove(filePath)) {
        std::cerr << "error: failed to delete file '" << filePath << "'\n";
        return false;
    }
    std::cout<< "Removed '" << filePath << "' from index and working directory\n";
    return true;
}


bool Rm::updateIndex(const std::string& repoPath, const std::string& filePath) {
    std::string indexPath = repoPath + "/index";
    
    // Read existing index
    std::ifstream indexFile(indexPath);
    if (!indexFile) {
        return false;
    }
    
    std::string indexContent((std::istreambuf_iterator<char>(indexFile)),
                              std::istreambuf_iterator<char>());
    indexFile.close();
    
    // Parse and remove the entry for filePath
    std::istringstream indexStream(indexContent);
    std::string line;
    std::string updatedIndex;
    bool found = false;
    
    while (std::getline(indexStream, line)) {
        if (line.find(filePath) != std::string::npos) {
            found = true; // Skip this line to remove it
        } else {
            updatedIndex += line + "\n";
        }
    }
    
    if (!found) {
        std::cerr << "fatal: pathspec '" << filePath << "' did not match any files in index\n";
        return false;
    }
    
    // Write updated index back
    std::ofstream outIndexFile(indexPath, std::ios::binary | std::ios::trunc);
    if (!outIndexFile) {
        return false;
    }
    
    outIndexFile << updatedIndex;
    outIndexFile.close();
    return true;
}


bool Rm::findRepoRoot(std::string& repoPath) {
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