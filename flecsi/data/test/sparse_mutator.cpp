#include <vector>
#include <iostream>
#include <cassert>
#include <map>
#include <algorithm>
#include <cstring>
#include <np.h>

double uniform(){
  return double(rand())/RAND_MAX;
}

double uniform(double a, double b){
  return a + (b - a) * uniform();
}

size_t equilikely(size_t a, size_t b){
  return uniform(a, b + 1);
}

template<typename T>
struct entry_value__
{
  using index_t = uint64_t;

  entry_value__(index_t entry)
  : entry(entry){}

  entry_value__(index_t entry, T value)
  : entry(entry),
  value(value){}

  entry_value__(){}

  bool operator<(const entry_value__& ev) const{
    return entry < ev.entry;
  }

  index_t entry;
  T value;
};

namespace flecsi {
namespace utils {

template<size_t COUNT_BITS>
class offset__{
public:
  static_assert(COUNT_BITS <= 32, "COUNT_BITS max exceeded");

  static constexpr uint64_t count_mask = (1ul << COUNT_BITS) - 1;
  static constexpr uint32_t count_max = 1ul << COUNT_BITS;

  offset__()
  : o_(0ul){}

  offset__(uint64_t start, uint32_t count)
  : o_(start << COUNT_BITS | count){}

  offset__(const offset__& prev, uint32_t count)
  : o_(prev.end() << COUNT_BITS | count){}

  uint64_t start() const{
    return o_ >> COUNT_BITS;
  }

  uint32_t count() const{
    return o_ & count_mask;
  }

  uint64_t end() const{
    return start() + count();
  }

  void
  set_count(uint32_t count)
  {
    assert(count < count_max);
    o_ = o_ & ~count_mask | count;
  }

  void
  set_offset(uint64_t offset)
  {
    o_ = (o_ & count_mask) | (offset << COUNT_BITS);
  }

  std::pair<size_t, size_t>
  range()
  const
  {
    uint64_t s = start(); 
    return {s, s + count()};    
  }

private:
  uint64_t o_;
};

} // namespace utils
} // namespace flecsi


using namespace std;
using namespace flecsi;

using offset_t = utils::offset__<16>;

template<typename T>
class mutator{
public:
  using entry_value_t = entry_value__<T>;

  using index_t = uint64_t;

  struct partition_info_t{
    size_t count[3];
    size_t start[3];
    size_t end[3];
  };

  struct commit_info_t{
    offset_t* offsets;
    entry_value_t* entries[3];
  };

  mutator(
    size_t num_exclusive,
    size_t num_shared,
    size_t num_ghost,
    size_t max_entries_per_index,
    size_t num_slots
  )
  : num_entries_(num_exclusive + num_shared + num_ghost),
  num_exclusive_(num_exclusive),
  max_entries_per_index_(max_entries_per_index),
  num_slots_(num_slots),
  num_exclusive_insertions_(0), 
  offsets_(new offset_t[num_entries_]), 
  entries_(new entry_value_t[num_entries_ * num_slots]){
    
    pi_.count[0] = num_exclusive;
    pi_.count[1] = num_shared;
    pi_.count[2] = num_ghost;

    pi_.start[0] = 0;
    pi_.end[0] = num_exclusive;
    
    pi_.start[1] = num_exclusive;
    pi_.end[1] = num_exclusive + num_shared;
    
    pi_.start[2] = pi_.end[1];
    pi_.end[2] = pi_.end[1] + num_ghost;
  }

  T &
  operator () (
    index_t index,
    index_t entry
  )
  {
    assert(entries_ && "mutator has alread been committed");
    assert(index < num_entries_);

    offset_t& offset = offsets_[index]; 

    index_t n = offset.count();

    if(n >= num_slots_) {
      if(index < num_exclusive_){
        num_exclusive_insertions_++;
      }
      
      return spare_map_.emplace(index,
        entry_value_t(entry))->second.value;
    } // if

    entry_value_t * start = entries_ + index * num_slots_;
    entry_value_t * end = start + n;

    entry_value_t * itr =
      std::lower_bound(start, end, entry_value_t(entry),
        [](const auto & k1, const auto & k2) -> bool{
          return k1.entry < k2.entry;
        });

    // if we are creating an that has already been created, just
    // over-write the value and exit.  No need to increment the
    // counters or move data.
    if ( itr != end && itr->entry == entry) {
      return itr->value;
    }

    while(end != itr) {
      *(end) = *(end - 1);
      --end;
    } // while

    itr->entry = entry;

    if(index < num_exclusive_){
      num_exclusive_insertions_++;
    }

    offset.set_count(n + 1);

    return itr->value;
  } // operator ()

