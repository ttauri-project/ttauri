// Copyright Take Vos 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gfx_device.hpp"
#include "../unfair_recursive_mutex.hpp"
#include "../subsystem.hpp"
#include <span>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace tt {
class gfx_surface;

/** Graphics system
 */
class gfx_system {
public:
    //! List of all devices.
    std::vector<std::shared_ptr<gfx_device>> devices;

    gfx_system() noexcept
    {
    }

    virtual ~gfx_system() {
    }

    gfx_system(const gfx_system &) = delete;
    gfx_system &operator=(const gfx_system &) = delete;
    gfx_system(gfx_system &&) = delete;
    gfx_system &operator=(gfx_system &&) = delete;

    /** Initialize after construction.
     * Call this function directly after the constructor on the same thread.
     */
    virtual void init() {};
    virtual void deinit() {};

    [[nodiscard]] virtual std::unique_ptr<gfx_surface> make_surface(os_handle instance, void *os_window) const noexcept = 0;

    static inline gfx_system &global() noexcept
    {
        return *start_subsystem_or_terminate(_global, nullptr, subsystem_init, subsystem_deinit);
    }

    gfx_device *findBestDeviceForSurface(gfx_surface const &surface);

private:
    static inline std::atomic<gfx_system *> _global;

    [[nodiscard]] static gfx_system *subsystem_init() noexcept;
    [[nodiscard]] static void subsystem_deinit() noexcept;
};

}
