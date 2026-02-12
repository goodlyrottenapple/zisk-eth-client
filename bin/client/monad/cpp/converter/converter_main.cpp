// Converts Ethereum blockchain test JSON files to binary format.
// Usage: ethtest_converter <input.json> <output.bin> [--rev <revision>]

#include "binary_format.hpp"
#include "../evmone_test/blockchaintest.hpp"
#include "../evmone_test/utils.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input.json> <output.bin> [--rev <revision>]\n";
        std::cerr << "  --rev <revision>  Filter tests by genesis revision (e.g. Cancun, Prague)\n";
        return 1;
    }

    const std::string input_path = argv[1];
    const std::string output_path = argv[2];

    // Parse optional --rev flag
    std::optional<evmc_revision> filter_rev;
    for (int i = 3; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--rev" && i + 1 < argc)
        {
            filter_rev = evmone::test::to_rev(argv[i + 1]);
            ++i;
        }
    }

    // Load JSON
    std::ifstream input_file(input_path);
    if (!input_file)
    {
        std::cerr << "Error: cannot open " << input_path << "\n";
        return 1;
    }

    auto tests = evmone::test::load_blockchain_tests(input_file);
    const size_t total = tests.size();
    std::cout << "Loaded " << total << " test(s) from " << input_path << "\n";

    // Filter by revision if requested
    if (filter_rev.has_value())
    {
        std::erase_if(tests,
            [&](const auto& t) { return t.rev.genesis_rev != *filter_rev; });
        std::cout << "After --rev filter: " << tests.size() << " test(s) kept, "
                  << (total - tests.size()) << " filtered out\n";
    }

    if (tests.empty())
    {
        std::cerr << "Error: no tests to write\n";
        return 1;
    }

    // Serialize to binary
    auto binary = binary_format::serialize_blockchain_tests(tests);
    std::cout << "Binary size: " << binary.size() << " bytes\n";

    // Write output
    std::ofstream output_file(output_path, std::ios::binary);
    if (!output_file)
    {
        std::cerr << "Error: cannot create " << output_path << "\n";
        return 1;
    }
    output_file.write(reinterpret_cast<const char*>(binary.data()), binary.size());

    std::cout << "Wrote " << tests.size() << " test(s) to " << output_path << "\n";
    return 0;
}
