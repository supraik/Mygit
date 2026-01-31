#ifndef ADD_H
#define ADD_H

#include <string>
#include <vector>

class Add {
public:
    bool execute(const std::string& filePath);
    
private:
    bool findRepoRoot(std::string& repoPath);
    std::vector<char> readFile(const std::string& filePath);
    bool storeObject(const std::string& repoPath, const std::string& hash, const std::vector<char>& content);
    bool updateIndex(const std::string& repoPath, const std::string& hash, const std::string& filePath);
};

#endif
