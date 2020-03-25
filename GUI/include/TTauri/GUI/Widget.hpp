// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "TTauri/GUI/BoxModel.hpp"
#include "TTauri/GUI/PipelineImage_Backing.hpp"
#include "TTauri/GUI/Window_forward.hpp"
#include "TTauri/GUI/Device_forward.hpp"
#include "TTauri/GUI/MouseEvent.hpp"
#include "TTauri/GUI/HitBox.hpp"
#include "TTauri/GUI/KeyboardEvent.hpp"
#include "TTauri/Text/ShapedText.hpp"
#include "TTauri/Foundation/attributes.hpp"
#include "TTauri/Foundation/Path.hpp"
#include "TTauri/Foundation/R16G16B16A16SFloat.hpp"
#include "TTauri/Foundation/URL.hpp"
#include <TTauri/Foundation/pickle.hpp>
#include <TTauri/Foundation/vspan.hpp>
#include <TTauri/Foundation/utils.hpp>
#include <limits>
#include <memory>
#include <vector>
#include <mutex>
#include <typeinfo>

namespace TTauri::GUI::PipelineImage {
struct Image;
struct Vertex;
}
namespace TTauri::GUI::PipelineSDF {
struct Vertex;
}
namespace TTauri::GUI::PipelineFlat {
struct Vertex;
}
namespace TTauri::GUI::PipelineBox {
struct Vertex;
}

namespace TTauri::GUI::Widgets {

/*! View of a widget.
 * A view contains the dynamic data for a Widget. It is often accompanied with a Backing
 * which contains that static data of an Widget and the drawing code. Backings are shared
 * between Views.
 */
class Widget {
private:
    /** Incremented when the widget's state was modified.
    */
    std::atomic<uint64_t> modificationRequest = 1;

    /** Copied from modificationRequest before processing the modificationRequest.
    */
    uint64_t modificationVersion = 0;


public:
    //! Convenient reference to the Window.
    Window *window;

    Widget *parent;

    std::vector<std::unique_ptr<Widget>> children;

    /** The next widget to select when pressing tab.
    */
    Widgets::Widget *nextKeyboardWidget = nullptr;

    /** The prev widget to select when pressing shift-tab.
     */
    Widgets::Widget *prevKeyboardWidget = nullptr;

    /** A key for checking if the state of the widget has changed.
     */
    std::string current_state_key;

    /** Temporary for calculation of the current_state_key.
    */
    mutable std::string next_state_key;

    //! Location of the frame compared to the window.
    BoxModel box;

    //! Rectangle, extracted from the box
    rect rectangle; 

    float elevation = 0.0;

    /** The widget is enabled.
     */
    bool enabled = true;

    /** Mouse cursor is hovering over the widget.
     */
    bool hover = false;

    /** The widget has keyboard focus.
     */
    bool focus = false;

    /*! Constructor for creating sub views.
     */
    Widget() noexcept;
    virtual ~Widget() {}

    Widget(const Widget &) = delete;
    Widget &operator=(const Widget &) = delete;
    Widget(Widget &&) = delete;
    Widget &operator=(Widget &&) = delete;

    virtual void setParent(Widget *parent) noexcept;

    template<typename T, typename... Args>
    T *addWidget(Args... args) noexcept {
        auto widget = std::make_unique<T>(args...);
        auto widget_ptr = widget.get();

        widget->setParent(this);

        children.push_back(move(widget));
        return widget_ptr;
    }

    [[nodiscard]] Device *device() const noexcept;

    /** Should be called after the internal state of the widget was modified.
    */
    force_inline bool setModified(bool x=true) noexcept {
        if (x) {
            modificationRequest.fetch_add(1, std::memory_order::memory_order_relaxed);
        }
        return x;
    }

    /** Check if the state of the widget is modified.
     */
    [[nodiscard]] force_inline bool modified() noexcept {
        let request = modificationRequest.load(std::memory_order::memory_order_acquire);
        if (modificationVersion != request) {
            modificationVersion = request;
            return true;
        } else {
            return false;
        }
    }

    /** Force all widgets to redraw themselves.
    * This will be called when the widgets are being laid out because of
    * window resize or widgets being added.
    */
    void setModifiedRecursive() noexcept {
        setModified();
        for (let &child: children) {
            child->setModifiedRecursive();
        }
    }

    /** Find the widget that is under the mouse cursor.
     */
    [[nodiscard]] virtual HitBox hitBoxTest(vec position) noexcept;

    /** Check if the widget will accept keyboard focus.
     */
    [[nodiscard]] virtual bool acceptsFocus() noexcept {
        return false;
    }

    /** Update and place vertices.
    *
    * The overriding function should call the base class's update(), the place
    * where the call this function will determine the order of the vertices into
    * each buffer. This is important when needing to do the painters algorithm
    * for alpha-compositing. However the pipelines are always drawn in the same
    * order.
    *
    * @param modified The data in the widget has been modified.
    * @param flat_vertices Vertex buffer of the flat-pipeline.
    * @param box_vertices Vertex buffer of the box-pipeline.
    * @param image_vertices Vertex buffer of the image-pipeline.
    * @param sdf_vertices Vertex buffer of the sdf-pipeline.
    * @return true when a widgets is currently running an animation and wants
    *         to change its appearance in the next frame.
    */
    [[nodiscard]] virtual bool updateAndPlaceVertices(
        vspan<PipelineFlat::Vertex> &flat_vertices,
        vspan<PipelineBox::Vertex> &box_vertices,
        vspan<PipelineImage::Vertex> &image_vertices,
        vspan<PipelineSDF::Vertex> &sdf_vertices
    ) noexcept;

    /** Handle command.
     * @return true when a widgets wants to change its appearance in the next frame.
     */
    [[nodiscard]] virtual bool handleCommand(string_ltag command) noexcept {
        return false;
    }

    /*! Handle mouse event.
    * Called by the operating system to show the position and button state of the mouse.
    * This is called very often so it must be made efficient.
    * This function is also used to determine the mouse cursor.
    *
    * @return true when a widgets wants to change its appearance in the next frame.
    */
    [[nodiscard]] virtual bool handleMouseEvent(MouseEvent const &event) noexcept {
        if (event.type == MouseEvent::Type::Entered) {
            hover = true;
            return true;
        } else if (event.type == MouseEvent::Type::Exited) {
            hover = false;
            return true;
        } else {
            return false;
        }
    }

    /*! Handle keyboard event.
    * Called by the operating system when editing text, or entering special keys
    *
    * @return true when a widgets wants to change its appearance in the next frame.
    */
    [[nodiscard]] virtual bool handleKeyboardEvent(KeyboardEvent const &event) noexcept {
        auto continueRendering = false;

        switch (event.type) {
        case KeyboardEvent::Type::Entered:
            focus = true;
            continueRendering |= true;
            break;

        case KeyboardEvent::Type::Exited:
            focus = false;
            continueRendering |= true;
            break;

        case KeyboardEvent::Type::Key:
            for (let command : event.getCommands()) {
                continueRendering |= handleCommand(command);
            }
            break;

        default:;
        }
        return continueRendering;
    }

};

}
