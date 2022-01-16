// SPDX-License-Identifier: GPL-2.0-or-later
#include <filesystem>
#include <fstream>
#include <iostream>
#include "framecheck.h"
namespace fs = std::filesystem;

namespace Inkscape {
namespace FrameCheck {

std::ostream &logfile()
{
    static std::ofstream f;
    
    if (!f.is_open()) {
        try {
            auto path = fs::temp_directory_path() / "framecheck.txt";
            auto mode = std::ios_base::out|std::ios_base::app|std::ios_base::binary;
            f.open(path.string(), mode);
        } catch (...) {
            std::cerr << "failed to create framecheck logfile" << std::endl;
        }
    }
    
    return f;
}

} // namespace FrameCheck
} // namespace Inkscape
