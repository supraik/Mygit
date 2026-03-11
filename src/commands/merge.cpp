#include "merge.h"
#include "../utils/hash.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <queue>
#include <ctime>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

bool Merge::execute(const std::string& branchName) {
    std::string repoPath;
    if (!findRepoRoot(repoPath)) {
        std::cerr << "fatal: not a mygit repository\n";
        return false;
    }

    // Get current branch and commit
    std::string currentBranch = getCurrentBranch(repoPath);
    std::string currentCommit = getCurrentCommit(repoPath);
    
    if (currentCommit.empty()) {
        std::cerr << "fatal: no commits yet\n";
        return false;
    }

    // Check if we're in detached HEAD state
    if (currentBranch.empty()) {
        std::cerr << "fatal: cannot merge in detached HEAD state\n";
        return false;
    }

    // Get target branch commit
    std::string targetCommit = getBranchCommit(repoPath, branchName);
    if (targetCommit.empty()) {
        std::cerr << "fatal: branch '" << branchName << "' not found\n";
        return false;
    }

    // Check if already up-to-date
    if (currentCommit == targetCommit) {
        std::cout << "Already up to date.\n";
        return true;
    }

    // Find merge base (common ancestor)
    std::string mergeBase = findMergeBase(repoPath, currentCommit, targetCommit);
    
    if (mergeBase.empty()) {
        std::cerr << "fatal: no common ancestor found\n";
        return false;
    }

    // Check if it's a fast-forward merge
    if (mergeBase == currentCommit) {
        std::cout << "Fast-forward merge\n";
        return performFastForward(repoPath, targetCommit);
    }

    // Check if target is already merged (current is ahead)
    if (mergeBase == targetCommit) {
        std::cout << "Already up to date.\n";
        return true;
    }

    // Perform three-way merge
    std::cout << "Performing three-way merge...\n";
    return performThreeWayMerge(repoPath, currentCommit, targetCommit, mergeBase, branchName);
}

bool Merge::findRepoRoot(std::string& repoPath) {
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

std::string Merge::getCurrentCommit(const std::string& repoPath) {
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
    
    // Check if it's a symbolic reference
    if (line.substr(0, 5) == "ref: ") {
        std::string refPath = line.substr(5);
        std::string refFilePath = repoPath + "/" + refPath;
        
        if (!fs::exists(refFilePath)) {
            return "";
        }
        
        std::ifstream refFile(refFilePath);
        if (!refFile) {
            return "";
        }
        
        std::string commitHash;
        std::getline(refFile, commitHash);
        
        while (!commitHash.empty() && (commitHash.back() == '\n' || commitHash.back() == '\r')) {
            commitHash.pop_back();
        }
        
        return commitHash;
    } else {
        return line;
    }
}

std::string Merge::getCurrentBranch(const std::string& repoPath) {
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
    
    // Check if it's a symbolic reference
    if (line.substr(0, 5) == "ref: ") {
        std::string refPath = line.substr(5);
        // Extract branch name from refs/heads/<branch>
        if (refPath.substr(0, 11) == "refs/heads/") {
            return refPath.substr(11);
        }
    }
    
    return ""; // Detached HEAD
}

std::string Merge::getBranchCommit(const std::string& repoPath, const std::string& branchName) {
    std::string branchPath = repoPath + "/refs/heads/" + branchName;
    
    if (!fs::exists(branchPath)) {
        return "";
    }
    
    std::ifstream branchFile(branchPath);
    if (!branchFile) {
        return "";
    }
    
    std::string commitHash;
    std::getline(branchFile, commitHash);
    
    while (!commitHash.empty() && (commitHash.back() == '\n' || commitHash.back() == '\r')) {
        commitHash.pop_back();
    }
    
    return commitHash;
}

std::string Merge::getCommitParent(const std::string& repoPath, const std::string& commitHash) {
    fs::path commitPath = fs::path(repoPath) / "objects" / commitHash.substr(0, 2) / commitHash.substr(2);
    if (!fs::exists(commitPath)) {
        return "";
    }
    
    std::ifstream commitFile(commitPath, std::ios::binary);
    if (!commitFile) {
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(commitFile)),
                         std::istreambuf_iterator<char>());
    
    size_t nullPos = content.find('\0');
    if (nullPos == std::string::npos) {
        return "";
    }
    
    std::string body = content.substr(nullPos + 1);
    
    // Find "parent <hash>" line
    size_t parentPos = body.find("parent ");
    if (parentPos == std::string::npos) {
        return ""; // No parent (root commit)
    }
    
    size_t start = parentPos + 7;
    size_t end = body.find('\n', start);
    if (end == std::string::npos) {
        end = body.length();
    }
    
    return body.substr(start, end - start);
}

