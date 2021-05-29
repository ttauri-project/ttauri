// Copyright Take Vos 2020-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "widget.hpp"
#include "label_delegate.hpp"
#include "../GUI/draw_context.hpp"
#include "../stencils/label_stencil.hpp"
#include "../observable.hpp"
#include "../alignment.hpp"
#include <memory>
#include <string>
#include <array>
#include <optional>
#include <future>

namespace tt {

class label_widget final : public widget {
public:
    using super = widget;

    template<typename Label>
    label_widget(
        gui_window &window,
        std::shared_ptr<widget> parent,
        std::shared_ptr<label_delegate> delegate,
        alignment alignment,
        Label &&label) noexcept :
        super(window, std::move(parent), std::move(delegate)), _alignment(alignment)
    {
        set_label(std::forward<Label>(label));
    }

    template<typename Label>
    label_widget(gui_window &window, std::shared_ptr<widget> parent, Label &&label) noexcept :
        label_widget(
            window,
            std::move(parent),
            std::make_shared<label_delegate>(),
            alignment::middle_right,
            std::forward<Label>(label))
    {
    }

    ~label_widget() {}

    tt::label label() const noexcept
    {
        return delegate<label_delegate>().label(*this);
    }

    void set_label(observable<tt::label> label) noexcept
    {
        return delegate<label_delegate>().set_label(*this, std::move(label));
    }

    void set_label(l10n label) noexcept
    {
        return delegate<label_delegate>().set_label(*this, tt::label{label});
    }

    [[nodiscard]] bool update_constraints(hires_utc_clock::time_point display_time_point, bool need_reconstrain) noexcept override
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());

        if (super::update_constraints(display_time_point, need_reconstrain)) {
            _label_cell = stencil::make_unique(_alignment, label(), theme::global->labelStyle);
            _minimum_size = _label_cell->minimum_size();
            _preferred_size = _label_cell->preferred_size();
            _maximum_size = _label_cell->maximum_size();
            tt_axiom(_minimum_size <= _preferred_size && _preferred_size <= _maximum_size);
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
            _label_cell->set_layout_parameters(rectangle(), this->base_line());
        }
        super::update_layout(displayTimePoint, need_layout);
    }

    void draw(draw_context context, hires_utc_clock::time_point display_time_point) noexcept override
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());

        if (overlaps(context, _clipping_rectangle)) {
            tt_stencil_draw(_label_cell, context, this->label_color());
        }

        super::draw(std::move(context), display_time_point);
    }

private:
    std::unique_ptr<label_stencil> _label_cell;
    alignment _alignment;
};

} // namespace tt
