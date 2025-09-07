#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <yaml-cpp/yaml.h>
#include <cxxopts.hpp>
#include "schema.hpp"
#include "solver.hpp"
#include "api_handler.hpp"

namespace kagamiz {

std::string create_random_plan(int n) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 5);
    
    std::string plan;
    plan.reserve(18 * n);
    for (int i = 0; i < 18 * n; i++) {
        plan += std::to_string(dis(gen));
    }
    return plan;
}

Config load_config(const std::string& filename) {
    YAML::Node config = YAML::LoadFile(filename);
    
    Config cfg;
    cfg.api_domain.local = config["api_domain"]["local"].as<std::string>();
    cfg.api_domain.production = config["api_domain"]["production"].as<std::string>();
    cfg.token = config["token"].as<std::string>();
    cfg.request_timeout = config["request_timeout"].as<int>();
    
    return cfg;
}

} // namespace kagamiz

int main(int argc, char* argv[]) {
    try {
        cxxopts::Options options("kagamiz_cpp", "ICFPC 2025 Kagamiz C++ Solver");
        
        options.add_options()
            ("p,problem", "問題名", cxxopts::value<std::string>())
            ("e,env", "環境 (local/production)", cxxopts::value<std::string>()->default_value("local"))
            ("h,help", "ヘルプを表示");
        
        auto result = options.parse(argc, argv);
        
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            std::cout << "問題名の選択肢: probatio, primus, secundus, tertius, quartus, quintus" << std::endl;
            return 0;
        }
        
        if (!result.count("problem")) {
            std::cerr << "エラー: --problem オプションが必要です" << std::endl;
            std::cout << options.help() << std::endl;
            return 1;
        }
        
        std::string problem_name_str = result["problem"].as<std::string>();
        std::string env = result["env"].as<std::string>();
        
        // Convert string to ProblemName enum
        kagamiz::ProblemName problem_name;
        if (problem_name_str == "probatio") problem_name = kagamiz::ProblemName::PROBATIO;
        else if (problem_name_str == "primus") problem_name = kagamiz::ProblemName::PRIMUS;
        else if (problem_name_str == "secundus") problem_name = kagamiz::ProblemName::SECUNDUS;
        else if (problem_name_str == "tertius") problem_name = kagamiz::ProblemName::TERTIUS;
        else if (problem_name_str == "quartus") problem_name = kagamiz::ProblemName::QUARTUS;
        else if (problem_name_str == "quintus") problem_name = kagamiz::ProblemName::QUINTUS;
        else {
            std::cerr << "エラー: 無効な問題名です: " << problem_name_str << std::endl;
            std::cout << "問題名の選択肢: probatio, primus, secundus, tertius, quartus, quintus" << std::endl;
            return 1;
        }
        
        if (env != "local" && env != "production") {
            std::cerr << "エラー: 無効な環境です: " << env << std::endl;
            std::cout << "環境の選択肢: local, production" << std::endl;
            return 1;
        }
        
        kagamiz::Config config = kagamiz::load_config("config.yaml");
        
        std::string api_domain;
        if (env == "local") {
            api_domain = config.api_domain.local;
        } else {
            api_domain = config.api_domain.production;
        }
        
        kagamiz::ApiHandler api_handler(api_domain, config.request_timeout);
        
        kagamiz::SelectRequest select_request{
            .id = config.token,
            .problem_name = problem_name
        };
        kagamiz::SelectResponse select_response = api_handler.select(select_request);
        std::cout << "Select response: " << nlohmann::json(select_response).dump() << std::endl;
        
        int n = kagamiz::get_problem_size(problem_name);
        std::vector<std::string> plans = {kagamiz::create_random_plan(n)};
        kagamiz::ExploreRequest explore_request{
            .id = config.token,
            .plans = plans
        };
        kagamiz::ExploreResponse explore_response = api_handler.explore(explore_request);
        std::cout << "Explore response: " << nlohmann::json(explore_response).dump() << std::endl;
        
        kagamiz::MapData map_data = kagamiz::solve2(n, plans[0], explore_response.results[0]);
        kagamiz::GuessRequest guess_request{
            .id = config.token,
            .map = map_data
        };
        kagamiz::GuessResponse guess_response = api_handler.guess(guess_request);
        std::cout << "Guess response: " << nlohmann::json(guess_response).dump() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