std::string Merge::findMergeBase(const std::string& repoPath, const std::string& commit1, const std::string& commit2) {
    // BFS to find common ancestor
    std::set<std::string> ancestors1;
    std::queue<std::string> queue1;
    queue1.push(commit1);
    
    // Collect all ancestors of commit1
    while (!queue1.empty()) {
        std::string current = queue1.front();
        queue1.pop();
        
        if (ancestors1.count(current)) {
            continue;
        }
        
        ancestors1.insert(current);
        
        std::string parent = getCommitParent(repoPath, current);
        if (!parent.empty()) {
            queue1.push(parent);
        }
    }
    
    // BFS from commit2 to find first common ancestor
    std::set<std::string> visited;
    std::queue<std::string> queue2;
    queue2.push(commit2);
    
    while (!queue2.empty()) {
        std::string current = queue2.front();
        queue2.pop();
        
        if (visited.count(current)) {
            continue;
        }
        
        visited.insert(current);
        
        if (ancestors1.count(current)) {
            return current; // Found common ancestor
        }
        
        std::string parent = getCommitParent(repoPath, current);
        if (!parent.empty()) {
            queue2.push(parent);
        }
    }
    
    return ""; // No common ancestor
}

std::string Merge::readCommitTree(const std::string& repoPath, const std::string& commitHash) {
    fs::path commitPath = fs::path(repoPath) / "objects" / commitHash.substr(0, 2) / commitHash.substr(2);
    if (!fs::exists(commitPath)) {
        return "";
    }
    
    std::ifstream commitFile(commitPath, std::ios::binary);
    if (!commitFile) {
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(commitFile)),
                         std::istreambuf_iterator<char>());
    
    size_t nullPos = content.find('\0');
    if (nullPos == std::string::npos) {
        return "";
    }
    
    std::string body = content.substr(nullPos + 1);
    
    size_t treePos = body.find("tree ");
    if (treePos == std::string::npos) {
        return "";
    }
    
    size_t start = treePos + 5;
    size_t end = body.find('\n', start);
    if (end == std::string::npos) {
        end = body.length();
    }
    
    return body.substr(start, end - start);
}

bool Merge::readTreeContents(const std::string& repoPath, const std::string& treeHash, 
                              std::map<std::string, std::string>& files) {
    fs::path treePath = fs::path(repoPath) / "objects" / treeHash.substr(0, 2) / treeHash.substr(2);
    if (!fs::exists(treePath)) {
        return false;
    }
    
    std::ifstream treeFile(treePath, std::ios::binary);
    if (!treeFile) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(treeFile)),
                         std::istreambuf_iterator<char>());
    
    size_t nullPos = content.find('\0');
    if (nullPos == std::string::npos) {
        return false;
    }
    
    std::string body = content.substr(nullPos + 1);
    
    std::istringstream bodyStream(body);
    std::string line;
    while (std::getline(bodyStream, line)) {
        size_t tabPos = line.find('\t');
        if (tabPos == std::string::npos) {
            continue;
        }
        
        std::string meta = line.substr(0, tabPos);
        std::string filename = line.substr(tabPos + 1);
        
        std::istringstream metaStream(meta);
        std::string mode, type, hash;
        if (!(metaStream >> mode >> type >> hash)) {
            continue;
        }
        
        if (type != "blob") {
            continue;
        }
        
        files[filename] = hash;
    }
    
    return true;
}

