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

    [[nodiscard]] bool constrain(hires_utc_clock::time_point display_time_point, bool need_reconstrain) noexcept override
    {
        tt_axiom(is_gui_thread());

        if (super::constrain(display_time_point, need_reconstrain)) {
            // On left side a check mark, on right side short-cut. Around the label extra margin.
            ttlet extra_size = extent2{theme::global().margin * 2.0f, theme::global().margin * 2.0f};
            _minimum_size += extra_size;
            _preferred_size += extra_size;
            _maximum_size += extra_size;

            tt_axiom(_minimum_size <= _preferred_size && _preferred_size <= _maximum_size);
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] void layout(hires_utc_clock::time_point displayTimePoint, bool need_layout) noexcept override
    {
        tt_axiom(is_gui_thread());

        need_layout |= _request_layout.exchange(false);
        if (need_layout) {
            _label_rectangle = aarectangle{theme::global().margin, 0.0f, width() - theme::global().margin * 2.0f, height()};
        }
        super::layout(displayTimePoint, need_layout);
    }

    void draw(draw_context context, hires_utc_clock::time_point display_time_point) noexcept override
    {
        tt_axiom(is_gui_thread());

        if (overlaps(context, _clipping_rectangle)) {
            draw_label_button(context);
        }

        super::draw(std::move(context), display_time_point);
    }

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

    void draw_label_button(draw_context const &context) noexcept
    {
        tt_axiom(is_gui_thread());

        // Move the border of the button in the middle of a pixel.
        context.draw_box_with_border_inside(
            rectangle(), background_color(), focus_color(), corner_shapes{theme::global().rounding_radius});
    }
};

} // namespace tt
