#ifndef LOG_H
#define LOG_H

#include <string>
#include <vector>

struct CommitInfo {
    std::string hash;
    std::string tree;
    std::string parent;
    std::string author;
    std::string committer;
    std::string message;
    std::string timestamp;
};

class Log {
private:
    bool findRepoRoot(std::string& repoPath);
    std::string getCurrentCommit(const std::string& repoPath);
    bool readCommitObject(const std::string& repoPath, const std::string& commitHash, CommitInfo& info);
    std::string readObjectContent(const std::string& repoPath, const std::string& hash);
    void displayCommit(const CommitInfo& info);

public:
    bool execute();
};

#endif