bool Merge::readBlobContent(const std::string& repoPath, const std::string& blobHash, std::string& content) {
    fs::path blobPath = fs::path(repoPath) / "objects" / blobHash.substr(0, 2) / blobHash.substr(2);
    if (!fs::exists(blobPath)) {
        return false;
    }
    
    std::ifstream blobFile(blobPath, std::ios::binary);
    if (!blobFile) {
        return false;
    }
    
    std::string data((std::istreambuf_iterator<char>(blobFile)),
                      std::istreambuf_iterator<char>());
    
    // Skip blob header "blob <size>\0"
    size_t nullPos = data.find('\0');
    if (nullPos == std::string::npos) {
        return false;
    }
    
    content = data.substr(nullPos + 1);
    return true;
}

bool Merge::performFastForward(const std::string& repoPath, const std::string& targetCommit) {
    // Get target tree
    std::string targetTree = readCommitTree(repoPath, targetCommit);
    if (targetTree.empty()) {
        std::cerr << "fatal: failed to read target commit tree\n";
        return false;
    }
    
    // Read target files
    std::map<std::string, std::string> targetFiles;
    if (!readTreeContents(repoPath, targetTree, targetFiles)) {
        std::cerr << "fatal: failed to read target tree contents\n";
        return false;
    }
    
    // Get workspace root (parent of .mygit)
    fs::path workspaceRoot = fs::path(repoPath).parent_path();
    
    // Clear working directory (except .mygit)
    for (const auto& entry : fs::directory_iterator(workspaceRoot)) {
        if (entry.path().filename() != ".mygit") {
            if (fs::is_directory(entry.path())) {
                fs::remove_all(entry.path());
            } else {
                fs::remove(entry.path());
            }
        }
    }
    
    // Write target files to workspace
    for (const auto& [filename, blobHash] : targetFiles) {
        std::string content;
        if (!readBlobContent(repoPath, blobHash, content)) {
            std::cerr << "warning: failed to read blob for " << filename << "\n";
            continue;
        }
        
        fs::path filePath = workspaceRoot / filename;
        if (!writeFileToWorkspace(filePath.string(), content)) {
            std::cerr << "warning: failed to write " << filename << "\n";
        }
    }
    
    // Update index
    if (!updateIndex(repoPath, targetFiles)) {
        std::cerr << "warning: failed to update index\n";
    }
    
    // Update HEAD
    if (!updateHead(repoPath, targetCommit)) {
        std::cerr << "fatal: failed to update HEAD\n";
        return false;
    }
    
    std::cout << "Updated to commit " << targetCommit << "\n";
    return true;
}

