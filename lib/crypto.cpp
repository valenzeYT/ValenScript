#include <bcrypt.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace crypto_lib {
namespace {
std::string to_hex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (uint8_t byte : data) {
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

std::vector<uint8_t> hash_data(const std::string& input) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        throw std::runtime_error("crypto: failed to open hash provider");
    }
    DWORD objectLength = 0;
    DWORD dataLength = 0;
    BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &dataLength, 0);
    std::vector<uint8_t> object(objectLength);

    BCRYPT_HASH_HANDLE hHash = nullptr;
    if (BCryptCreateHash(hAlg, &hHash, object.data(), objectLength, nullptr, 0, 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("crypto: failed to create hash");
    }
    if (BCryptHashData(hHash, reinterpret_cast<PUCHAR>(const_cast<char*>(input.data())), static_cast<ULONG>(input.size()), 0) != 0) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("crypto: failed to hash data");
    }
    ULONG resultSize = 0;
    BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&resultSize), sizeof(resultSize), &dataLength, 0);
    std::vector<uint8_t> result(resultSize);
    if (BCryptFinishHash(hHash, result.data(), resultSize, 0) != 0) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("crypto: failed to finish hash");
    }
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return result;
}
} // namespace

std::string sha256(const std::string& input) {
    return to_hex(hash_data(input));
}

std::string random_bytes(size_t count) {
    std::vector<uint8_t> buffer(count);
    if (BCryptGenRandom(nullptr, buffer.data(), static_cast<ULONG>(count), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        throw std::runtime_error("crypto: random generation failed");
    }
    return to_hex(buffer);
}

} // namespace crypto_lib
