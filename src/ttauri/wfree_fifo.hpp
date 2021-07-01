

#pragma once

#include <concepts>
#include <atomic>
#include <memory>
#include <array>

namespace tt {

/** A wait-free multiple-producer/single-consumer fifo designed for absolute performance.
 * Because of performance reasons the ring-buffer is 64kByte.
 * Each slot in the ring buffer consists of a pointer and a byte buffer for storage.
 *
 * The number of slots in the ring-buffer is dictated by the size of each
 * slot and the ring-buffer size.
 *
 * @tparam T Base class of the value type stored in the ring buffer.
 * @tparam S Size of each slot, must be power-of-two.
 */
template<typename T, size_t S>
class wfree_fifo {
public:
    static_assert(std::has_single_bit(S), "Only power-of-two number of messages size allowed.");

    using value_type = T;

    constexpr size_t buffer_size = 65536;
    constexpr size_t slot_size = S;
    constexpr size_t num_slots = buffer_size / N;

    struct slot_type {
        std::atomic<value_type *> pointer;
        std::array<std::byte, slot_size - sizeof(value_type *)> buffer;
    };

    /** Take one message from the fifo slot.
     * Reads one message from the ring buffer and passes it to a call of operation.
     * If no message is available this function returns without calling operation.
     *
     * @param operation A `void(value_type const &)` which is caled when a message is available.
     */
    void take_one(std::invocable<value_type const &> auto &operation) noexcept
    {
        auto index = _tail;

        // The shift here should be eliminated by the equal shift inside the index operator.
        auto &slot = _slots[index / slot_size];

        // Check if the slot.pointer is not null, this is when the writer
        // has finished writing the slot.
        if (auto ptr = slot.pointer.load(std::memory_order::acquire)) {
            operation(*ptr);

            // Destroy the object depending if it lives in the buffer or on the heap.
            if (ptr == static_cast<void *>(slot.buffer.data())) {
                std::destroy_at(ptr);
            } else {
                delete ptr;
            }

            // We are done with the slot.
            slot.pointer.store(nullptr, std::memory_order::release);
            ++_tail;
        }
    }


    /** Create an message in-place on the fifo.
     * @tparam Message Message type derived from value_type to be stored in a free slot.
     * @param args The arguments passed to the constructor of Message.
     */
    template<typename Message, typename... Args> requires(sizeof(Message) > std::size(slot_type::buffer))
    void emplace(Args &&... args) noexcept
    {
        // We need a new index.
        // - The index is a byte index into 64kByte of memory.
        // - Increment index by the slot_size and the _head will overflow naturally
        //   at the end of the fifo.
        // - We don't care about memory ordering with other writer threads. as
        //   each slot has an atomic for handling read/writer contention.
        // - We don't have to check full/empty, this is done on the slot itself.
        ttlet index = _head.fetch_add(slot_size, std::memory_order::relaxed);

        // The division here should be eliminated by the equal multiplication inside the index operator.
        auto &slot = _slots[index / slot_size];

        // We need a heap allocated pointer with a fully constructed object
        // Lets do this ahead of time to let another thread have some time
        // to release the ring-buffer-slot.
        ttlet new_ptr = new Message(std::forward<Args>(args)...);

        // Wait until the slot.pointer is a nullptr.
        // We don't need to acquire since we wrote into a new heap location.
        // There are no other threads that will make this non-null afterwards.
        while (slot.pointer.load(std::memory_order::relaxed)) {
            // If we get here, that would suck, but nothing to do about it.
            [[unlikely]] increment_counter<"wfree_fifo">();
            std::this_thread::sleep_for(16ms);
            expected = 0;
        }

        // Release the heap for reading.
        slot.pointer.store(new_ptr, std::memory_order::release);
    }

    /** Create an message in-place on the fifo.
     * @tparam Message Message type derived from value_type to be stored in a free slot.
     * @param args The arguments passed to the constructor of Message.
     */
    template<typename Message, typename... Args> requires(sizeof(Message) <= std::size(slot_type::buffer))
    void emplace(Args &&... args) noexcept
    {
        // We need a new index.
        // - The index is a byte index into 64kByte of memory.
        // - Increment index by the slot_size and the _head will overflow naturally
        //   at the end of the fifo.
        // - We don't care about memory ordering with other writer threads. as
        //   each slot has an atomic for handling read/writer contention.
        // - We don't have to check full/empty, this is done on the slot itself.
        ttlet index = _head.fetch_add(slot_size, std::memory_order::relaxed);

        // The division here should be eliminated by the equal multiplication inside the index operator.
        auto &slot = _slots[index / slot_size];

        // Wait until the slot.pointer is a nullptr.
        // And aquire the buffer to start overwriting it.
        // There are no other threads that will make this non-null afterwards.
        while (slot.pointer.load(std::memory_order_acquire) != nullptr) {
            // If we get here, that would suck, but nothing to do about it.
            [[unlikely]] increment_counter<"wfree_fifo">();
            std::this_thread::sleep_for(16ms);
            expected = 0;
        }

        // Overwrite the buffer with the new slot.
        auto new_ptr = new (slot.buffer.data()) Message(std::forward<Args>(args)...);

        // Release the buffer for reading.
        slot.pointer.store(new_ptr, std::memory_order::release);
    }

private:
    std::atomic<uint16_t> _head;
    uint16_t _tail;
    std::array<slot_type, 256> _slots;
};


}

