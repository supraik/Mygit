#include "init.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

bool Init::execute(const std::string& path) {
    std::string repoPath = path + "/.mygit";
    
    // Check if already initialized
    if (fs::exists(repoPath)) {
        std::cerr << "Repository already initialized\n";
        return false;
    }
    
    try {
        // Create .mygit directory structure
        fs::create_directory(repoPath);
        fs::create_directory(repoPath + "/objects");
        fs::create_directory(repoPath + "/refs");
        fs::create_directory(repoPath + "/refs/heads");
        
        // Create config file
        std::ofstream config(repoPath + "/config");
        config << "[core]\n\trepositoryformatversion = 0\n";
        config.close();
        
        // Create HEAD file (points to master branch)
        std::ofstream head(repoPath + "/HEAD");
        head << "ref: refs/heads/master\n";
        head.close();
        
        std::cout << "Initialized empty repository in " << repoPath << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
}