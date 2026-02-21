#include "checkout.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;


bool Checkout::execute(const std::string& target) {
        // Find repo root
        std::string repoPath;
        if (!findRepoRoot(repoPath)) {
            std::cerr << "fatal: not a mygit repository\n";
            return false;
        }
        
        // Check if target is a branch name (refs/heads/<target> exists)
        fs::path branchPath = fs::path(repoPath) / "refs" / "heads" / target;
        if (fs::exists(branchPath)) {
            return checkoutBranch(repoPath, target);
        }
        
        // If not a branch, treat as commit hash
        return checkoutCommit(repoPath, target);
}

bool Checkout::findRepoRoot(std::string& repoPath) {
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

std::string Checkout::readCommitTree(const std::string& repoPath, const std::string& commitHash) {
    // Read commit object from .mygit/objects/<hash>
    fs::path commitPath = fs::path(repoPath) / "objects" / commitHash.substr(0, 2) / commitHash.substr(2);
    if (!fs::exists(commitPath)) {
        std::cerr << "fatal: commit object does not exist\n";
        return "";
    }
    std::ifstream commitFile(commitPath, std::ios::binary);
    if (!commitFile) {
        std::cerr << "fatal: failed to read commit object\n";
        return "";
    }
    // Read entire file
    std::string content((std::istreambuf_iterator<char>(commitFile)),
                         std::istreambuf_iterator<char>());
    
    
    // Parse the commit object to find the "tree <hash>" line
    // The commit object format is: "commit <size>\0tree <hash>\nparent <hash>\nauthor ...\ncommitter ...\n\n<message>"
    // Split by null byte to separate header and body
    size_t nullPos = content.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "fatal: commit object is malformed\n";
        return "";
    }
    
    // Body is after the null byte
    std::string body = content.substr(nullPos + 1);
    
    // Find the "tree <hash>" line in the body
    size_t treePos = body.find("tree ");
    if (treePos == std::string::npos) {
        std::cerr << "fatal: commit object does not contain tree reference\n";
        return "";
    }
    
    // Extract the tree hash
    size_t start = treePos + 5; // Skip "tree "
    size_t end = body.find('\n', start);
    if (end == std::string::npos) {
        end = body.length();
    }
    return body.substr(start, end - start);
}

bool Checkout::readTreeContents(const std::string& repoPath, const std::string& treeHash, 
                      std::map<std::string, std::string>& files) {
    // Read tree object from .mygit/objects/<hash>
    fs::path treePath = fs::path(repoPath) / "objects" / treeHash.substr(0, 2) / treeHash.substr(2);
    if (!fs::exists(treePath)) {
        std::cerr << "fatal: tree object does not exist\n";
        return false;
    }
    std::ifstream treeFile(treePath, std::ios::binary);
    if (!treeFile) {
        std::cerr << "fatal: failed to read tree object\n";
        return false;
    }
    
    // Read entire file
    std::string content((std::istreambuf_iterator<char>(treeFile)),
                         std::istreambuf_iterator<char>());
    
    // Skip tree header "tree <size>\0"
    size_t nullPos = content.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "fatal: tree object is malformed\n";
        return false;
    }
    
    // Body is after the null byte
    std::string body = content.substr(nullPos + 1);
    
    // Parse format: "100644 blob <blob-hash>\t<filename>\n"
    std::istringstream bodyStream(body);
    std::string line;
    while (std::getline(bodyStream, line)) {
        size_t tabPos = line.find('\t');
        if (tabPos == std::string::npos) {
            continue; // Skip malformed lines
        }
        std::string meta = line.substr(0, tabPos);
        std::string filename = line.substr(tabPos + 1);
        
        // Meta format: "100644 blob <blob-hash>"
        std::istringstream metaStream(meta);
        std::string mode, type, hash;
        if (!(metaStream >> mode >> type >> hash)) {
            continue; // Skip malformed meta
        }
        
        if (type != "blob") {
            continue; // We only care about blobs (files)
        }
        
        files[filename] = hash;
    }
    // For each entry:
    //   - Store filename → blob-hash in the map
    // Return true on success
    return true;
}

