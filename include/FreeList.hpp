#ifndef FREELIST_HPP
#define FREELIST_HPP

#include <vector>
#include <iterator>
#include <limits>
#include <cstddef>
#include <cstdint>

template<typename T>
class FreeList {
private:
    struct Node {
        T data;
        size_t next;
        size_t prev;
        size_t nextFree;

        Node(const T& data) : data(data), next(SIZE_MAX), prev(SIZE_MAX), nextFree(SIZE_MAX) {}
        Node(T&& data) : data(std::move(data)), next(SIZE_MAX), prev(SIZE_MAX), nextFree(SIZE_MAX) {}
        Node() : data(T{}), next(SIZE_MAX), prev(SIZE_MAX), nextFree(SIZE_MAX) {}
        ~Node() = default;

        Node(const Node&) = default;
        Node(Node&&) noexcept = default;
        Node& operator=(const Node&) = default;
        Node& operator=(Node&&) noexcept = default;
    };

    std::vector<Node> nodes;
    size_t head;
    size_t tail;
    size_t freeHead;
    size_t size_;

    template <typename U>
    size_t allocateNode(U&& data) {
        size_t index;

        if (freeHead != SIZE_MAX) {
            index = freeHead;
            freeHead = nodes[freeHead].nextFree;
            nodes[index] = Node(std::forward<T>(data));
        } else {
            index = nodes.size();
            nodes.emplace_back(std::forward<T>(data));
        }

        size_++;
        return index;
    }

    size_t allocateNode(const T& data) {
        size_t index;

        if (freeHead != SIZE_MAX) {
            index = freeHead;
            freeHead = nodes[freeHead].nextFree;
            nodes[index] = Node(data);
        } else {
            index = nodes.size();
            nodes.emplace_back(data);
        }

        size_++;

        return index;
    }

    void remove(size_t index) {
        if (index >= nodes.size()) return;

        size_t nextIndex = nodes[index].next;
        size_t prevIndex = nodes[index].prev;

        if (prevIndex == SIZE_MAX) {
            head = nextIndex;
        } else {
            nodes[prevIndex].next = nextIndex;
        }

        if (nextIndex == SIZE_MAX) {
            tail = prevIndex;
        } else {
            nodes[nextIndex].prev = prevIndex;
        }

        nodes[index].nextFree = freeHead;
        freeHead = index;

        size_--;
    }

    template <typename Compare = std::less<T> >
    size_t merge(const size_t first, const size_t second, size_t& tail, const Compare& comp = Compare()) {
        if (first == SIZE_MAX) return second;
        if (second == SIZE_MAX) return first;

        size_t result = SIZE_MAX;

        if (comp(nodes[first].data, nodes[second].data)) {
            result = first;

            nodes[first].next = merge(nodes[first].next, second, tail, comp);
            if (nodes[first].next != SIZE_MAX) {
                nodes[nodes[first].next].prev = first;
            } else {
                tail = first;
            }
            nodes[first].prev = SIZE_MAX;
        } else {
            result = second;

            nodes[second].next = merge(first, nodes[second].next, tail, comp);
            if (nodes[second].next != SIZE_MAX) {
                nodes[nodes[second].next].prev = second;
            } else {
                tail = second;
            }
            nodes[second].prev = SIZE_MAX;
        }
        return result;
    }

    std::pair<size_t,size_t> split(const size_t start, const size_t end) {
        if (start == end) return {start, start};

        size_t slow = start;
        size_t fast = start;

        while (fast != end && nodes[fast].next != end) {
            slow = nodes[slow].next;
            fast = nodes[nodes[fast].next].next;
        }

        size_t second_half = nodes[slow].next;
        nodes[slow].next = SIZE_MAX;

        if (second_half != SIZE_MAX) {
            nodes[second_half].prev = SIZE_MAX;
        }
        return {second_half,slow};
    }

    template <typename Compare = std::less<T> >
    size_t mergeSort(size_t start, size_t end, const Compare& comp = Compare()) {
        if (start == SIZE_MAX || start == end) return start;

        auto [second_half_start, start_end] = split(start, end);

        start = mergeSort(start, start_end, comp);
        second_half_start = mergeSort(second_half_start, end, comp);

        size_t _tail = SIZE_MAX;
        size_t _head = merge(start, second_half_start, _tail, comp);
        tail = _tail;

        return _head;
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
            return list->nodes[index].data;
        }

