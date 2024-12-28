#include "config.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Configuration load(const std::string& config_path)
{
    std::ifstream f(config_path);

    Configuration config;
    try {
        config = json::parse(f);
    } catch (json::parse_error& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    return config;
}

RigidCoupleSimConfiguration RigidCoupleSimConfiguration::load(const std::string& config_path)
{
    std::ifstream f(config_path);
    RigidCoupleSimConfiguration config = json::parse(f);
    return config;
}
