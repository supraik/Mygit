#ifndef PUSH_H
#define PUSH_H

#include <string>
#include <vector>
#include <windows.h>
#include <winhttp.h>

class Push {
public:
    bool execute(const std::string& remoteName, const std::string& branchName);
    
private:
    std::string getRemoteUrl(const std::string& remoteName);
    std::string getCurrentBranch();
    std::string getBranchHash(const std::string& branchName);
    std::vector<std::string> getCommitHistory(const std::string& commitHash);
    std::vector<std::string> getAllReferencedObjects(const std::string& commitHash);
    bool sendObject(HINTERNET hConnect, const std::wstring& basePath, const std::string& hash, bool isHttps);
    bool updateRemoteRef(HINTERNET hConnect, const std::wstring& basePath, const std::string& refName, const std::string& hash, bool isHttps);
    std::string readObject(const std::string& hash);
    std::string httpPost(HINTERNET hConnect, const std::wstring& path, const std::string& data, bool isHttps);
};

#endif // PUSH_H
