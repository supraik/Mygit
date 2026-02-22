#include <iostream>
#include <sstream>
#include <map>
#include <filesystem>
#include <fstream>



namespace fs = std::filesystem;

class Checkout {
private:
    bool findRepoRoot(std::string& repoPath);
    bool checkoutBranch(const std::string& repoPath, const std::string& branchName);
    bool checkoutCommit(const std::string& repoPath, const std::string& commitHash);
    std::string readCommitTree(const std::string& repoPath, const std::string& commitHash);
    bool readTreeContents(const std::string& repoPath, const std::string& treeHash, 
                          std::map<std::string, std::string>& files);
    bool writeFilesToWorkingDirectory(const std::map<std::string, std::string>& files, 
                                      const std::string& repoPath);
    bool updateIndex(const std::string& repoPath, 
                     const std::map<std::string, std::string>& files);
    bool updateHEAD(const std::string& repoPath, const std::string& ref, bool isBranch);
    
public:
    bool execute(const std::string& target);
};