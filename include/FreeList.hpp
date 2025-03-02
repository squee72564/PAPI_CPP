#ifndef FREELIST_HPP
#define FREELIST_HPP

#include <vector>
#include <iterator>
#include <limits>
#include <cstddef>
#include <cstdint>
#include <execution>

template<typename T>
class FreeList {
private:
    // Structure of Arrays (SoA) approach
    std::vector<size_t> next_indices;  // Linked list "next" pointers
    std::vector<size_t> prev_indices;  // Linked list "prev" pointers
    std::vector<size_t> next_free;     // Free list management
    std::vector<T> data;               // Actual data values
    
    // List endpoints and size
    size_t head;
    size_t tail;
    size_t freeHead;
    size_t size_;

    // Helper methods
    template <typename U>
    size_t allocateNode(U&& value) {
        size_t index;

        if (freeHead != SIZE_MAX) {
            // Reuse a freed node
            index = freeHead;
            freeHead = next_free[freeHead];
            data[index] = std::forward<U>(value);
            next_indices[index] = SIZE_MAX;
            prev_indices[index] = SIZE_MAX;
        } else {
            // Allocate new node
            index = data.size();
            data.emplace_back(std::forward<U>(value));
            next_indices.push_back(SIZE_MAX);
            prev_indices.push_back(SIZE_MAX);
            next_free.push_back(SIZE_MAX);
        }

        size_++;
        return index;
    }
    
    void remove(size_t index) {
        if (index >= data.size()) return;

        // Update linked list pointers
        size_t nextIndex = next_indices[index];
        size_t prevIndex = prev_indices[index];

        if (prevIndex == SIZE_MAX) {
            head = nextIndex;
        } else {
            next_indices[prevIndex] = nextIndex;
        }

        if (nextIndex == SIZE_MAX) {
            tail = prevIndex;
        } else {
            prev_indices[nextIndex] = prevIndex;
        }

        // Add to free list
        next_free[index] = freeHead;
        freeHead = index;

        size_--;
    }

public:
    using value_type = T;

    class Iterator {
        friend class FreeList;
        friend class ConstIterator;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;

        Iterator() : list(nullptr), index(SIZE_MAX) {}

        Iterator(FreeList* list, size_t index)
            : list(list), index(index) {}

        Iterator(const FreeList* list, size_t index)
            : list(const_cast<FreeList*>(list)), index(index) {}

        Iterator(const Iterator&) = default;
        Iterator(Iterator&&) noexcept = default;
        Iterator& operator=(const Iterator&) = default;
        Iterator& operator=(Iterator&&) noexcept = default;

        reference operator*() {
            return list->data[index];
        }

        reference operator*() const {
            return list->data[index];
        }

        pointer operator->() {
            return &list->data[index];
        }

        pointer operator->() const {
            return &list->data[index];
        }

        Iterator& operator++() {
            index = list->next_indices[index];
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }

        Iterator& operator--() {
            if (index == SIZE_MAX) {
                index = list->tail;
            } else {
                index = list->prev_indices[index];
            }
            return *this;
        }

        Iterator operator--(int) {
            Iterator temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const Iterator& other) const {
            return (index == other.index) && (list == other.list);
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        size_t getIndex() const {
            return index;
        }

        FreeList* getList() const {
            return list;
        }
        
        FreeList* list;
        size_t index;
    };

    class ConstIterator {
        friend class FreeList;
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = const T*;
        using reference = const T&;

        ConstIterator() : list(nullptr), index(SIZE_MAX) {}

        ConstIterator(const FreeList* list, size_t index)
            : list(list), index(index) {}

        ConstIterator(const Iterator& it)
            : list(it.getList()), index(it.getIndex()) {}

        ConstIterator(const ConstIterator&) = default;
        ConstIterator(ConstIterator&&) noexcept = default;
        ConstIterator& operator=(const ConstIterator&) = default;
        ConstIterator& operator=(ConstIterator&&) noexcept = default;

        reference operator*() const {
            return list->data[index];
        }

        pointer operator->() const {
            return &list->data[index];
        }

        ConstIterator& operator++() {
            index = list->next_indices[index];
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator temp = *this;
            ++(*this);
            return temp;
        }

