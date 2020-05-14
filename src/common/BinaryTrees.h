#ifndef BINARY_TREES_H
#define BINARY_TREES_H

#include <vector>
#include <cstdlib>
#include <climits>
#include <cstdint>
#include <algorithm>

template<class NodeTypeT, class FinalType>
class SegmentTreeBase
{
public:
    using NodeIndex = size_t;
    using NodeType = NodeTypeT;

    explicit SegmentTreeBase(size_t size)
        : size(size)
        , nodeCount(2 * size)
        , nodes(nodeCount)
    {}

    SegmentTreeBase(size_t size, const NodeType &initialValue)
        : SegmentTreeBase(size)
    {
        init(initialValue);
    }
protected:
    // Curiously recurrent pattern
    FinalType &This()
    {
        return static_cast<FinalType &>(*this);
    }

    // Curiously recurrent pattern
    const FinalType &This() const
    {
        return static_cast<const FinalType &>(*this);
    }

    size_t leaveIndexToPosition(NodeIndex index) const
    {
        return index - size;
    }

    NodeIndex positionToLeaveIndex(size_t position) const
    {
        return position + size;
    }

    bool isLeave(NodeIndex position) const
    {
        return position >= size;
    }

    void buildInnerNodes()
    {
        for (size_t i = size - 1; i > 0; i--) {
            This().updateFromChildren(nodes[i], nodes[i << 1], nodes[(i << 1) | 1]);
        }
    }

    void init(const NodeType &value)
    {
        std::fill_n(nodes.begin() + size, size, value);
        buildInnerNodes();
    }

    const size_t size; //< number of leaves and also index of left most leave
    const size_t nodeCount;
    std::vector<NodeType> nodes;
};

template<class NodeType, class FinalType>
class PointSetSegmentTree : public SegmentTreeBase<NodeType, FinalType>
{
    using BaseType = SegmentTreeBase<NodeType, FinalType>;
public:
    using BaseType::BaseType;

    void set(size_t index, const NodeType &value)
    {
        auto pos = this->positionToLeaveIndex(index);
        this->nodes[pos] = value;
        while (pos > 1) {
            auto parrent = pos >> 1;
            this->This().updateFromChildren(this->nodes[parrent], this->nodes[pos], this->nodes[pos ^ 1]);
            pos = parrent;
        }
    }

    const NodeType &valueAtPoint(size_t pos) const
    {
        return this->nodes[this->positionToLeaveIndex(pos)];
    }

    // Implement range query when necesarry
};

class PointSetMinTree : public PointSetSegmentTree<int, PointSetMinTree>
{
    using BaseType = PointSetSegmentTree<int, PointSetMinTree>;
public:
    using NodeType = int;

    using BaseType::BaseType;

    void updateFromChildren(NodeType &parent, NodeType &leftChild, NodeType &rightChild)
    {
        parent = std::min(leftChild, rightChild);
    }

    /**
     * @brief Find right most position with value than less than given in range [0; position].
     * @param position inclusive right side of query range
     * @param value search for position with value less than this
     * @return returns the position with searched property or -1 if there is no such position.
     */
    int rightMostLessThan(size_t position, int value)
    {
        auto isGood = [&](size_t pos) {
            return nodes[pos] < value;
        };
        // right side exclusive range [l;r)
        size_t goodSubtree = 0;
        for (size_t l = positionToLeaveIndex(0), r = positionToLeaveIndex(position + 1); l < r;
                l >>= 1, r >>= 1) {
            if (l & 1) {
                if (isGood(l)) {
                    // mark subtree as good but don't stop yet, there might be something good further to the right
                    goodSubtree = l;
                }
                ++l;
            }
            if (r & 1) {
                --r;
                if (isGood(r)) {
                    goodSubtree = r;
                    break;
                }
            }
        }
        if (!goodSubtree) {
            return -1;
        }
        // find rightmost good leave
        while (goodSubtree < size) {
            goodSubtree = (goodSubtree << 1) + 1;
            if (!isGood(goodSubtree)) {
                goodSubtree ^= 1;
            }
        }
        return leaveIndexToPosition(goodSubtree); // convert from node index to position in range (0;size]
    }

