// Copyright Take Vos 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memory>
#include <functional>

namespace tt {
class grid_layout_widget;

class grid_layout_delegate {
public:

    virtual void init(grid_layout_widget &sender) noexcept {}

    virtual void deinit(grid_layout_widget &sender) noexcept {}
};

} // namespace tt
