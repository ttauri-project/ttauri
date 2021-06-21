// Copyright Take Vos 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "gui_system.hpp"
#include "gui_system_win32.hpp"
#include "../logger.hpp"
#include <chrono>

namespace tt {

using namespace std;


ssize_t gui_system::num_windows()
{
    ttlet lock = std::scoped_lock(gfx_system_mutex);

    ssize_t numberOfWindows = 0;
    for (const auto &device: gfx_system::global().devices) {
        numberOfWindows+= device->num_windows();
    }

    return numberOfWindows;
}

void gui_system::_handlevertical_sync(void *data, hires_utc_clock::time_point displayTimePoint)
{
    auto self = static_cast<gui_system *>(data);

    self->handlevertical_sync(displayTimePoint);
}

[[nodiscard]] gui_system *gui_system::subsystem_init() noexcept
{
    auto tmp = new gui_system_win32();
    tmp->init();
    return tmp;
}

void gui_system::subsystem_deinit() noexcept
{
    if (auto tmp = _global.exchange(nullptr)) {
        tmp->deinit();
        delete tmp;
    }
}

}
