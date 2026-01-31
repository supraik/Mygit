#include "hash.h"
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <wincrypt.h>

std::string Hash::sha1(const std::string& data) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE hash[20];
    DWORD hashLen = 20;
    
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return "";
    }
    
    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    if (!CryptHashData(hHash, (BYTE*)data.c_str(), data.length(), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    
    return toHex(hash, hashLen);
}

std::string Hash::blobHash(const std::vector<char>& content) {
    // Create git blob header: "blob <size>\0"
    std::string header = "blob " + std::to_string(content.size()) + '\0';
    
    // Combine header and content
    std::string data = header + std::string(content.begin(), content.end());
    
    return sha1(data);
}

std::string Hash::toHex(const unsigned char* hash, size_t length) {
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
