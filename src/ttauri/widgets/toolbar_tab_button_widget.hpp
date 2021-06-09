// Copyright Take Vos 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "abstract_button_widget.hpp"
#include "value_button_delegate.hpp"

namespace tt {

class toolbar_tab_button_widget final : public abstract_button_widget {
public:
    using super = abstract_button_widget;
    using delegate_type = typename super::delegate_type;
    using callback_ptr_type = typename delegate_type::callback_ptr_type;

    toolbar_tab_button_widget(
        gui_window &window,
        std::shared_ptr<widget> parent,
        std::shared_ptr<delegate_type> delegate = std::make_shared<delegate_type>()) noexcept :
        super(window, std::move(parent), std::move(delegate))
    {
        this->_label_alignment = alignment::top_center;
    }

    template<typename Label, typename Value, typename OnValue>
    toolbar_tab_button_widget(
        gui_window &window,
        std::shared_ptr<widget> parent,
        Label &&label,
        Value &&value,
        OnValue &&on_value) noexcept :
        toolbar_tab_button_widget(
            window,
            std::move(parent),
            make_button_delegate<button_type::radio>(std::forward<Value>(value), std::forward<OnValue>(on_value)))
    {
        set_label(std::forward<Label>(label));
    }

    [[nodiscard]] bool update_constraints(hires_utc_clock::time_point display_time_point, bool need_reconstrain) noexcept override
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());

        if (super::update_constraints(display_time_point, need_reconstrain)) {
            // On left side a check mark, on right side short-cut. Around the label extra margin.
            ttlet extra_size = extent2{theme::global->margin * 2.0f, theme::global->margin};
            this->_minimum_size += extra_size;
            this->_preferred_size += extra_size;
            this->_maximum_size += extra_size;

            tt_axiom(this->_minimum_size <= this->_preferred_size && this->_preferred_size <= this->_maximum_size);
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] void update_layout(hires_utc_clock::time_point displayTimePoint, bool need_layout) noexcept override
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());

        need_layout |= std::exchange(this->_request_relayout, false);
        if (need_layout) {
            this->_label_rectangle = aarectangle{
                theme::global->margin,
                0.0f,
                this->width() - theme::global->margin * 2.0f,
                this->height() - theme::global->margin};
        }
        super::update_layout(displayTimePoint, need_layout);
    }

    void draw(draw_context context, hires_utc_clock::time_point display_time_point) noexcept override
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());

        if (overlaps(context, this->_clipping_rectangle)) {
            draw_toolbar_tab_button(context);
            draw_toolbar_tab_focus_line(context);
        }

        super::draw(std::move(context), display_time_point);
    }

    void request_redraw() const noexcept override
    {
        // A toolbar tab button draws a focus line across the whole toolbar
        // which is beyond it's own clipping rectangle. The parent is the toolbar
        // so it will include everything that needs to be redrawn.
        this->parent().request_redraw();
    }

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return is_toolbar(group) and this->enabled;
    }

    [[nodiscard]] bool handle_event(command command) noexcept
    {
        ttlet lock = std::scoped_lock(gui_system_mutex);

        if (this->enabled) {
            switch (command) {
            case command::gui_toolbar_next:
                if (!this->is_last(keyboard_focus_group::toolbar)) {
                    this->window.update_keyboard_target(keyboard_focus_group::toolbar, keyboard_focus_direction::forward);
                    return true;
                }
                break;

            case command::gui_toolbar_prev:
                if (!this->is_first(keyboard_focus_group::toolbar)) {
                    this->window.update_keyboard_target(keyboard_focus_group::toolbar, keyboard_focus_direction::backward);
                    return true;
                }
                break;

            default:;
            }
        }

        return super::handle_event(command);
    }

private:
    void draw_toolbar_tab_focus_line(draw_context context) noexcept
    {
        if (this->_focus and this->window.active and this->state() == tt::button_state::on) {
            ttlet &parent_ = this->parent();
            ttlet parent_rectangle = aarectangle{this->_parent_to_local * parent_.rectangle()};

            // Create a line, on the bottom of the toolbar over the full width.
            ttlet line_rectangle = aarectangle{
                parent_rectangle.left(), parent_rectangle.bottom(), parent_rectangle.width(), theme::global->borderWidth};

            context.set_clipping_rectangle(line_rectangle);

            if (overlaps(context, line_rectangle)) {
                // Draw the line above every other direct child of the toolbar, and between
                // the selected-tab (0.6) and unselected-tabs (0.8).
                context.draw_filled_quad(translate_z(0.7f) * line_rectangle, this->focus_color());
            }
        }
    }

    void draw_toolbar_tab_button(draw_context context) noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());

        // Override the clipping rectangle to match the toolbar rectangle exactly
        // so that the bottom border of the tab button is not drawn.
        context.set_clipping_rectangle(aarectangle{this->_parent_to_local * this->parent().clipping_rectangle()});

        ttlet offset = theme::global->margin + theme::global->borderWidth;
        ttlet outline_rectangle = aarectangle{
            this->rectangle().left(),
            this->rectangle().bottom() - offset,
            this->rectangle().width(),
            this->rectangle().height() + offset};

        // The focus line will be placed at 0.7.
        ttlet button_z = (this->_focus && this->window.active) ? translate_z(0.8f) : translate_z(0.6f);

        auto button_color = (this->_hover || this->state() == button_state::on) ?
            theme::global->fillColor(this->_semantic_layer - 1) :
            theme::global->fillColor(this->_semantic_layer);

        ttlet corner_shapes = tt::corner_shapes{0.0f, 0.0f, theme::global->roundingRadius, theme::global->roundingRadius};
        context.draw_box_with_border_inside(
            button_z * outline_rectangle,
            button_color,
            (this->_focus && this->window.active) ? this->focus_color() : button_color,
            corner_shapes);
    }
};

} // namespace tt
