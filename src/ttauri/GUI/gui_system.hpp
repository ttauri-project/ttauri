// Copyright Take Vos 2020, 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gui_window.hpp"
#include "gui_window_win32.hpp"
#include "gui_system_delegate.hpp"
#include "../GFX/gfx_system.hpp"
#include "../GFX/gfx_device.hpp"
#include "../thread.hpp"
#include "../unfair_recursive_mutex.hpp"
#include <span>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace tt {

/** Graphics system
 */
class gui_system {
public:
    static inline os_handle instance;

    thread_id const thread_id;

    gui_system() noexcept : thread_id(current_thread_id()), _delegate() {}

    virtual ~gui_system() {}

    gui_system(const gui_system &) = delete;
    gui_system &operator=(const gui_system &) = delete;
    gui_system(gui_system &&) = delete;
    gui_system &operator=(gui_system &&) = delete;

    /** Initialize after construction.
     * Call this function directly after the constructor on the same thread.
     */
    virtual void init() noexcept
    {
        if (auto delegate = _delegate.lock()) {
            delegate->init(*this);
        }
    }

    virtual void deinit() noexcept
    {
        if (auto delegate = _delegate.lock()) {
            delegate->deinit(*this);
        }
    }

    void set_delegate(std::weak_ptr<gui_system_delegate> delegate) noexcept
    {
        _delegate = std::move(delegate);
    }

    virtual void run_from_event_queue(std::function<void()> function) = 0;

    /** Start the GUI event loop.
     *
     * This function will start the GUI event loop.
     * The event loop will monitor keyboard & mouse event, changes in window
     * size & position and rendering of all windows.
     *
     * When all windows are closed this function will return with an exit
     * code of zero, or the return value from the delegate.
     * Calling `exit()` will also cause this function to return.
     *
     * @return exit code.
     */
    virtual int loop() = 0;

    virtual void exit(int exit_code) = 0;

    gui_window &add_window(std::unique_ptr<gui_window> window);

    /** Create a new window.
     * @param args The arguments that are forwarded to the constructor of
     *             `tt::gui_window_win32`.
     * @return A reference to the new window.
     */
    template<typename... Args>
    gui_window &make_window(Args &&...args)
    {
        tt_axiom(is_gui_thread());

        // XXX abstract away the _win32 part.
        auto window = std::make_unique<gui_window_win32>(std::forward<Args>(args)...);
        window->init();

        return add_window(std::move(window));
    }

    /*! Count the number of windows managed by the GUI.
     */
    ssize_t num_windows();

    void render(hires_utc_clock::time_point display_time_point)
    {
        tt_axiom(is_gui_thread());

        for (auto &window : _windows) {
            window->render(display_time_point);
            if (window->is_closed()) {
                window->deinit();
                window = nullptr;
            }
        }
        std::erase(_windows, nullptr);

        ttlet num_windows = std::size(_windows);
        if (num_windows == 0 && num_windows != _previous_num_windows) {
            // If last_window_closed() creates a new window we should
            // let it do that before entering the event queue again.
            // win32 is a bit picky about running without windows.
            if (auto delegate = _delegate.lock()) {
                if (auto exit_code = delegate->last_window_closed(*this)) {
                    gui_system::global().exit(*exit_code);
                }
            } else {
                gui_system::global().exit(0);
            }
        }
        _previous_num_windows = num_windows;
    }

    /** Check if this thread is the same as the gui thread.
     */
    [[nodiscard]] bool is_gui_thread() const noexcept
    {
        return thread_id == current_thread_id();
    }

    /** Get a reference to the global gui_system.
     *
     * The first time this function is called it will initialize the gui_system.
     *
     * @return A reference to the global gui_system.
     */
    [[nodiscard]] static gui_system &global() noexcept
    {
        return *start_subsystem_or_terminate(_global, nullptr, subsystem_init, subsystem_deinit);
    }

private:
    static inline std::atomic<gui_system *> _global;

    std::weak_ptr<gui_system_delegate> _delegate;

    std::vector<std::unique_ptr<gui_window>> _windows;
    size_t _previous_num_windows;

    [[nodiscard]] static gui_system *subsystem_init() noexcept;
    static void subsystem_deinit() noexcept;
};

} // namespace tt
