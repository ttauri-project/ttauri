// Copyright Take Vos 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "abstract_button_widget.hpp"
#include "default_button_delegate.hpp"

namespace tt {

class toggle_widget final : public abstract_button_widget {
public:
    using super = abstract_button_widget;
    using delegate_type = typename super::delegate_type;
    using callback_ptr_type = typename delegate_type::callback_ptr_type;

    toggle_widget(gui_window &window, widget *parent, std::unique_ptr<delegate_type> delegate) noexcept :
        toggle_widget(window, parent, weak_or_unique_ptr<delegate_type>{std::move(delegate)})
    {
    }

    template<typename Value, typename... Args>
    requires(not std::is_convertible_v<Value, weak_or_unique_ptr<delegate_type>>)
        toggle_widget(gui_window &window, widget *parent, Value &&value, Args &&...args) noexcept :
        toggle_widget(
            window,
            parent,
            make_unique_default_button_delegate<button_type::toggle>(std::forward<Value>(value), std::forward<Args>(args)...))
    {
    }

    [[nodiscard]] bool constrain(hires_utc_clock::time_point display_time_point, bool need_reconstrain) noexcept override
    {
        tt_axiom(is_gui_thread());

        if (super::constrain(display_time_point, need_reconstrain)) {
            // Make room for button and margin.
            _button_size = {theme::global().size * 2.0f, theme::global().size};
            ttlet extra_size = extent2{theme::global().margin + _button_size.width(), 0.0f};
            _minimum_size += extra_size;
            _preferred_size += extra_size;
            _maximum_size += extra_size;

            _minimum_size = max(_minimum_size, _button_size);
            _preferred_size = max(_minimum_size, _button_size);
            _maximum_size = max(_minimum_size, _button_size);

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
            _button_rectangle = align(rectangle(), _button_size, alignment::top_left);

            _label_rectangle = aarectangle{_button_rectangle.right() + theme::global().margin, 0.0f, width(), height()};

            ttlet button_square =
                aarectangle{get<0>(_button_rectangle), extent2{_button_rectangle.height(), _button_rectangle.height()}};

            _pip_rectangle =
                align(button_square, extent2{theme::global().icon_size, theme::global().icon_size}, alignment::middle_center);

            ttlet pip_to_button_margin_x2 = _button_rectangle.height() - _pip_rectangle.height();
            _pip_move_range = _button_rectangle.width() - _pip_rectangle.width() - pip_to_button_margin_x2;
        }
        super::layout(displayTimePoint, need_layout);
    }

    void draw(draw_context context, hires_utc_clock::time_point display_time_point) noexcept override
    {
        tt_axiom(is_gui_thread());

        if (overlaps(context, _clipping_rectangle)) {
            draw_toggle_button(context);
            draw_toggle_pip(context, display_time_point);
        }

        super::draw(std::move(context), display_time_point);
    }

private:
    static constexpr hires_utc_clock::duration _animation_duration = 150ms;

    extent2 _button_size;
    aarectangle _button_rectangle;
    animator<float> _animated_value = _animation_duration;
    aarectangle _pip_rectangle;
    float _pip_move_range;

    toggle_widget(gui_window &window, widget *parent, weak_or_unique_ptr<delegate_type> delegate) noexcept :
        super(window, parent, std::move(delegate))
    {
        label_alignment = alignment::top_left;
    }

    void draw_toggle_button(draw_context context) noexcept
    {
        tt_axiom(is_gui_thread());

        context.draw_box_with_border_inside(
            _button_rectangle, background_color(), focus_color(), corner_shapes{_button_rectangle.height() * 0.5f});
    }

    void draw_toggle_pip(draw_context draw_context, hires_utc_clock::time_point display_time_point) noexcept
    {
        tt_axiom(is_gui_thread());

        _animated_value.update(state() == button_state::on ? 1.0f : 0.0f, display_time_point);
        if (_animated_value.is_animating()) {
            request_redraw();
        }

        ttlet positioned_pip_rectangle =
            translate3{_pip_move_range * _animated_value.current_value(), 0.0f, 0.1f} * _pip_rectangle;

        ttlet forground_color_ = state() == button_state::on ? accent_color() : foreground_color();
        draw_context.draw_box(
            positioned_pip_rectangle, forground_color_, corner_shapes{positioned_pip_rectangle.height() * 0.5f});
    }
};

} // namespace tt