        ConstIterator& operator--() {
            if (index == SIZE_MAX) {
                index = list->tail;
            } else {
                index = list->prev_indices[index];
            }
            return *this;
        }

        ConstIterator operator--(int) {
            ConstIterator temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const ConstIterator& other) const {
            return (index == other.index) && (list == other.list);
        }

        bool operator!=(const ConstIterator& other) const {
            return !(*this == other);
        }

    private:
        size_t getIndex() const {
            return index;
        }

        const FreeList* list;
        size_t index;
    };

    using iterator = Iterator;
    using const_iterator = ConstIterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructors
    FreeList() : next_indices(), prev_indices(), next_free(), data(), 
                 head(SIZE_MAX), tail(SIZE_MAX), freeHead(SIZE_MAX), size_(0) {}

    FreeList(size_t count) : FreeList() {
        reserve(count);
        for (size_t i = 0; i < count; ++i) {
            emplace_back(T{});
        }
    }

    template <typename U>
    FreeList(size_t count, U&& value) : FreeList() {
        reserve(count);
        for (size_t i = 0; i < count; ++i) {
            emplace_back(std::forward<U>(value));
        }
    }

    template <typename Iterator>
    FreeList(
        Iterator first,
        Iterator last,
        typename std::enable_if<!std::is_same<typename std::iterator_traits<Iterator>::value_type, T>::value>::type* = nullptr)
        : FreeList()
    {
        auto n = std::distance(first, last);
        reserve(n);
        for (auto it = first; it != last; ++it) {
            emplace_back(*it);
        }
    }
    
    template <typename Iterator>
    FreeList(
        Iterator first,
        Iterator last,
        typename std::enable_if<std::is_same<typename std::iterator_traits<Iterator>::value_type, T>::value>::type* = nullptr)
        : FreeList()
    {
        auto n = std::distance(first, last);
        reserve(n);
        for (auto it = first; it != last; ++it) {
            emplace_back(*it);
        }
    }

    FreeList(std::initializer_list<T> init) : FreeList() {
        reserve(init.size());
        for (const auto& value : init) {
            emplace_back(value);
        }
    }

    ~FreeList() = default;

    // Copy and move operations
    FreeList(const FreeList& other) 
        : next_indices(other.next_indices), prev_indices(other.prev_indices), 
          next_free(other.next_free), data(other.data),
          head(other.head), tail(other.tail), freeHead(other.freeHead), size_(other.size_) {}

    FreeList(FreeList&& other) noexcept
        : next_indices(std::move(other.next_indices)), prev_indices(std::move(other.prev_indices)), 
          next_free(std::move(other.next_free)), data(std::move(other.data)),
          head(other.head), tail(other.tail), freeHead(other.freeHead), size_(other.size_) {
        other.head = other.tail = other.freeHead = SIZE_MAX;
        other.size_ = 0;
    }

    FreeList& operator=(const FreeList& other) {
        if (this != &other) {
            next_indices = other.next_indices;
            prev_indices = other.prev_indices;
            next_free = other.next_free;
            data = other.data;
            head = other.head;
            tail = other.tail;
            freeHead = other.freeHead;
            size_ = other.size_;
        }
        return *this;
    }

    FreeList& operator=(FreeList&& other) noexcept {
        if (this != &other) {
            next_indices = std::move(other.next_indices);
            prev_indices = std::move(other.prev_indices);
            next_free = std::move(other.next_free);
            data = std::move(other.data);
            head = other.head;
            tail = other.tail;
            freeHead = other.freeHead;
            size_ = other.size_;
            
            other.head = other.tail = other.freeHead = SIZE_MAX;
            other.size_ = 0;
        }
        return *this;
    }

    // Iterator methods
    iterator begin() { return iterator(this, head); }
    const_iterator begin() const { return const_iterator(this, head); }
    const_iterator cbegin() const noexcept { return const_iterator(this, head); }

