#include "automap/io/canonical_json.hpp"

#include <iostream>

int main() {
    std::cout << "AutoMapOps " << AUTOMAP_PROJECT_VERSION << '\n';
    std::cout << "Canonical format: "
              << automap::io::canonical_json_format_name() << '\n';
    return 0;
}
