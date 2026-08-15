#include "automap/core/map_header.hpp"

namespace automap::core {

std::string_view local_coordinate_frame_name(LocalCoordinateFrame frame) noexcept {
    switch (frame) {
    case LocalCoordinateFrame::enu:
        return "enu";
    }

    return "unknown";
}

}  // namespace automap::core