        reference operator*() const {
            return list->nodes[index].data;
        }

        pointer operator->() {
            return &list->nodes[index].data;
        }

        pointer operator->() const {
            return &list->nodes[index].data;
        }


        Iterator& operator++() {
            index = list->nodes[index].next;
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
            index = list->nodes[index].prev;
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
	    return list->nodes[index].data;
	}

	pointer operator->() const {
	    return &list->nodes[index].data;
	}

	ConstIterator& operator++() {
	    index = list->nodes[index].next;
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
            index = list->nodes[index].prev;
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

    FreeList() : nodes(), head(SIZE_MAX), tail(SIZE_MAX), freeHead(SIZE_MAX), size_(0) {}

    FreeList(size_t count) : FreeList() {
        for (size_t i = 0; i < count; ++i) {
            push_back(T{});
        }
    }

    FreeList(size_t count, const T& value) : FreeList() {
        for (size_t i = 0; i < count; ++i) {
            push_back(value);
        }
    }

    template <typename Iterator>
    FreeList(Iterator first, Iterator last) : FreeList() {
        for (auto it = first; it != last; ++it) {
            push_back(*it);
        }
    }

    //FreeList(const_iterator first, const_iterator last) : FreeList() {
    //    for (auto it = first; it != last; ++it) {
    //        push_back(*it);
    //    }
    //}

    FreeList(std::initializer_list<T> init) : FreeList() {
        for (const auto& value : init) {
            push_back(value);
        }
    }

    ~FreeList() = default;

    FreeList(const FreeList& other) = default;
    FreeList(FreeList&& other) noexcept = default;
    FreeList& operator=(const FreeList& other) = default;
    FreeList& operator=(FreeList&& other) noexcept = default;

    template <typename Compare = std::less<T> >
    void sort(const Compare& comp = Compare()) {
        if (empty()) return;

        head = mergeSort(head, tail, comp);
    }

    template <typename Compare = std::less<T> >
    void sort(const const_iterator start,
	      const const_iterator _end,
	      const Compare& comp = Compare())
    {
        if (empty() || start == end() || start == _end) return;

        iterator actualStart = (start == const_iterator()) ? cbegin() : start;
        iterator actualEnd = (_end == const_iterator()) ? cend() : _end;

        size_t start_idx = actualStart.getIndex();
        size_t end_idx = (actualEnd == cend()) ? tail : _end.getIndex();

        head = mergeSort(start_idx, end_idx, comp);
    }

    void reserve(size_t count) {
        nodes.reserve(count);
    }

    template <typename U>
    void push_front(U&& data) {
        size_t index = allocateNode(std::forward<U>(data));
    
        if (head != SIZE_MAX) {
            nodes[index].next = head;
            nodes[head].prev = index;
        }

        head = index;

        if (tail == SIZE_MAX) {
            tail = index;
        }
    }
    
    template <typename U>
    void push_back(U&& data) {
        size_t index = allocateNode(std::forward<U>(data));
    
        if (head == SIZE_MAX) {
            head = index;
            tail = index;
        } else {
            nodes[tail].next = index;
            nodes[index].prev = tail;
            tail = index;
        }
    }

    template<class... Args>
    iterator emplace(const_iterator pos, Args&&... args) {

        if (pos == end()) {
            return insert(end(), T(std::forward<Args>(args)...));
        }

        size_t currentIndex = pos.getIndex();

        size_t newIndex = allocateNode(T(std::forward<Args>(args)...));

        nodes[newIndex].next = currentIndex;
        nodes[newIndex].prev = nodes[currentIndex].prev;

        if (nodes[currentIndex].prev != SIZE_MAX) {
            nodes[nodes[currentIndex].prev].next = newIndex;
        } else {
            head = newIndex;
        }

        nodes[currentIndex].prev = newIndex;

        return iterator(this, newIndex);
    }

    template<typename... Args>
    T& emplace_back(Args&&... args) {
        size_t index = allocateNode(T(std::forward<Args>(args)...));

        if (head == SIZE_MAX) {
            head = index;
            tail = index;
        } else {
            nodes[tail].next = index;
            nodes[index].prev = tail;
            tail = index;
        }

        return nodes[index].data;
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
            first = erase(first);
        }

        return iterator(this, last.getIndex());
    }

