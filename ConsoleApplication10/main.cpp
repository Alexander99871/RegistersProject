#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <nlohmann/json.hpp>
typedef unsigned int uint;

using json = nlohmann::json;

struct Field {
    std::string name;
    uint32_t mask;
    std::string description;
};

struct Register {
    std::string name;
    uint32_t address;
    uint32_t RAW_value = 0;
    std::vector<Field> fields;
};

static void from_json(const json& j, Field& field) {
    j.at("name").get_to(field.name);
    j.at("mask").get_to(field.mask);
    j.at("description").get_to(field.description);
}

static void from_json(const json& j, Register& reg) {
    j.at("name").get_to(reg.name);
    j.at("address").get_to(reg.address);
    j.at("fields").get_to(reg.fields);
}

static uint count_trailing_zeros(uint32_t val) {
    unsigned long shift = 0;
    #ifdef GNUC
    shift = __builtin_ctz(val);
    #elif defined(_MSC_VER)
    _BitScanForward(&shift, val);
    #else
    shift = 0;
    while (((val >> shift) & 1) == 0) {
        shift++;
    }
    #endif
    return shift;
}

static std::vector<Register> parse_registers(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + file_path);
    }
    json j;
    file >> j;
    return j.at("registers").get<std::vector<Register>>();
}

static void apply_values(std::vector<Register>& registers, const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + file_path);
    }
    json j;
    file >> j;
    for (auto& reg : registers) {
        if (j.contains(reg.name)) {
            reg.RAW_value = j.at(reg.name).get<uint32_t>();
        }
    }
}

static void print_parsed_fields(const std::vector<Register>& registers) {
    for (const auto& reg : registers) {
        std::cout << "Register: " << reg.name << " (0x" << std::hex << reg.address << ")\n";
        std::cout << "RAW Value: 0x" << std::hex << reg.RAW_value << "\n";
        for (const auto& field : reg.fields) {
            uint32_t field_value = (reg.RAW_value & field.mask);
            unsigned long shift = count_trailing_zeros(field.mask);
            field_value >>= shift;
            std::cout << " " << field.name << ": 0x" << std::hex << field_value << " (" << field.description << ")\n";
        }
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <registers.json> <values.json>\n";
        return 1;
    }

    try {
        const std::string schema_file_path = argv[1];
        const std::string values_file_path = argv[2];
        auto registers = parse_registers(schema_file_path);
        apply_values(registers, values_file_path);
        print_parsed_fields(registers);
    }
    catch (const json::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << '\n';
        return 1;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}



