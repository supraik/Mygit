#include "commit.h"
#include "../utils/hash.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <ctime>
namespace fs = std::filesystem;


bool Commit::execute(const std::string& message) {

    std::string repoPath;
    if (!findRepoRoot(repoPath)) {
        std::cerr << "fatal: not a mygit repository\n";
        return false;
    }
    
    // Read index
    std::map<std::string, std::string> index = readIndex(repoPath);
    if (index.empty()) {
        std::cerr << "error: nothing to commit\n";
        return false;
    }
    
    // Create tree object
    std::string treeHash = createTreeObject(repoPath, index);
    if (treeHash.empty()) {
        std::cerr << "error: failed to create tree object\n";
        return false;
    }
    
    // Get parent commit
    std::string parent = getParentCommit(repoPath);
    
    // Create commit object
    std::string commitHash = createCommitObject(repoPath, treeHash, parent, message);
    if (commitHash.empty()) {
        std::cerr << "error: failed to create commit object\n";
        return false;
    }
    
    // Note: Commit object is already stored in createCommitObject
    // Tree object is already stored in createTreeObject
    
    // Update HEAD reference
    if (!updateHead(repoPath, commitHash)) {
        std::cerr << "error: failed to update HEAD reference\n";
        return false;
    }
    
    std::cout << "Committed with hash: " << commitHash << "\n";
    return true;
}



bool Commit::findRepoRoot(std::string& repoPath) {
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

// Read the index file and return a map of filepath -> hash
std::map<std::string, std::string> Commit::readIndex(const std::string& repoPath) {
    std::map<std::string, std::string> index;
    std::string indexPath = repoPath + "/index";
    
    if (!fs::exists(indexPath)) {
        return index; // Empty map if no index
    }
    
    std::ifstream indexFile(indexPath);
    if (!indexFile) {
        return index;
    }
    
    // Parse index file: "mode hash filepath"
    std::string line;
    while (std::getline(indexFile, line)) {
        std::istringstream iss(line);
        std::string mode, hash, filepath;
        
        if (iss >> mode >> hash) {
            // Read the rest as filepath (handles spaces in filenames)
            std::getline(iss, filepath);
            // Trim leading space
            if (!filepath.empty() && filepath[0] == ' ') {
                filepath = filepath.substr(1);
            }
            
            // Store filepath -> hash mapping
            index[filepath] = hash;
        }
    }
    
    return index;
}

// Create a tree object from the index
std::string Commit::createTreeObject(const std::string& repoPath, const std::map<std::string, std::string>& index) {
    // Build tree content: each line is "mode type hash\tfilename"
    std::string treeContent;
    
    for (const auto& entry : index) {
        const std::string& filepath = entry.first;
        const std::string& hash = entry.second;
        
        // Git tree format: "mode blob hash\tfilename\n"
        treeContent += "100644 blob " + hash + "\t" + filepath + "\n";
    }
    
    // Create tree header: "tree <size>\0"
    std::string header = "tree " + std::to_string(treeContent.size()) + '\0';
    std::string treeData = header + treeContent;
    
    // Compute hash
    std::string treeHash = Hash::sha1(treeData);
    
    // Store the tree object
    if (!storeObject(repoPath, treeHash, treeData)) {
        return "";
    }
    
    return treeHash;
}

// Get the parent commit hash from HEAD
std::string Commit::getParentCommit(const std::string& repoPath) {
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
        
        // Remove trailing newline
        if (!refPath.empty() && refPath.back() == '\n') {
            refPath.pop_back();
        }
        if (!refPath.empty() && refPath.back() == '\r') {
            refPath.pop_back();
        }
        
        // Read the reference file
        std::string refFilePath = repoPath + "/" + refPath;
        
        if (!fs::exists(refFilePath)) {
            // First commit - no parent
            return "";
        }
        
        std::ifstream refFile(refFilePath);
        if (!refFile) {
            return "";
        }
        
        std::string commitHash;
        std::getline(refFile, commitHash);
        
        // Remove trailing newline
        if (!commitHash.empty() && commitHash.back() == '\n') {
            commitHash.pop_back();
        }
        if (!commitHash.empty() && commitHash.back() == '\r') {
            commitHash.pop_back();
        }
        
        return commitHash;
    } else {
        // Direct hash (detached HEAD)
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        return line;
    }
}