    iterator find(const T& value) {
        for (iterator it = begin(); it != end(); ++it) {
            if (*it == value) {
            return it;
            }
        }
        return end();
    }

    template <typename U>
    iterator insert(const_iterator it, U&& data) {
        size_t newIndex = allocateNode(std::forward<U>(data));
    
        if (!(it == end())) {
            size_t currentIndex = it.getIndex();
    
            nodes[newIndex].next = currentIndex;
            nodes[newIndex].prev = nodes[currentIndex].prev;
    
            if (nodes[currentIndex].prev != SIZE_MAX) {
                nodes[nodes[currentIndex].prev].next = newIndex;
            } else {
                head = newIndex;
            }
    
            nodes[currentIndex].prev = newIndex;
        } else {

            if (tail != SIZE_MAX) {
                nodes[tail].next = newIndex;
                nodes[newIndex].prev = tail;
            } else {
                head = newIndex;
            }
            tail = newIndex;
        }
    
        return iterator(this, newIndex);
    }

    iterator insert(const_iterator it, const T& data) {
        size_t newIndex = allocateNode(data);
    
        if (!(it == end())) {
            size_t currentIndex = it.getIndex();
    
            nodes[newIndex].next = currentIndex;
            nodes[newIndex].prev = nodes[currentIndex].prev;
    
            if (nodes[currentIndex].prev != SIZE_MAX) {
                nodes[nodes[currentIndex].prev].next = newIndex;
            } else {
                head = newIndex;
            }
    
            nodes[currentIndex].prev = newIndex;
        } else {

            if (tail != SIZE_MAX) {
                nodes[tail].next = newIndex;
                nodes[newIndex].prev = tail;
            } else {
                head = newIndex;
            }
            tail = newIndex;
        }
    
        return iterator(this, newIndex);
    }

    template<class InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        size_t currentIndex = pos.getIndex();
        size_t firstNewIndex = SIZE_MAX;

        while (first != last) {
            size_t newIndex = allocateNode(*first++);

            if (firstNewIndex == SIZE_MAX) {
                firstNewIndex = newIndex;
            }

            if (currentIndex != SIZE_MAX) {
                nodes[newIndex].next = currentIndex;
                nodes[newIndex].prev = nodes[currentIndex].prev;

                if (nodes[currentIndex].prev != SIZE_MAX) {
                    nodes[nodes[currentIndex].prev].next = newIndex;
                } else {
                    head = newIndex;
                }

                nodes[currentIndex].prev = newIndex;
                currentIndex = newIndex;
            } else {
                if (tail != SIZE_MAX) {
                    nodes[tail].next = newIndex;
                    nodes[newIndex].prev = tail;
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

    void swap(FreeList& other) noexcept {
        std::swap(head, other.head);
        std::swap(tail, other.tail);
        std::swap(freeHead, other.freeHead);
        nodes.swap(other.nodes);
        std::swap(size_, other.size_);
    }

    const T& front() const {
        return nodes[head].data;
    }

    const T& back() const {
        return nodes[tail].data;
    }

    T& front() {
        return nodes[head].data;
    }

    T& back() {
        return nodes[tail].data;
    }

    void pop_front() {
        if (head == SIZE_MAX) return;
    
        remove(head);
    }

    void pop_back() {
        if (tail == SIZE_MAX) { 
            return;
        }
    
        remove(tail);
    }

    bool empty() const noexcept {
        return head == SIZE_MAX && tail == SIZE_MAX;
    }

    size_t size() const noexcept {
        return size_;
    }

    size_t capacity() const noexcept {
        return nodes.capacity();
    }

    void shrink_to_fit() {
        nodes.shrink_to_fit();
    }

    void clear() {
        head = tail = freeHead = SIZE_MAX;
        size_ = 0;
        nodes.clear();
    }
};

#endif
