#include<map>
#include<string>

#ifndef COMMIT_H
#define COMMIT_H

#include <string>
#include <vector>
#include <map>

class Commit {
private:
bool findRepoRoot(std::string& repoPath);  // Same pattern as Add
std::map<std::string, std::string> readIndex(const std::string& repoPath);  // Returns hash->path map
std::string createTreeObject(const std::string& repoPath, const std::map<std::string, std::string>& index);  // Returns tree hash
std::string createCommitObject(const std::string& repoPath, const std::string& treeHash, const std::string& parent, const std::string& message);  // Returns commit hash
std::string getParentCommit(const std::string& repoPath);  // Returns parent commit hash (or empty)
bool storeObject(const std::string& repoPath, const std::string& hash, const std::string& content);  // Stores object
bool updateHead(const std::string& repoPath, const std::string& commitHash);  // Updates HEAD reference

public:
bool execute(const std::string& message);  // Takes commit message, returns success
};

#endif