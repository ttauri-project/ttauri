// Copyright Take Vos 2019-2020.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "abstract_button_widget.hpp"

namespace tt {

class momentary_button_widget final : public abstract_button_widget {
public:
    using super = abstract_button_widget;
    using delegate_type = typename super::delegate_type;
    using callback_ptr_type = typename delegate_type::callback_ptr_type;

    template<typename Label>
    momentary_button_widget(
        gui_window &window,
        widget *parent,
        Label &&label,
        std::weak_ptr<delegate_type> delegate) noexcept :
        momentary_button_widget(window, parent, std::forward<Label>(label), weak_or_unique_ptr{std::move(delegate)})
    {
    }

    template<typename Label>
    momentary_button_widget(gui_window &window, widget *parent, Label &&label) noexcept :
        momentary_button_widget(
            window,
            parent,
            std::forward<Label>(label),
            std::make_unique<delegate_type>())
    {
    }

    /// @privatesection
    [[nodiscard]] bool constrain(hires_utc_clock::time_point display_time_point, bool need_reconstrain) noexcept override;
    [[nodiscard]] void layout(hires_utc_clock::time_point displayTimePoint, bool need_layout) noexcept override;
    void draw(draw_context context, hires_utc_clock::time_point display_time_point) noexcept override;
    /// @endprivatesection
private:
    template<typename Label>
    momentary_button_widget(
        gui_window &window,
        widget *parent,
        Label &&label,
        weak_or_unique_ptr<delegate_type> delegate) noexcept :
        super(window, parent, std::move(delegate))
    {
        label_alignment = alignment::middle_left;
        set_label(std::forward<Label>(label));
    }

    void draw_label_button(draw_context const &context) noexcept;
};

} // namespace tt
