#pragma once
#include <filesystem>

class ConsumerApp final {
public:
    int run(const std::filesystem::path& export_directory) const;
};
