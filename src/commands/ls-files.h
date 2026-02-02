#ifndef LS_FILES_H
#define LS_FILES_H

#include <string>
#include <map>

class LsFiles {
public:
    bool execute();
    
private:
    bool findRepoRoot(std::string& repoPath);
    std::map<std::string, std::string> readIndex(const std::string& repoPath);
};

#endif
