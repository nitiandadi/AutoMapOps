#pragma once

#include <string>

namespace automap::core {

// Stable business identifier used by map objects. Format and uniqueness are
// checked by map_validation so map_core can also represent invalid drafts.
using ObjectId = std::string;

}  // namespace automap::core
