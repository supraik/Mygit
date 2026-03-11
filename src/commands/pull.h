#ifndef PULL_H
#define PULL_H

#include <string>
#include <vector>
#include <map>

class Pull {
public:
    bool execute(const std::string& remoteName, const std::string& branchName);
    
private:
    std::string getRemoteUrl(const std::string& remoteName);
    std::string getCurrentBranch();
    std::map<std::string, std::string> fetchRemoteRefs(const std::string& url);
    bool fetchObject(const std::string& url, const std::string& hash);
    bool updateLocalRef(const std::string& branchName, const std::string& hash);
    std::string httpGet(const std::string& url);
    bool httpGetToFile(const std::string& url, const std::string& filePath);
    bool objectExists(const std::string& hash);
    std::string readObject(const std::string& hash);
};

#endif // PULL_H