    iterator end() { return iterator(this, SIZE_MAX); }
    const_iterator end() const { return const_iterator(this, SIZE_MAX); }
    const_iterator cend() const noexcept { return const_iterator(this, SIZE_MAX); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    // Element access
    const T& front() const { return data[head]; }
    const T& back() const { return data[tail]; }
    T& front() { return data[head]; }
    T& back() { return data[tail]; }

    // Capacity
    bool empty() const noexcept { return head == SIZE_MAX && tail == SIZE_MAX; }
    size_t size() const noexcept { return size_; }
    size_t capacity() const noexcept { return data.capacity(); }

    void reserve(size_t count) {
        data.reserve(count);
        next_indices.reserve(count);
        prev_indices.reserve(count);
        next_free.reserve(count);
    }

    void shrink_to_fit() {
        data.shrink_to_fit();
        next_indices.shrink_to_fit();
        prev_indices.shrink_to_fit();
        next_free.shrink_to_fit();
    }

    void clear() {
        head = tail = freeHead = SIZE_MAX;
        size_ = 0;
        data.clear();
        next_indices.clear();
        prev_indices.clear();
        next_free.clear();
    }

    // Modifiers
    template <typename U>
    void push_front(U&& value) {
        size_t index = allocateNode(std::forward<U>(value));
        if (head != SIZE_MAX) {
            next_indices[index] = head;
            prev_indices[head] = index;
        }
        head = index;
        if (tail == SIZE_MAX) {
            tail = index;
        }
    }

    template <typename U>
    void push_back(U&& value) {
        size_t index = allocateNode(std::forward<U>(value));
        if (head == SIZE_MAX) {
            head = index;
            tail = index;
        } else {
            next_indices[tail] = index;
            prev_indices[index] = tail;
            tail = index;
        }
    }

    template<class... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        T value(std::forward<Args>(args)...);
        
        if (pos == end()) {
            size_t index = allocateNode(std::move(value));
            if (tail != SIZE_MAX) {
                next_indices[tail] = index;
                prev_indices[index] = tail;
            } else {
                head = index;
            }
            tail = index;
            return iterator(this, index);
        }

        size_t currentIndex = pos.getIndex();
        size_t newIndex = allocateNode(std::move(value));

        next_indices[newIndex] = currentIndex;
        prev_indices[newIndex] = prev_indices[currentIndex];

        if (prev_indices[currentIndex] != SIZE_MAX) {
            next_indices[prev_indices[currentIndex]] = newIndex;
        } else {
            head = newIndex;
        }

        prev_indices[currentIndex] = newIndex;
        return iterator(this, newIndex);
    }

    template<typename... Args>
    T& emplace_back(Args&&... args) {
        T value(std::forward<Args>(args)...);
        size_t index = allocateNode(std::move(value));
        
        if (head == SIZE_MAX) {
            head = index;
            tail = index;
        } else {
            next_indices[tail] = index;
            prev_indices[index] = tail;
            tail = index;
        }
        
        return data[index];
    }

    iterator erase(iterator pos) {
        iterator next = pos;
        ++next;
        remove(pos.getIndex());
        return next;
    }

    iterator erase(const_iterator pos) {
        const_iterator next = pos;
        ++next;
        remove(pos.getIndex());
        return iterator(this, next.getIndex());
    }

    iterator erase(iterator first, iterator last) {
        while (first != last) {
            first = erase(first);
        }
        return last;
    }

    iterator erase(const_iterator first, const_iterator last) {
        while (first != last) {
            const_iterator current = first++;
            remove(current.getIndex());
        }
        return iterator(this, last.getIndex());
    }

    // Fixed insert functions
    template <typename U>
    iterator insert(const_iterator it, U&& value) {
        if (!(it == end())) {
            size_t currentIndex = it.getIndex();
            size_t newIndex = allocateNode(std::forward<U>(value));
            
            next_indices[newIndex] = currentIndex;
            prev_indices[newIndex] = prev_indices[currentIndex];

            if (prev_indices[currentIndex] != SIZE_MAX) {
                next_indices[prev_indices[currentIndex]] = newIndex;
            } else {
                head = newIndex;
            }
            
            prev_indices[currentIndex] = newIndex;
            return iterator(this, newIndex);
        } else {
            size_t newIndex = allocateNode(std::forward<U>(value));
            
            if (tail != SIZE_MAX) {
                next_indices[tail] = newIndex;
                prev_indices[newIndex] = tail;
            } else {
                head = newIndex;
            }
            
            tail = newIndex;
            return iterator(this, newIndex);
        }
    }

