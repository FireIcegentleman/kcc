//
// Created by kaiser on 2019/11/8.
//

#ifndef KCC_SRC_MEMORY_POOL_H_
#define KCC_SRC_MEMORY_POOL_H_

#include <cstddef>
#include <cstdint>
#include <utility>

namespace kcc {

template <typename T, std::size_t BlockSize = 4096>
class MemoryPool {
 public:
  using ValueType = T;
  using Pointer = T*;
  using ConstPointer = const T*;
  using SizeType = std::size_t;

  MemoryPool() noexcept = default;
  ~MemoryPool() noexcept;

  MemoryPool(const MemoryPool&) = delete;
  MemoryPool(MemoryPool&&) = delete;
  MemoryPool& operator=(const MemoryPool&) = delete;
  MemoryPool& operator=(MemoryPool&&) = delete;

  Pointer Allocate(SizeType n = 1, ConstPointer hint = 0);

 private:
  union Slot {
    ValueType element;
    Slot* next;
  };

  using DataPointer = char*;
  using SlotType = Slot;
  using SlotPointer = Slot*;

  SlotPointer current_block_{};
  SlotPointer current_slot_{};
  SlotPointer last_slot_{};
  SlotPointer free_slots_{};

  void AllocateBlock();
  SizeType PadPointer(DataPointer p, SizeType align) const noexcept;

  static_assert(BlockSize >= 2 * sizeof(SlotType), "BlockSize too small.");
};

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool() noexcept {
  auto curr{current_block_};
  while (curr != nullptr) {
    auto prev{curr->next};
    operator delete(reinterpret_cast<void*>(curr));
    curr = prev;
  }
}

template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::Pointer
MemoryPool<T, BlockSize>::Allocate(SizeType, ConstPointer) {
  if (free_slots_ != nullptr) {
    auto result{reinterpret_cast<Pointer>(free_slots_)};
    free_slots_ = free_slots_->next;
    return result;
  } else {
    if (current_slot_ >= last_slot_) {
      AllocateBlock();
    }
    return reinterpret_cast<Pointer>(current_slot_++);
  }
}

template <typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::AllocateBlock() {
  auto new_block{reinterpret_cast<DataPointer>(operator new(BlockSize))};
  reinterpret_cast<SlotPointer>(new_block)->next = current_block_;
  current_block_ = reinterpret_cast<SlotPointer>(new_block);

  auto body{new_block + sizeof(SlotPointer)};
  auto body_padding{PadPointer(body, alignof(SlotType))};
  current_slot_ = reinterpret_cast<SlotPointer>(body + body_padding);
  last_slot_ = reinterpret_cast<SlotPointer>(new_block + BlockSize -
                                             sizeof(SlotType) + 1);
}

template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::SizeType
MemoryPool<T, BlockSize>::PadPointer(DataPointer p, SizeType align) const
    noexcept {
  auto result{reinterpret_cast<std::uintptr_t>(p)};
  return ((align - result) % align);
}

}  // namespace kcc

#endif  // KCC_SRC_MEMORY_POOL_H_