bool Merge::performThreeWayMerge(const std::string& repoPath, const std::string& currentCommit,
                                 const std::string& targetCommit, const std::string& baseCommit,
                                 const std::string& targetBranch) {
    // Read trees for base, current, and target
    std::string baseTree = readCommitTree(repoPath, baseCommit);
    std::string currentTree = readCommitTree(repoPath, currentCommit);
    std::string targetTree = readCommitTree(repoPath, targetCommit);
    
    if (baseTree.empty() || currentTree.empty() || targetTree.empty()) {
        std::cerr << "fatal: failed to read commit trees\n";
        return false;
    }
    
    std::map<std::string, std::string> baseFiles, currentFiles, targetFiles;
    readTreeContents(repoPath, baseTree, baseFiles);
    readTreeContents(repoPath, currentTree, currentFiles);
    readTreeContents(repoPath, targetTree, targetFiles);
    
    // Merged files map
    std::map<std::string, std::string> mergedFiles = currentFiles;
    bool hasConflicts = false;
    std::vector<std::string> conflictedFiles;
    
    // Collect all filenames
    std::set<std::string> allFiles;
    for (const auto& [name, _] : baseFiles) allFiles.insert(name);
    for (const auto& [name, _] : currentFiles) allFiles.insert(name);
    for (const auto& [name, _] : targetFiles) allFiles.insert(name);
    
    fs::path workspaceRoot = fs::path(repoPath).parent_path();
    
    // Three-way merge logic
    for (const auto& filename : allFiles) {
        bool inBase = baseFiles.count(filename) > 0;
        bool inCurrent = currentFiles.count(filename) > 0;
        bool inTarget = targetFiles.count(filename) > 0;
        
        if (!inBase && !inCurrent && inTarget) {
            // File added in target only
            mergedFiles[filename] = targetFiles[filename];
            
        } else if (!inBase && inCurrent && !inTarget) {
            // File added in current only (keep it)
            
        } else if (!inBase && inCurrent && inTarget) {
            // File added in both branches
            if (currentFiles[filename] == targetFiles[filename]) {
                mergedFiles[filename] = currentFiles[filename];
            } else {
                // Conflict: both added differently — write conflict markers
                std::string currentContent, targetContent;
                readBlobContent(repoPath, currentFiles[filename], currentContent);
                readBlobContent(repoPath, targetFiles[filename], targetContent);
                
                bool fileConflict = false;
                std::string merged = mergeFileContents("", currentContent, targetContent, targetBranch, fileConflict);
                
                std::string newHash = storeBlobObject(repoPath, merged);
                mergedFiles[filename] = newHash;
                
                if (fileConflict) {
                    hasConflicts = true;
                    conflictedFiles.push_back(filename);
                    std::cerr << "CONFLICT (add/add): Merge conflict in " << filename << "\n";
                }
            }
            
        } else if (inBase && !inCurrent && !inTarget) {
            // File deleted in both branches
            mergedFiles.erase(filename);
            
        } else if (inBase && !inCurrent && inTarget) {
            // File deleted in current, check if modified in target
            if (baseFiles[filename] == targetFiles[filename]) {
                mergedFiles.erase(filename);
            } else {
                std::cerr << "CONFLICT (modify/delete): " << filename << " deleted in HEAD but modified in " << targetBranch << "\n";
                // Keep the target version with conflict marker
                std::string targetContent;
                readBlobContent(repoPath, targetFiles[filename], targetContent);
                
                std::string conflictContent = "<<<<<<< HEAD\n=======\n" + targetContent + "\n>>>>>>> " + targetBranch + "\n";
                std::string newHash = storeBlobObject(repoPath, conflictContent);
                mergedFiles[filename] = newHash;
                hasConflicts = true;
                conflictedFiles.push_back(filename);
            }
            
        } else if (inBase && inCurrent && !inTarget) {
            // File deleted in target, check if modified in current
            if (baseFiles[filename] == currentFiles[filename]) {
                mergedFiles.erase(filename);
            } else {
                std::cerr << "CONFLICT (modify/delete): " << filename << " modified in HEAD but deleted in " << targetBranch << "\n";
                std::string currentContent;
                readBlobContent(repoPath, currentFiles[filename], currentContent);
                
                std::string conflictContent = "<<<<<<< HEAD\n" + currentContent + "\n=======\n>>>>>>> " + targetBranch + "\n";
                std::string newHash = storeBlobObject(repoPath, conflictContent);
                mergedFiles[filename] = newHash;
                hasConflicts = true;
                conflictedFiles.push_back(filename);
            }
            
        } else if (inBase && inCurrent && inTarget) {
            // File exists in all three
            std::string baseHash = baseFiles[filename];
            std::string currentHash = currentFiles[filename];
            std::string targetHash = targetFiles[filename];
            
            if (currentHash == targetHash) {
                mergedFiles[filename] = currentHash;
            } else if (baseHash == currentHash) {
                mergedFiles[filename] = targetHash;
            } else if (baseHash == targetHash) {
                mergedFiles[filename] = currentHash;
            } else {
                // Modified in both branches differently — line-level merge with conflict markers
                std::string baseContent, currentContent, targetContent;
                readBlobContent(repoPath, baseHash, baseContent);
                readBlobContent(repoPath, currentHash, currentContent);
                readBlobContent(repoPath, targetHash, targetContent);
                
                bool fileConflict = false;
                std::string merged = mergeFileContents(baseContent, currentContent, targetContent, targetBranch, fileConflict);
                
                std::string newHash = storeBlobObject(repoPath, merged);
                mergedFiles[filename] = newHash;
                
                if (fileConflict) {
                    hasConflicts = true;
                    conflictedFiles.push_back(filename);
                    std::cerr << "CONFLICT (content): Merge conflict in " << filename << "\n";
                }
            }
        }
    }
    
    // Write all merged files to working directory
    for (const auto& entry : fs::directory_iterator(workspaceRoot)) {
        if (entry.path().filename() != ".mygit") {
            if (fs::is_directory(entry.path())) {
                fs::remove_all(entry.path());
            } else {
                fs::remove(entry.path());
            }
        }
    }
    
    for (const auto& [filename, blobHash] : mergedFiles) {
        std::string content;
        if (!readBlobContent(repoPath, blobHash, content)) {
            std::cerr << "warning: failed to read blob for " << filename << "\n";
            continue;
        }
        
        fs::path filePath = workspaceRoot / filename;
        if (!writeFileToWorkspace(filePath.string(), content)) {
            std::cerr << "warning: failed to write " << filename << "\n";
        }
    }
    
    // Update index
    if (!updateIndex(repoPath, mergedFiles)) {
        std::cerr << "warning: failed to update index\n";
    }
    
    if (hasConflicts) {
        // Write merge state files so commit knows about the merge
        writeMergeState(repoPath, targetCommit, targetBranch, conflictedFiles);
        std::cerr << "Automatic merge failed; fix conflicts and then commit the result.\n";
        std::cerr << "Use 'mygit resolve' to check conflict status.\n";
        return false;
    }
    
    // No conflicts — create merge commit directly
    std::string newTreeHash = createTreeObject(repoPath, mergedFiles);
    if (newTreeHash.empty()) {
        std::cerr << "fatal: failed to create tree object\n";
        return false;
    }
    
    std::string message = "Merge branch '" + targetBranch + "'";
    if (!createMergeCommit(repoPath, newTreeHash, currentCommit, targetCommit, message)) {
        std::cerr << "fatal: failed to create merge commit\n";
        return false;
    }
    
    std::cout << "Merge successful\n";
    return true;
}

