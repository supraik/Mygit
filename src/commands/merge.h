#ifndef MERGE_H
#define MERGE_H

#include <string>
#include <map>
#include <vector>

class Merge {
private:
    bool findRepoRoot(std::string& repoPath);
    std::string getCurrentCommit(const std::string& repoPath);
    std::string getCurrentBranch(const std::string& repoPath);
    std::string getBranchCommit(const std::string& repoPath, const std::string& branchName);
    std::string findMergeBase(const std::string& repoPath, const std::string& commit1, const std::string& commit2);
    std::string getCommitParent(const std::string& repoPath, const std::string& commitHash);
    std::string readCommitTree(const std::string& repoPath, const std::string& commitHash);
    bool readTreeContents(const std::string& repoPath, const std::string& treeHash, std::map<std::string, std::string>& files);
    bool readBlobContent(const std::string& repoPath, const std::string& blobHash, std::string& content);
    bool performFastForward(const std::string& repoPath, const std::string& targetCommit);
    bool performThreeWayMerge(const std::string& repoPath, const std::string& currentCommit, 
                              const std::string& targetCommit, const std::string& baseCommit,
                              const std::string& targetBranch);
    bool createMergeCommit(const std::string& repoPath, const std::string& treeHash, 
                           const std::string& parent1, const std::string& parent2, 
                           const std::string& message);
    std::string createTreeObject(const std::string& repoPath, const std::map<std::string, std::string>& files);
    bool storeObject(const std::string& repoPath, const std::string& hash, const std::string& content);
    bool updateHead(const std::string& repoPath, const std::string& commitHash);
    bool writeFileToWorkspace(const std::string& filepath, const std::string& content);
    bool updateIndex(const std::string& repoPath, const std::map<std::string, std::string>& files);

    // Conflict resolution support
    std::string mergeFileContents(const std::string& baseContent, const std::string& currentContent,
                                  const std::string& targetContent, const std::string& targetBranch,
                                  bool& hasConflict);
    std::vector<std::string> splitLines(const std::string& content);
    std::string storeBlobObject(const std::string& repoPath, const std::string& content);
    bool writeMergeState(const std::string& repoPath, const std::string& targetCommit,
                         const std::string& targetBranch, const std::vector<std::string>& conflictedFiles);
    bool cleanMergeState(const std::string& repoPath);

public:
    bool execute(const std::string& branchName);
};

#endif
