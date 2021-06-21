// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <optional>

namespace tt {
class gui_system;

class gui_system_delegate {
public:
    /** This function is called when the last window is closed.
     * @param An exit code if the gui-system's event-loop should exit; otherwise empty.
     */
    [[nodiscard]] virtual std::optional<int> last_window_closed(gui_system &self) { return 0; };
};

}