std::string Merge::createTreeObject(const std::string& repoPath, const std::map<std::string, std::string>& files) {
    std::string treeContent;
    
    for (const auto& [filepath, hash] : files) {
        treeContent += "100644 blob " + hash + "\t" + filepath + "\n";
    }
    
    std::string header = "tree " + std::to_string(treeContent.size()) + '\0';
    std::string treeData = header + treeContent;
    
    std::string treeHash = Hash::sha1(treeData);
    
    if (!storeObject(repoPath, treeHash, treeData)) {
        return "";
    }
    
    return treeHash;
}

bool Merge::createMergeCommit(const std::string& repoPath, const std::string& treeHash,
                              const std::string& parent1, const std::string& parent2,
                              const std::string& message) {
    std::time_t now = std::time(nullptr);
    
    std::ostringstream commitContent;
    commitContent << "tree " << treeHash << "\n";
    commitContent << "parent " << parent1 << "\n";
    commitContent << "parent " << parent2 << "\n";
    commitContent << "author User <user@example.com> " << now << " +0000\n";
    commitContent << "committer User <user@example.com> " << now << " +0000\n";
    commitContent << "\n";
    commitContent << message << "\n";
    
    std::string content = commitContent.str();
    std::string header = "commit " + std::to_string(content.size()) + '\0';
    std::string commitData = header + content;
    
    std::string commitHash = Hash::sha1(commitData);
    
    if (!storeObject(repoPath, commitHash, commitData)) {
        return false;
    }
    
    if (!updateHead(repoPath, commitHash)) {
        return false;
    }
    
    std::cout << "Created merge commit: " << commitHash << "\n";
    return true;
}

