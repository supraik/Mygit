#ifndef RM_H
#define RM_H

#include <string>

class Rm {
public:
    bool execute(const std::string& filePath);
    
private:
    bool findRepoRoot(std::string& repoPath);
    bool updateIndex(const std::string& repoPath, const std::string& filePath);
};

#endif