    /**
     * @brief Find left most position with value than less than given in range [position; size).
     * @param position inclusive left side of query range
     * @param value search for position with value less than this
     * @return returns the position with searched property or -1 if there is no such position.
     */
    int leftMostLessThan(size_t position, int value)
    {
        auto isGood = [&](size_t pos) {
            return nodes[pos] < value;
        };
        // right side exclusive range [l;r)
        size_t goodSubtree = 0;
        for (size_t l = positionToLeaveIndex(position), r = positionToLeaveIndex(size); l < r;
                l >>= 1, r >>= 1) {
            if (l & 1) {
                if (isGood(l)) {
                    goodSubtree = l;
                    break;
                }
                ++l;
            }
            if (r & 1) {
                --r;
                if (isGood(r)) {
                    goodSubtree = r;
                    // mark subtree as good but don't stop yet, there might be something good further to the left
                }
            }
        }
        if (!goodSubtree) {
            return -1;
        }
        // find leftmost good leave
        while (goodSubtree < size) {
            goodSubtree = (goodSubtree << 1);
            if (!isGood(goodSubtree)) {
                goodSubtree ^= 1;
            }
        }
        return leaveIndexToPosition(goodSubtree); // convert from node index to position in range (0;size]
    }
};

template <class NodeType, class PromiseType, class FinalType>
class LazySegmentTreeBase : public SegmentTreeBase<NodeType, FinalType>
{
    using BaseType = SegmentTreeBase<NodeType, FinalType>;
public:
    LazySegmentTreeBase(size_t size, const PromiseType &neutralPromise)
        : BaseType(size)
        , neutralPromiseElement(neutralPromise)
        , promise(size, neutralPromise)
    {
        h = 0;
        size_t v = size;
        while (v) {
            v >>= 1;
            h++;
        }
    }

    LazySegmentTreeBase(size_t size, NodeType value, PromiseType neutralPromise)
        : LazySegmentTreeBase(size, neutralPromise)
    {
        this->init(value);
    }

    NodeType rangeOperation(size_t l, size_t r, NodeType initialValue)
    {
        NodeType result = initialValue;
        l = this->positionToLeaveIndex(l);
        r = this->positionToLeaveIndex(r);
        pushDownFromRoot(l);
        pushDownFromRoot(r - 1);
        for (; l < r; l >>= 1, r >>= 1) {
            if (l & 1) {
                This().updateFromChildren(result, result, this->nodes[l++]);
            }
            if (r & 1) {
                This().updateFromChildren(result, result, this->nodes[--r]);
            }
        }
        return result;
    }

protected:
    void pushDownFromRoot(size_t p)
    {
        for (size_t i = h; i > 0; i--) {
            This().pushDown(p >> i);
        }
    }

    void updateUntilRoot(size_t p)
    {
        while (p > 1) {
            auto parent = p >> 1;
            if (promise[parent] == neutralPromiseElement) {
                This().updateFromChildren(this->nodes[parent], this->nodes[p & ~size_t(1)], this->nodes[p | 1]);
            }
            p = parent;
        }
    }

    using BaseType::This;

    int h;
    const PromiseType neutralPromiseElement;
    std::vector<PromiseType> promise;
};

class RangeAssignMaxTree : public LazySegmentTreeBase<int, uint8_t, RangeAssignMaxTree>
{
    using BaseType = LazySegmentTreeBase<int, uint8_t, RangeAssignMaxTree>;
public:
    using ValueType = int;
    RangeAssignMaxTree(size_t size, ValueType initialValue)
        : BaseType(size, initialValue, 0)
    {
    }

    void updateFromChildren(NodeType &parent, const NodeType &left, const NodeType &right)
    {
        parent = std::max(left, right);
    }

    void pushDown(size_t parent)
    {
        if (promise[parent]) {
            size_t left = (parent << 1);
            size_t right = (parent << 1) | 1;
            nodes[left] = nodes[right] = nodes[parent];
            if (left < size) {
                promise[left] = promise[parent];
            }
            if (right < size) {
                promise[right] = promise[parent];
            }
            promise[parent] = neutralPromiseElement;
        }
    }

    void setRange(size_t left, size_t right, NodeType value)
    {
        left = positionToLeaveIndex(left);
        right = positionToLeaveIndex(right);
        pushDownFromRoot(left);
        pushDownFromRoot(right - 1);
        for (size_t l = left, r = right; l < r; l >>= 1, r >>= 1) {
            if (l & 1) {
                nodes[l] = value;
                if (!isLeave(l)) {
                    promise[l] = 1;
                }
                l += 1;
            }
            if (r & 1) {
                r -= 1;
                nodes[r] = value;
                if (!isLeave(r)) {
                    promise[r] = 1;
                }
            }
        }
        updateUntilRoot(left);
        updateUntilRoot(right - 1);
    }

    int rangeMaximum(size_t l, size_t r)
    {
        return rangeOperation(l, r, std::numeric_limits<ValueType>::min());
    }
};

#endif // BINARY_TREES_H
