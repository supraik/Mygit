#include "branch.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;


bool Branch::execute(const std::string& target, const std::string& flag) {
    // Find repo root
    std::string repoPath;
    if (!findRepoRoot(repoPath)) {
        std::cerr << "fatal: not a mygit repository\n";
        return false;
    }
    
    // Check if delete flag is provided
    if (flag == "-d") {
        return deleteBranch(repoPath, target);
    }
    
    // Get current commit hash from HEAD
    std::string currentCommit = getCurrentCommit(repoPath);
    if (currentCommit.empty()) {
        std::cerr << "error: no commits to branch from\n";
        return false;
    }
    
    // Create new branch reference file with current commit hash
    if (!createBranch(repoPath, target, currentCommit)) {
        std::cerr << "error: failed to create branch\n";
        return false;
    }
    
    std::cout << "Created branch '" << target << "' at commit " << currentCommit << "\n";
    return true;
}

std::string Branch::getCurrentCommit(const std::string& repoPath) {
    std::string headPath = repoPath + "/HEAD";
    // Read HEAD file
    std::ifstream headFile(headPath);
    if (!headFile) {
        std::cerr << "fatal: HEAD file does not exist\n";
        return ""; // No HEAD file
    }

    std::string line;
    std::getline(headFile, line);
    headFile.close();
    
    // Remove trailing newlines
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }
    
    // Check if it's a symbolic reference (ref: refs/heads/master)
    if (line.substr(0, 5) == "ref: ") {
        // Extract the reference path
        std::string refPath = line.substr(5);
        
        // Read the reference file
        std::string refFilePath = repoPath + "/" + refPath;
        
        if (!fs::exists(refFilePath)) {
            // No commits yet
            return "";
        }
        
        std::ifstream refFile(refFilePath);
        if (!refFile) {
            return "";
        }
        
        std::string commitHash;
        std::getline(refFile, commitHash);
        
        // Remove trailing newlines
        while (!commitHash.empty() && (commitHash.back() == '\n' || commitHash.back() == '\r')) {
            commitHash.pop_back();
        }
        
        return commitHash;
    } else {
        // Direct hash (detached HEAD)
        return line;
    }
}

bool Branch::createBranch(const std::string& repoPath, const std::string& branchName, const std::string& commitHash) {
    std::string refPath = repoPath + "/refs/heads/" + branchName;
    
    // Check if branch already exists
    if (fs::exists(refPath)) {
        std::cerr << "fatal: A branch named '" << branchName << "' already exists.\n";
        return false;
    }
    
    // Create parent directories if needed
    fs::path refPathObj(refPath);
    try {
        fs::create_directories(refPathObj.parent_path());
    } catch (const std::exception& e) {
        std::cerr << "Failed to create ref directory: " << e.what() << "\n";
        return false;
    }
    
    // Write commit hash to branch reference file
    std::ofstream refFile(refPath);
    if (!refFile) {
        std::cerr << "fatal: failed to create branch reference\n";
        return false;
    }
    refFile << commitHash << "\n";
    return true;
}

std::string Branch::getCurrentBranch(const std::string& repoPath) {
    std::string headPath = repoPath + "/HEAD";
    
    std::ifstream headFile(headPath);
    if (!headFile) {
        return "";
    }
    
    std::string line;
    std::getline(headFile, line);
    headFile.close();
    
    // Remove trailing newlines
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }
    
    // Check if it's a symbolic reference (ref: refs/heads/master)
    if (line.substr(0, 5) == "ref: ") {
        std::string refPath = line.substr(5);
        // Extract branch name from refs/heads/branchname
        if (refPath.substr(0, 11) == "refs/heads/") {
            return refPath.substr(11);
        }
    }
    
    return ""; // Detached HEAD or invalid
}

bool Branch::deleteBranch(const std::string& repoPath, const std::string& branchName) {
    std::string refPath = repoPath + "/refs/heads/" + branchName;
    
    // Check if branch exists
    if (!fs::exists(refPath)) {
        std::cerr << "error: branch '" << branchName << "' not found.\n";
        return false;
    }
    
    // Check if trying to delete current branch
    std::string currentBranch = getCurrentBranch(repoPath);
    if (currentBranch == branchName) {
        std::cerr << "error: Cannot delete branch '" << branchName << "' checked out at '" << fs::current_path().string() << "'\n";
        return false;
    }
    
    // Delete the branch reference file
    try {
        fs::remove(refPath);
        std::cout << "Deleted branch " << branchName << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "error: failed to delete branch: " << e.what() << "\n";
        return false;
    }
}

bool Branch::findRepoRoot(std::string& repoPath) {
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