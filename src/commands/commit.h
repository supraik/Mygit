#include<map>
#include<string>

#ifndef COMMIT_H
#define COMMIT_H

#include <string>
#include <vector>
#include <map>

class Commit {
private:
bool findRepoRoot(std::string& repoPath);
std::map<std::string, std::string> readIndex(const std::string& repoPath);
std::string createTreeObject(const std::string& repoPath, const std::map<std::string, std::string>& index);
std::string createCommitObject(const std::string& repoPath, const std::string& treeHash, const std::string& parent, const std::string& message);
std::string createMergeCommitObject(const std::string& repoPath, const std::string& treeHash, const std::string& parent1, const std::string& parent2, const std::string& message);
std::string getParentCommit(const std::string& repoPath);
bool storeObject(const std::string& repoPath, const std::string& hash, const std::string& content);
bool updateHead(const std::string& repoPath, const std::string& commitHash);

public:
bool execute(const std::string& message);
};

#endif