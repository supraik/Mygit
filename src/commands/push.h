#ifndef PUSH_H
#define PUSH_H

#include <string>
#include <vector>

class Push {
public:
    bool execute(const std::string& remoteName, const std::string& branchName);
    
private:
    std::string getRemoteUrl(const std::string& remoteName);
    std::string getCurrentBranch();
    std::string getBranchHash(const std::string& branchName);
    std::vector<std::string> getCommitHistory(const std::string& commitHash);
    std::vector<std::string> getAllReferencedObjects(const std::string& commitHash);
    bool sendObject(const std::string& url, const std::string& hash);
    bool updateRemoteRef(const std::string& url, const std::string& refName, const std::string& hash);
    std::string readObject(const std::string& hash);
    std::string httpPost(const std::string& url, const std::string& data);
};

#endif // PUSH_H
