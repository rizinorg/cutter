#ifndef LINKED_LIST_POOL_H
#define LINKED_LIST_POOL_H

#include <vector>
#include <cstdint>
#include <iterator>

/**
 * @brief Pool of singly linked lists.
 *
 *  Should not be used as general purpose container. Use only for algorithms that require linked lists ability
 * to split and concatenate them. All the data is owned by LinkedListPool.
 *
 * In contrast to std::list and std::forward_list doesn't allocate each node separately. LinkedListPool can reserve
 * all the memory for multiple lists during construction. Uses std::vector as backing container.
 */
template<class T>
class LinkedListPool
{
    using IndexType = size_t;
    struct Item {
        IndexType next;
        T value;
    };
public:
    /**
     * @brief List iterator.
     *
     * Iterators don't get invalidated by adding items to list, but the items may be relocated.
     */
    class ListIterator
    {
        IndexType index = 0;
        LinkedListPool<T> *pool = nullptr;
        ListIterator(IndexType index, LinkedListPool<T> *pool)
            : index(index)
            , pool(pool)
        {}

        friend class LinkedListPool<T>;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = size_t;
        using pointer = T*;
        using reference = T&;
        ListIterator() = default;
        reference operator*()
        {
            return pool->data[index].value;
        }
        pointer operator->()
        {
            return &pool->data[index].value;
        }

        ListIterator &operator++()
        {
            index = pool->data[index].next;
            return *this;
        }
        ListIterator operator++(int)
        {
            ListIterator tmp(*this);
            operator++();
            return tmp;
        }
        bool operator!=(const ListIterator &b) const
        {
            return index != b.index || pool != b.pool;
        };
        /**
         * @brief Test if iterator points to valid value.
         */
        explicit operator bool() const
        {
            return index;
        }
    };

    /**
     * @brief Single list within LinkedListPool.
     *
     * List only refers to chain of elements. Copying it doesn't copy any element. Item data is owned by
     * LinkedListPool.
     *
     * Use LinkedListPool::makeList to create non-empty list.
     */
    class List
    {
        IndexType head = 0;
        IndexType tail = 0;
        friend class LinkedListPool;
        List(IndexType head, IndexType tail)
            : head(head)
            , tail(tail)
        {}
    public:
        /**
         * @brief Create an empty list
         */
        List() = default;

        bool isEmpty() const
        {
            return head == 0;
        }
    };

    /**
     * @brief Create a linked list pool with capacity for \a initialCapacity list items.
     * @param initialCapacity number of elements to preallocate.
     */
    LinkedListPool(size_t initialCapacity)
        : data(1)
    {
        data.reserve(initialCapacity + 1); // [0] element reserved
    }

    /**
     * @brief Create a list containing single item.
     *
     * Does not invalidate any iterators, but may cause item relocation when initialCapacity is exceeded.
     * @param value value of element that will be inserted in the created list
     * @return List containing single value \a value .
     */
    List makeList(const T &value)
    {
        size_t position = data.size();
        data.push_back(Item{0, value});
        return {position, position};
    }

    /**
     * @brief Split list and return second half.
     *
     * After performing the operation, list passed as argument and return list point to the same items. Modifying them
     * will affect both lists.
     *
     * @param list The list that needs to be split.
     * @param head Iterator to the first item in new list. Needs to be within \a list .
     * @return Returns suffix of \a list.
     */
    List splitTail(const List &list, const ListIterator &head)
    {
        return List {head.index, list.tail};
    }

    /**
     * @brief Split list and return first half.
     *
     * @param list The list that needs to be split.
     * @param end Iterator to the first item that should not be included in returned list. Needs to be within \a list .
     * @return Returns prefix of \a list.
     */
    List splitHead(const List &list, const ListIterator &end)
    {
        if (!end) {
            return list;
        }
        if (end.index == list.head) {
            return {};
        }
        auto last = list.head;
        while (data[last].next != end.index) {
            last = data[last].next;
        }
        data[last].next = 0;
        return List {list.head, last};
    }

    /**
     * @brief Create list iterator from list.
     * @param list
     * @return Iterator pointing to the first item in the list.
     */
    ListIterator head(const List &list)
    {
        return iteratorFromIndex(list.head);
    }

    ListIterator end(const List &list)
    {
        return std::next(iteratorFromIndex(list.tail));
    }

    List append(const List &head, const List &tail)
    {
        if (head.isEmpty()) {
            return tail;
        }
        if (tail.isEmpty()) {
            return head;
        }
        List result{head.head, tail.tail};
        data[head.tail].next = tail.head;
        return result;
    }
private:
    ListIterator iteratorFromIndex(IndexType index)
    {
        return ListIterator{ index, this };
    }

    std::vector<Item> data;
};


#endif // LINKED_LIST_POOL