  void dump(){
    for(size_t p = 0; p < 3; ++p){
      switch(p){
        case 0:
          std::cout << "exclusive: " << std::endl;
          break;
        case 1:
          std::cout << "shared: " << std::endl;
          break;
        case 2:
          std::cout << "ghost: " << std::endl;
          break;
        default:
          break;
      }

      size_t start = pi_.start[p];
      size_t end = pi_.end[p];

      for(size_t i = start; i < end; ++i){
        const offset_t& offset = offsets_[i];
        std::cout << "  index: " << i << std::endl;
        for(size_t j = 0; j < offset.count(); ++j){
          std::cout << "    " << entries_[i + j].entry << " = " << 
            entries_[i + j].value << std::endl;
        }

        auto p = spare_map_.equal_range(i);
        auto itr = p.first;
        while(itr != p.second){
          std::cout << "    +" << itr->second.entry << " = " << 
            itr->second.value << std::endl;
          ++itr;
        }
      }      
    }
  }

  bool commit_requires_resize(size_t& size){

  }

  void init_resized(offset_t* old_offsets,
                    entry_value_t* old_entries,
                    offset_t* new_offsets,
                    entry_value_t* new_entries){

  }

  void commit(commit_info_t* ci){
    entry_value_t* cbuf = 
      new entry_value_t[num_exclusive_entries_ + num_exclusive_insertions_];

    entry_value_t* entries = ci->entries[0];
    offset_t* offsets = ci->offsets;

    entry_value_t* cptr = cbuf;
    entry_value_t* eptr = entries;

    size_t offset = 0;

    auto cmp = [](const auto & k1, const auto & k2) -> bool {
      return k1.entry < k2.entry;
    };

    for(size_t i = 0; i < num_exclusive_; ++i){
      const offset_t& oi = offsets_[i];
      offset_t& coi = offsets[i];

      size_t n1 = coi.count();
      entry_value_t* s1 = cptr;
      std::copy(eptr, eptr + n1, cptr);
      cptr += n1;
      eptr += n1;

      entry_value_t* ptr = entries_[i] * num_slots_;

      size_t n2 = oi.count();

      assert(n1 + n2 < max_entries_per_index_ && "max entries exceeded");

      entry_value_t* s2 = cptr;
      std::copy(ptr, ptr + n2, cptr);
      cptr += n2;

      std::inplace_merge(s1, s1 + n1, s2, cptr, cmp);

      auto p = spare_map_.equal_range(i);

      size_t n3 = distance(p.first, p.second);

      assert(n1 + n2 + n3 < max_entries_per_index_ && "max entries exceeded");

      for(auto itr = p.first; itr != p.second; ++itr) {
        cptr->entry = itr->second.entry;
        cptr->value = itr->second.value;
        ++cptr;
      }

      std::inplace_merge(s1, cptr, cptr, cptr + n3, cmp);

      uint32_t count = n1 + n2 + n3;

      coi.set_count(count);
      coi.set_offset(offset);

      offset += count;
    }

    std::memcpy(entries, cbuf, sizeof(entry_value_t) * (cptr - cbuf));
    delete[] cbuf;

    size_t start = num_exclusive_;
    size_t end = start + pi_.count[1] + pi_.count[2];

    for(size_t i = start; i < end; ++i){
      entry_value_t* cptr = ci->entries[1] + max_entries_per_index_ * i;

      const offset_t& oi = offsets_[i];
      offset_t& coi = offsets[i];

      entry_value_t* ptr = entries_[i] * num_slots_;

      entry_value_t* s1 = cptr;

      size_t n1 = coi.count();

      cptr += n1;

      size_t n2 = oi.count();

      assert(n1 + n2 < max_entries_per_index_ && "max entries exceeded");
      
      entry_value_t* s2 = cptr;
      std::copy(ptr, ptr + n2, cptr);
      cptr += n2;
      std::inplace_merge(s1, s1 + n1, s2, cptr, cmp);

      auto p = spare_map_.equal_range(i);

      size_t n3 = distance(p.first, p.second);

      assert(n1 + n2 + n3 < max_entries_per_index_ && "max entries exceeded");

      for(auto itr = p.first; itr != p.second; ++itr) {
        cptr->entry = itr->second.entry;
        cptr->value = itr->second.value;
        ++cptr;
      }

      std::inplace_merge(s1, cptr, cptr, cptr + n3, cmp);
    }
  }

private:
  using spare_map_t = std::multimap<size_t, entry_value__<T>>;

  partition_info_t pi_;
  size_t num_exclusive_;
  size_t max_entries_per_index_;
  size_t num_exclusive_insertions_;
  size_t num_slots_;
  size_t num_entries_;
  size_t num_exclusive_entries_;
  offset_t* offsets_;
  entry_value_t* entries_;
  entry_value_t* shared_entries_;
  spare_map_t spare_map_;
};

int main(int argc, char** argv){
  mutator<double> m(2, 1, 1, 3, 2);
  m(0, 10) = 56.5;
  m(0, 2) = 1.25;
  m(0, 1) = 3.21;

  m.dump();

  return 0;
}
