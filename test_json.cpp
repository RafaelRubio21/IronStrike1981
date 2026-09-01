#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

int main() {
    json pt = json::parse(R"({"x": 510})");
    try {
        float px = pt["x"].get<float>();
        std::cout << "SUCCESS: " << px << std::endl;
    } catch (std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
    }
    return 0;
}