// Create a commit object
std::string Commit::createCommitObject(const std::string& repoPath, const std::string& treeHash, const std::string& parent, const std::string& message) {
    // Get current timestamp
    std::time_t now = std::time(nullptr);
    
    // Build commit content
    std::ostringstream commitContent;
    commitContent << "tree " << treeHash << "\n";
    
    // Add parent if not the first commit
    if (!parent.empty()) {
        commitContent << "parent " << parent << "\n";
    }
    
    // Author and committer info (simplified - you can enhance this later)
    commitContent << "author User <user@example.com> " << now << " +0000\n";
    commitContent << "committer User <user@example.com> " << now << " +0000\n";
    commitContent << "\n"; // Blank line before message
    commitContent << message << "\n";
    
    std::string content = commitContent.str();
    
    // Create commit header: "commit <size>\0"
    std::string header = "commit " + std::to_string(content.size()) + '\0';
    std::string commitData = header + content;
    
    // Compute hash
    std::string commitHash = Hash::sha1(commitData);
    
    // Store the commit object
    if (!storeObject(repoPath, commitHash, commitData)) {
        return "";
    }
    
    return commitHash;
}

// Store an object in the objects directory
bool Commit::storeObject(const std::string& repoPath, const std::string& hash, const std::string& content) {
    // Create path: .mygit/objects/ab/cdef123...
    std::string dirPath = repoPath + "/objects/" + hash.substr(0, 2);
    std::string objPath = dirPath + "/" + hash.substr(2);
    
    // Create directory if it doesn't exist
    try {
        fs::create_directories(dirPath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory: " << e.what() << "\n";
        return false;
    }
    
    // Write object
    std::ofstream objFile(objPath, std::ios::binary);
    if (!objFile) {
        return false;
    }
    
    objFile.write(content.c_str(), content.size());
    
    return objFile.good();
}

// Update HEAD to point to the new commit
bool Commit::updateHead(const std::string& repoPath, const std::string& commitHash) {
    std::string headPath = repoPath + "/HEAD";
    
    // Read HEAD to find out what branch we're on
    std::ifstream headFile(headPath);
    if (!headFile) {
        std::cerr << "Failed to read HEAD\n";
        return false;
    }
    
    std::string line;
    std::getline(headFile, line);
    headFile.close();
    
    // Check if it's a symbolic reference
    if (line.substr(0, 5) == "ref: ") {
        // Extract reference path
        std::string refPath = line.substr(5);
        
        // Remove trailing newline
        if (!refPath.empty() && refPath.back() == '\n') {
            refPath.pop_back();
        }
        if (!refPath.empty() && refPath.back() == '\r') {
            refPath.pop_back();
        }
        
        // Update the branch reference file
        std::string refFilePath = repoPath + "/" + refPath;
        
        // Create parent directories if needed
        fs::path refFilePathObj(refFilePath);
        try {
            fs::create_directories(refFilePathObj.parent_path());
        } catch (const std::exception& e) {
            std::cerr << "Failed to create ref directory: " << e.what() << "\n";
            return false;
        }
        
        std::ofstream refFile(refFilePath);
        if (!refFile) {
            std::cerr << "Failed to update branch reference\n";
            return false;
        }
        
        refFile << commitHash << "\n";
        return true;
    } else {
        // Detached HEAD - update HEAD directly
        std::ofstream headFileOut(headPath);
        if (!headFileOut) {
            return false;
        }
        
        headFileOut << commitHash << "\n";
        return true;
    }
}