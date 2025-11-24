#pragma once

#include <Filters.hpp>

class HashingAlgorithm {
    public:
        virtual std::string Run(const std::string &filename, const FilterMap &filters) = 0;

    protected:
        HashingAlgorithm() {};
};

class HashingAlgorithmSHA256 : public HashingAlgorithm {
    public:
        HashingAlgorithmSHA256() : HashingAlgorithm() {};

        std::string Run(const std::string &filename, const FilterMap &filters) override;
};
class HashingAlgorithmSHA512 : public HashingAlgorithm {
    public:
        HashingAlgorithmSHA512() : HashingAlgorithm() {};

        std::string Run(const std::string &filename, const FilterMap &filters) override;
};
class HashingAlgorithmSHA3_256 : public HashingAlgorithm {
    public:
        HashingAlgorithmSHA3_256() : HashingAlgorithm() {};

        std::string Run(const std::string &filename, const FilterMap &filters) override;
};
class HashingAlgorithmSHA3_512 : public HashingAlgorithm {
    public:
        HashingAlgorithmSHA3_512() : HashingAlgorithm() {};

        std::string Run(const std::string &filename, const FilterMap &filters) override;
};
class HashingAlgorithmBlake2s256 : public HashingAlgorithm {
    public:
        HashingAlgorithmBlake2s256() : HashingAlgorithm() {};

        std::string Run(const std::string &filename, const FilterMap &filters) override;
};
class HashingAlgorithmBlake2s512 : public HashingAlgorithm {
    public:
        HashingAlgorithmBlake2s512() : HashingAlgorithm() {};

        std::string Run(const std::string &filename, const FilterMap &filters) override;
};
