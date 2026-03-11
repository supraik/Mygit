#ifndef REMOTE_H
#define REMOTE_H

#include <string>
#include <vector>
#include <map>

class Remote {
public:
    bool execute(const std::vector<std::string>& args);
    
private:
    bool addRemote(const std::string& name, const std::string& url);
    bool removeRemote(const std::string& name);
    bool listRemotes(bool verbose);
    std::map<std::string, std::string> getRemotes();
};

#endif // REMOTE_H
