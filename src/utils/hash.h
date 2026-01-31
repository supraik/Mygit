#ifndef HASH_H
#define HASH_H

#include <string>
#include <vector>

class Hash {
public:
    // Compute SHA-1 hash of data
    static std::string sha1(const std::string& data);
    
    // Compute SHA-1 hash for a blob object (with git header)
    static std::string blobHash(const std::vector<char>& content);
    
    // Convert binary hash to hex string
    static std::string toHex(const unsigned char* hash, size_t length);
};

#endif
