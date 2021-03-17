// Copyright Take Vos 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "pipeline_image_texture_map.hpp"
#include "gui_device_vulkan.hpp"

namespace tt::pipeline_image {

void texture_map::transitionLayout(const gui_device_vulkan &device, vk::Format format, vk::ImageLayout nextLayout) {
    if (layout != nextLayout) {
        device.transition_layout(image, format, layout, nextLayout);
        layout = nextLayout;
    }
}

}