bool Merge::storeObject(const std::string& repoPath, const std::string& hash, const std::string& content) {
    std::string dirPath = repoPath + "/objects/" + hash.substr(0, 2);
    std::string objPath = dirPath + "/" + hash.substr(2);
    
    try {
        fs::create_directories(dirPath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory: " << e.what() << "\n";
        return false;
    }
    
    std::ofstream objFile(objPath, std::ios::binary);
    if (!objFile) {
        return false;
    }
    
    objFile.write(content.c_str(), content.size());
    return objFile.good();
}

bool Merge::updateHead(const std::string& repoPath, const std::string& commitHash) {
    std::string headPath = repoPath + "/HEAD";
    
    std::ifstream headFile(headPath);
    if (!headFile) {
        return false;
    }
    
    std::string line;
    std::getline(headFile, line);
    headFile.close();
    
    // Remove trailing newlines
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }
    
    if (line.substr(0, 5) == "ref: ") {
        std::string refPath = line.substr(5);
        std::string refFilePath = repoPath + "/" + refPath;
        
        fs::path refFilePathObj(refFilePath);
        try {
            fs::create_directories(refFilePathObj.parent_path());
        } catch (const std::exception& e) {
            std::cerr << "Failed to create ref directory: " << e.what() << "\n";
            return false;
        }
        
        std::ofstream refFile(refFilePath);
        if (!refFile) {
            return false;
        }
        
        refFile << commitHash << "\n";
        return true;
    } else {
        std::ofstream headFileOut(headPath);
        if (!headFileOut) {
            return false;
        }
        
        headFileOut << commitHash << "\n";
        return true;
    }
}

bool Merge::writeFileToWorkspace(const std::string& filepath, const std::string& content) {
    fs::path filePath(filepath);
    
    try {
        fs::create_directories(filePath.parent_path());
    } catch (const std::exception&) {
        return false;
    }
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        return false;
    }
    
    file.write(content.c_str(), content.size());
    return file.good();
}

bool Merge::updateIndex(const std::string& repoPath, const std::map<std::string, std::string>& files) {
    std::string indexPath = repoPath + "/index";
    
    std::ofstream indexFile(indexPath);
    if (!indexFile) {
        return false;
    }
    
    for (const auto& [filepath, hash] : files) {
        indexFile << "100644 " << hash << " " << filepath << "\n";
    }
    
    return indexFile.good();
}

// ===== Conflict resolution helpers =====

