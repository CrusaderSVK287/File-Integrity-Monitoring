#include <HashingAlgorithm.hpp>
#include <CryptoUtil.hpp>

std::string HashingAlgorithmSHA256::Run(const std::string &s, const FilterMap &filters)
{
    return SHAFileUtil::SHA256(s, filters);
}
std::string HashingAlgorithmSHA512::Run(const std::string &s, const FilterMap &filters)
{
    return SHAFileUtil::SHA512(s, filters);
}
std::string HashingAlgorithmSHA3_256::Run(const std::string &s, const FilterMap &filters)
{
    return SHAFileUtil::SHA3_256(s, filters);
}
std::string HashingAlgorithmSHA3_512::Run(const std::string &s, const FilterMap &filters)
{
    return SHAFileUtil::SHA3_512(s, filters);
}
std::string HashingAlgorithmBlake2s256::Run(const std::string &s, const FilterMap &filters)
{
    return SHAFileUtil::Blake2s256(s, filters);
}
std::string HashingAlgorithmBlake2s512::Run(const std::string &s, const FilterMap &filters)
{
    return SHAFileUtil::Blake2s512(s, filters);
}
