#ifndef STATUS_H
#define STATUS_H

#include <string>
#include <map>
#include <vector>

class Status {
public:
    bool execute();
    
private:
    bool findRepoRoot(std::string& repoPath);
    std::map<std::string, std::string> readIndex(const std::string& repoPath);
    std::vector<std::string> getWorkingDirFiles();
    std::string computeFileHash(const std::string& filePath);
};

#endif
