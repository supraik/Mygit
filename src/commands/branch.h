#ifndef BRANCH_H
#define BRANCH_H

#include <iostream>
#include <sstream>
#include <map>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class Branch {
private:
    bool findRepoRoot(std::string& repoPath);
    // get currrent head commit hash
    std::string getCurrentCommit(const std::string& repoPath);
    // get current branch name from HEAD
    std::string getCurrentBranch(const std::string& repoPath);
    //create new file at .mygit/refs/heads/<branchName> with content of current head commit hash
    bool createBranch(const std::string& repoPath, const std::string& branchName, const std::string& commitHash);
    // delete a branch
    bool deleteBranch(const std::string& repoPath, const std::string& branchName);
    
public:
    bool execute(const std::string& target, const std::string& flag = "");
};

#endif