    // Add explicit implementation for const T& 
    iterator insert(const_iterator it, const T& value) {
        return insert<const T&>(it, value);
    }

    // Add direct support for brace initialization for pairs
    template <typename First, typename Second>
    iterator insert(const_iterator it, const std::pair<First, Second>& value) {
        return insert<const std::pair<First, Second>&>(it, value);
    }

    // Add direct support for brace initialization in general
    template <typename... Args>
    iterator insert(const_iterator it, Args&&... args) {
        return emplace(it, std::forward<Args>(args)...);
    }

    template<class InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        if (first == last) return iterator(this, pos.getIndex());
        
        size_t firstNewIndex = SIZE_MAX;
        size_t currentIndex = pos.getIndex();
        
        for (auto it = first; it != last; ++it) {
            size_t newIndex = allocateNode(*it);
            
            if (firstNewIndex == SIZE_MAX) {
                firstNewIndex = newIndex;
            }
            
            if (currentIndex != SIZE_MAX) {
                // Insert before currentIndex
                next_indices[newIndex] = currentIndex;
                prev_indices[newIndex] = prev_indices[currentIndex];
                
                if (prev_indices[currentIndex] != SIZE_MAX) {
                    next_indices[prev_indices[currentIndex]] = newIndex;
                } else {
                    head = newIndex;
                }
                
                prev_indices[currentIndex] = newIndex;
            } else {
                // Insert at end
                if (tail != SIZE_MAX) {
                    next_indices[tail] = newIndex;
                    prev_indices[newIndex] = tail;
                } else {
                    head = newIndex;
                }
                tail = newIndex;
            }
        }
        
        return iterator(this, firstNewIndex);
    }

    iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    void pop_front() {
        if (head == SIZE_MAX) return;
        remove(head);
    }

    void pop_back() {
        if (tail == SIZE_MAX) return;
        remove(tail);
    }

    void swap(FreeList& other) noexcept {
        std::swap(head, other.head);
        std::swap(tail, other.tail);
        std::swap(freeHead, other.freeHead);
        std::swap(size_, other.size_);
        next_indices.swap(other.next_indices);
        prev_indices.swap(other.prev_indices);
        next_free.swap(other.next_free);
        data.swap(other.data);
    }

    // Find
    iterator find(const T& value) {
        for (iterator it = begin(); it != end(); ++it) {
            if (*it == value) {
                return it;
            }
        }
        return end();
    }

    // Sorting
    template <typename Compare>
    void sort_impl(size_t start_idx, size_t end_idx, Compare& comp) {
        // Collect values and indices for the range
        std::vector<std::pair<T, size_t>> values_with_indices;
        values_with_indices.reserve(size_);
        
        for (size_t curr = start_idx; curr != end_idx; curr = next_indices[curr]) {
            values_with_indices.emplace_back(std::move(data[curr]), curr);
        }
        
        if (values_with_indices.size() <= 1) return;
        
        // Sort by value using std::sort
        std::sort(values_with_indices.begin(), values_with_indices.end(),
                [&comp](const auto& a, const auto& b) {
                    return comp(a.first, b.first);
                });
        
        // Restore values to original nodes in sorted order
        size_t i = 0;
        for (size_t curr = start_idx; curr != end_idx; curr = next_indices[curr]) {
            auto& [value, original_idx] = values_with_indices[i++];
            data[curr] = std::move(value);
        }
    }

    template <typename Compare = std::less<T>>
    void sort(const Compare& comp = Compare()) {
        if (empty() || size_ <= 1) return;
        sort_impl(head, SIZE_MAX, comp);
    }

    template <typename Compare = std::less<T>>
    void sort(const_iterator start, const_iterator end, const Compare& comp = Compare()) {
        if (empty() || start == this->end() || start == end) return;
        
        size_t start_idx = start.getIndex();
        size_t end_idx = (end == this->end()) ? SIZE_MAX : end.getIndex();
        
        sort_impl(start_idx, end_idx, comp);
    }
};

#endif