std::vector<std::string> Merge::splitLines(const std::string& content) {
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::string Merge::mergeFileContents(const std::string& baseContent, const std::string& currentContent,
                                      const std::string& targetContent, const std::string& targetBranch,
                                      bool& hasConflict) {
    hasConflict = false;
    
    std::vector<std::string> baseLines = splitLines(baseContent);
    std::vector<std::string> currentLines = splitLines(currentContent);
    std::vector<std::string> targetLines = splitLines(targetContent);
    
    std::ostringstream result;
    
    size_t maxLines = std::max({baseLines.size(), currentLines.size(), targetLines.size()});
    
    size_t bi = 0, ci = 0, ti = 0;
    
    // Simple line-by-line three-way merge
    while (ci < currentLines.size() || ti < targetLines.size()) {
        std::string baseLine = (bi < baseLines.size()) ? baseLines[bi] : "";
        std::string currentLine = (ci < currentLines.size()) ? currentLines[ci] : "";
        std::string targetLine = (ti < targetLines.size()) ? targetLines[ti] : "";
        
        bool baseExists = (bi < baseLines.size());
        bool currentExists = (ci < currentLines.size());
        bool targetExists = (ti < targetLines.size());
        
        if (currentExists && targetExists && currentLine == targetLine) {
            // Both same — take it
            result << currentLine << "\n";
            bi++; ci++; ti++;
        } else if (baseExists && currentExists && baseLine == currentLine && targetExists) {
            // Current unchanged, target changed — take target
            result << targetLine << "\n";
            bi++; ci++; ti++;
        } else if (baseExists && targetExists && baseLine == targetLine && currentExists) {
            // Target unchanged, current changed — take current
            result << currentLine << "\n";
            bi++; ci++; ti++;
        } else {
            // Conflict — collect the differing region
            hasConflict = true;
            
            // Gather current hunk
            std::vector<std::string> currentHunk;
            std::vector<std::string> targetHunk;
            
            // Simple approach: take lines until we find a common line again
            size_t lookAhead = std::min((size_t)10, maxLines);
            size_t syncPoint = std::string::npos;
            
            // Try to find a sync point where current and target agree again
            for (size_t ahead = 1; ahead <= lookAhead; ahead++) {
                if (ci + ahead < currentLines.size() && ti + ahead < targetLines.size() &&
                    currentLines[ci + ahead] == targetLines[ti + ahead]) {
                    syncPoint = ahead;
                    break;
                }
            }
            
            if (syncPoint != std::string::npos) {
                for (size_t k = 0; k < syncPoint; k++) {
                    if (ci < currentLines.size()) currentHunk.push_back(currentLines[ci++]);
                    if (ti < targetLines.size()) targetHunk.push_back(targetLines[ti++]);
                    if (bi < baseLines.size()) bi++;
                }
            } else {
                // No sync found — take all remaining as one conflict block
                while (ci < currentLines.size()) {
                    currentHunk.push_back(currentLines[ci++]);
                }
                while (ti < targetLines.size()) {
                    targetHunk.push_back(targetLines[ti++]);
                }
                bi = baseLines.size();
            }
            
            // Write conflict markers
            result << "<<<<<<< HEAD\n";
            for (const auto& l : currentHunk) {
                result << l << "\n";
            }
            result << "=======\n";
            for (const auto& l : targetHunk) {
                result << l << "\n";
            }
            result << ">>>>>>> " << targetBranch << "\n";
        }
    }
    
    return result.str();
}

std::string Merge::storeBlobObject(const std::string& repoPath, const std::string& content) {
    std::string header = "blob " + std::to_string(content.size()) + '\0';
    std::string blobData = header + content;
    
    std::string hash = Hash::sha1(blobData);
    
    if (!storeObject(repoPath, hash, blobData)) {
        return "";
    }
    
    return hash;
}

bool Merge::writeMergeState(const std::string& repoPath, const std::string& targetCommit,
                             const std::string& targetBranch, const std::vector<std::string>& conflictedFiles) {
    // Write MERGE_HEAD (records the commit being merged in)
    std::string mergeHeadPath = repoPath + "/MERGE_HEAD";
    std::ofstream mergeHeadFile(mergeHeadPath);
    if (!mergeHeadFile) {
        std::cerr << "warning: failed to write MERGE_HEAD\n";
        return false;
    }
    mergeHeadFile << targetCommit << "\n";
    mergeHeadFile.close();
    
    // Write MERGE_MSG (default commit message)
    std::string mergeMsgPath = repoPath + "/MERGE_MSG";
    std::ofstream mergeMsgFile(mergeMsgPath);
    if (!mergeMsgFile) {
        std::cerr << "warning: failed to write MERGE_MSG\n";
        return false;
    }
    mergeMsgFile << "Merge branch '" << targetBranch << "'\n\n";
    mergeMsgFile << "# Conflicts:\n";
    for (const auto& f : conflictedFiles) {
        mergeMsgFile << "#\t" << f << "\n";
    }
    mergeMsgFile.close();
    
    // Write MERGE_CONFLICTS (list of conflicted files)
    std::string conflictsPath = repoPath + "/MERGE_CONFLICTS";
    std::ofstream conflictsFile(conflictsPath);
    if (!conflictsFile) {
        std::cerr << "warning: failed to write MERGE_CONFLICTS\n";
        return false;
    }
    for (const auto& f : conflictedFiles) {
        conflictsFile << f << "\n";
    }
    conflictsFile.close();
    
    return true;
}

bool Merge::cleanMergeState(const std::string& repoPath) {
    fs::remove(repoPath + "/MERGE_HEAD");
    fs::remove(repoPath + "/MERGE_MSG");
    fs::remove(repoPath + "/MERGE_CONFLICTS");
    return true;
}