bool Checkout::writeFilesToWorkingDirectory(const std::map<std::string, std::string>& files, 
                                  const std::string& repoPath) {
    // For each file in the map:
    //   1. Read blob object: .mygit/objects/<blob-hash>
    fs::path objectsPath = fs::path(repoPath) / "objects";
    for (const auto& entry : files) {
        const std::string& filename = entry.first;
        const std::string& blobHash = entry.second;
        
        fs::path blobPath = objectsPath / blobHash.substr(0, 2) / blobHash.substr(2);
        if (!fs::exists(blobPath)) {
            std::cerr << "fatal: blob object does not exist for file '" << filename << "'\n";
            return false;
        }
        
        std::ifstream blobFile(blobPath, std::ios::binary);
        if (!blobFile) {
            std::cerr << "fatal: failed to read blob object for file '" << filename << "'\n";
            return false;
        }
        
        // Read entire file
        std::string content((std::istreambuf_iterator<char>(blobFile)),
                             std::istreambuf_iterator<char>());
        
        // Parse blob format: "blob <size>\0<content>"
        size_t nullPos = content.find('\0');
        if (nullPos == std::string::npos) {
            std::cerr << "fatal: blob object is malformed for file '" << filename << "'\n";
            return false;
        }
        
        std::string header = content.substr(0, nullPos);
        if (header.substr(0, 5) != "blob ") {
            std::cerr << "fatal: object is not a blob for file '" << filename << "'\n";
            return false;
        }
        
        // Get the actual file content (after null byte)
        std::string fileContent = content.substr(nullPos + 1);
        
        // Write content to working directory
        std::ofstream outFile(filename, std::ios::binary);
        if (!outFile) {
            std::cerr << "fatal: failed to write file '" << filename << "' to working directory\n";
            return false;
        }
        
        outFile.write(fileContent.c_str(), fileContent.size());
    }
  
    // TODO: Delete files that exist in working dir but NOT in tree
    // This is dangerous and should check against index/tracked files only
    // Skip for now to avoid accidentally deleting important files
    
    return true;
}

bool Checkout::checkoutBranch(const std::string& repoPath, const std::string& branchName) {
    // Read commit hash from branch reference
    std::string refPath = repoPath + "/refs/heads/" + branchName;
    std::ifstream refFile(refPath);
    if (!refFile) {
        std::cerr << "fatal: failed to read branch reference\n";
        return false;
    }
    
    std::string commitHash;
    std::getline(refFile, commitHash);
    refFile.close();
    
    // Remove trailing whitespace
    while (!commitHash.empty() && (commitHash.back() == '\n' || commitHash.back() == '\r')) {
        commitHash.pop_back();
    }
    
    if (commitHash.empty()) {
        std::cerr << "fatal: branch has no commits\n";
        return false;
    }
    
    // Get tree from commit
    std::string treeHash = readCommitTree(repoPath, commitHash);
    if (treeHash.empty()) {
        return false;
    }
    
    // Read tree contents
    std::map<std::string, std::string> files;
    if (!readTreeContents(repoPath, treeHash, files)) {
        return false;
    }
    
    // Write files to working directory
    if (!writeFilesToWorkingDirectory(files, repoPath)) {
        return false;
    }
    
    // Update index
    if (!updateIndex(repoPath, files)) {
        return false;
    }
    
    // Update HEAD to point to branch
    if (!updateHEAD(repoPath, branchName, true)) {
        return false;
    }
    
    std::cout << "Switched to branch '" << branchName << "'\n";
    return true;
}

bool Checkout::checkoutCommit(const std::string& repoPath, const std::string& commitHash) {
    // Get tree from commit
    std::string treeHash = readCommitTree(repoPath, commitHash);
    if (treeHash.empty()) {
        return false;
    }
    
    // Read tree contents
    std::map<std::string, std::string> files;
    if (!readTreeContents(repoPath, treeHash, files)) {
        return false;
    }
    
    // Write files to working directory
    if (!writeFilesToWorkingDirectory(files, repoPath)) {
        return false;
    }
    
    // Update index
    if (!updateIndex(repoPath, files)) {
        return false;
    }
    
    // Update HEAD to point directly to commit (detached HEAD)
    if (!updateHEAD(repoPath, commitHash, false)) {
        return false;
    }
    
    std::cout << "HEAD is now at " << commitHash.substr(0, 7) << "\n";
    return true;
}

bool Checkout::updateIndex(const std::string& repoPath, const std::map<std::string, std::string>& files) {
    std::string indexPath = repoPath + "/index";
    
    std::ofstream indexFile(indexPath);
    if (!indexFile) {
        std::cerr << "fatal: failed to update index\n";
        return false;
    }
    
    // Write index entries: "mode hash filename\n"
    for (const auto& entry : files) {
        indexFile << "100644 " << entry.second << " " << entry.first << "\n";
    }
    
    return true;
}

bool Checkout::updateHEAD(const std::string& repoPath, const std::string& ref, bool isBranch) {
    std::string headPath = repoPath + "/HEAD";
    
    std::ofstream headFile(headPath);
    if (!headFile) {
        std::cerr << "fatal: failed to update HEAD\n";
        return false;
    }
    
    if (isBranch) {
        // Symbolic reference to branch
        headFile << "ref: refs/heads/" << ref << "\n";
    } else {
        // Direct commit hash (detached HEAD)
        headFile << ref << "\n";
    }
    
    return true;
}