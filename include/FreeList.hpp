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


    template <typename Compare = std::less<T>>
    size_t mergeIterative(size_t left, size_t right, size_t& tail_out, const Compare& comp = Compare()) {
        if (left == SIZE_MAX) return right;
        if (right == SIZE_MAX) return left;
        
        // Create a dummy head to simplify the implementation
        size_t dummy_head = SIZE_MAX;
        size_t* current = &dummy_head;
        
        size_t left_ptr = left;
        size_t right_ptr = right;
        
        while (left_ptr != SIZE_MAX && right_ptr != SIZE_MAX) {
		if (comp(nodes[left_ptr].data, nodes[right_ptr].data)) {
		    *current = left_ptr;
		    current = &nodes[left_ptr].next;
		    left_ptr = nodes[left_ptr].next;
		} else {
		    *current = right_ptr;
		    current = &nodes[right_ptr].next;
		    right_ptr = nodes[right_ptr].next;
		}
        }
        
        // Attach the remaining list
        *current = (left_ptr != SIZE_MAX) ? left_ptr : right_ptr;
        
        // Fix prev pointers and find the tail
        size_t result = dummy_head;
        size_t prev = SIZE_MAX;
        size_t curr = result;
        
        while (curr != SIZE_MAX) {
		nodes[curr].prev = prev;
		prev = curr;
		curr = nodes[curr].next;
        }
        
        tail_out = prev;
        return dummy_head;
    }

    template <typename Compare = std::less<T>>
    void bottomUpMergeSortRange(size_t start_idx, size_t end_idx, const Compare& comp = Compare()) {
        if (empty() || start_idx == SIZE_MAX || (start_idx == end_idx && end_idx != SIZE_MAX)) return;
        
        // If end_idx is SIZE_MAX, we want to sort to the end of the list
        if (end_idx == SIZE_MAX) {
            end_idx = tail;
        }
        
        // Save connections to the rest of the list
        size_t before_range = nodes[start_idx].prev;
        size_t after_range = nodes[end_idx].next;
        
        // Disconnect the range
        if (before_range != SIZE_MAX) {
            nodes[before_range].next = SIZE_MAX;
        }
        nodes[start_idx].prev = SIZE_MAX;
        
        if (after_range != SIZE_MAX) {
            nodes[after_range].prev = SIZE_MAX;
        }
        nodes[end_idx].next = SIZE_MAX;
        
        // Count elements in our range
        size_t range_size = 1;
        size_t curr = start_idx;
        while (curr != end_idx) {
            range_size++;
            curr = nodes[curr].next;
        }
        
        // Bottom-up merge sort on the isolated range
        for (size_t sublist_size = 1; sublist_size < range_size; sublist_size *= 2) {
            size_t current_head = SIZE_MAX;
            size_t current_tail = SIZE_MAX;
            
            size_t remaining = start_idx;
            
            while (remaining != SIZE_MAX) {
                // Extract first sublist
                size_t list1_head = remaining;
                size_t list1_tail = list1_head;
                
                for (size_t i = 1; i < sublist_size && nodes[list1_tail].next != SIZE_MAX; ++i) {
                    list1_tail = nodes[list1_tail].next;
                }
                
                remaining = nodes[list1_tail].next;
                
                if (remaining != SIZE_MAX) {
                    nodes[list1_tail].next = SIZE_MAX;
                    nodes[remaining].prev = SIZE_MAX;
                }
                
                if (remaining == SIZE_MAX) {
                    if (current_head == SIZE_MAX) {
                        current_head = list1_head;
                        current_tail = list1_tail;
                    } else {
                        nodes[current_tail].next = list1_head;
                        nodes[list1_head].prev = current_tail;
                        current_tail = list1_tail;
                    }
                    continue;
                }
                
                // Extract second sublist
                size_t list2_head = remaining;
                size_t list2_tail = list2_head;
                
                for (size_t i = 1; i < sublist_size && nodes[list2_tail].next != SIZE_MAX; ++i) {
                    list2_tail = nodes[list2_tail].next;
                }
                
                remaining = nodes[list2_tail].next;
                
                if (remaining != SIZE_MAX) {
                    nodes[list2_tail].next = SIZE_MAX;
                    nodes[remaining].prev = SIZE_MAX;
                }
                
                // Merge the two sublists
                size_t merged_tail = SIZE_MAX;
                size_t merged_head = mergeIterative(list1_head, list2_head, merged_tail, comp);
                
                if (current_head == SIZE_MAX) {
                    current_head = merged_head;
                    current_tail = merged_tail;
                } else {
                    nodes[current_tail].next = merged_head;
                    nodes[merged_head].prev = current_tail;
                    current_tail = merged_tail;
                }
            }
            
            start_idx = current_head;
            end_idx = current_tail;
        }
        
        // Reconnect the sorted range
        if (before_range != SIZE_MAX) {
            nodes[before_range].next = start_idx;
            nodes[start_idx].prev = before_range;
        } else {
            head = start_idx;
        }
        
        if (after_range != SIZE_MAX) {
            nodes[end_idx].next = after_range;
            nodes[after_range].prev = end_idx;
        } else {
            tail = end_idx;
        }
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
    
    // Overload the constructor for the case where Iterator's value_type is the same as T
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

    FreeList(const FreeList& other) = default;
    FreeList(FreeList&& other) noexcept = default;
    FreeList& operator=(const FreeList& other) = default;
    FreeList& operator=(FreeList&& other) noexcept = default;


    template <typename Compare = std::less<T> >
    void sort(const Compare& comp = Compare()) {
        if (empty()) return;

	bottomUpMergeSortRange(head, tail, comp);
    }

    template <typename Compare = std::less<T> >
    void sort(const const_iterator start,
	      const const_iterator _end,
	      const Compare& comp = Compare())
    {
        if (empty() || start == end() || start == _end) return;

        size_t start_idx = start.getIndex();
        size_t end_idx = (_end == end()) ? tail : _end.prev().getIndex();

	bottomUpMergeSortRange(start_idx, end_idx, comp);
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
