#ifndef BINARY_TREES_H
#define BINARY_TREES_H

/** \file BinaryTrees.h
 * \brief Utilities to simplify creation of specialized augmented binary trees.
 */

#include <vector>
#include <cstdlib>
#include <climits>
#include <cstdint>
#include <algorithm>


/**
 * Not really a segment tree for storing segments as referred in academic literature. Can be considered a
 * full, almost perfect, augmented binary tree. In the context of competitive programming often called segment tree.
 *
 * Child classes are expected to implement updateFromChildren(NodeType&parent, NodeType& left, NodeType& right)
 * method which calculates inner node values from children nodes.
 *
 * \tparam NodeTypeT type of each tree element
 * \tparam FinalType final child class used for curiously recurring template pattern
 */
template<class NodeTypeT, class FinalType>
class SegmentTreeBase
{
public:
    using NodePosition = size_t;
    using NodeType = NodeTypeT;

    /**
     * @brief Create tree with \a size leaves.
     * @param size number of leaves in the tree
     */
    explicit SegmentTreeBase(size_t size)
        : size(size)
        , nodeCount(2 * size)
        , nodes(nodeCount)
    {}

    /**
     * @brief Create a tree with given size and initial value.
     *
     * Inner nodes are calculated from leaves.
     * @param size number of leaves
     * @param initialValue initial leave value
     */
    SegmentTreeBase(size_t size, const NodeType &initialValue)
        : SegmentTreeBase(size)
    {
        init(initialValue);
    }
protected:
    // Curiously recurring template pattern
    FinalType &This()
    {
        return static_cast<FinalType &>(*this);
    }

    // Curiously recurring template pattern
    const FinalType &This() const
    {
        return static_cast<const FinalType &>(*this);
    }

    size_t leavePositionToIndex(NodePosition pos) const
    {
        return pos - size;
    }

    NodePosition leaveIndexToPosition(size_t index) const
    {
        return index + size;
    }

    bool isLeave(NodePosition position) const
    {
        return position >= size;
    }

    /**
     * @brief Calculate inner node values from leaves.
     */
    void buildInnerNodes()
    {
        for (size_t i = size - 1; i > 0; i--) {
            This().updateFromChildren(nodes[i], nodes[i << 1], nodes[(i << 1) | 1]);
        }
    }

    /**
     * @brief Initialize leaves with given value.
     * @param value value that will be assigned to leaves
     */
    void init(const NodeType &value)
    {
        std::fill_n(nodes.begin() + size, size, value);
        buildInnerNodes();
    }

    const size_t size; //< number of leaves and also index of left most leave
    const size_t nodeCount;
    std::vector<NodeType> nodes;
};

/**
 * \brief Tree for point modification and range queries.
 */
template<class NodeType, class FinalType>
class PointSetSegmentTree : public SegmentTreeBase<NodeType, FinalType>
{
    using BaseType = SegmentTreeBase<NodeType, FinalType>;
public:
    using BaseType::BaseType;

    /**
     * @brief Set leave \a index to \a value.
     * @param index Leave index, should be in the range [0,size)
     * @param value
     */
    void set(size_t index, const NodeType &value)
    {
        auto pos = this->leaveIndexToPosition(index);
        this->nodes[pos] = value;
        while (pos > 1) {
            auto parrent = pos >> 1;
            this->This().updateFromChildren(this->nodes[parrent], this->nodes[pos], this->nodes[pos ^ 1]);
            pos = parrent;
        }
    }

    const NodeType &valueAtPoint(size_t index) const
    {
        return this->nodes[this->leaveIndexToPosition(index)];
    }

    // Implement range query when necessary
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
     * @param value search for position less than this
     * @return returns the position with searched property or -1 if there is no such position.
     */
    int rightMostLessThan(size_t position, int value)
    {
        auto isGood = [&](size_t pos) {
            return nodes[pos] < value;
        };
        // right side exclusive range [l;r)
        size_t goodSubtree = 0;
        for (size_t l = leaveIndexToPosition(0), r = leaveIndexToPosition(position + 1); l < r;
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
        return leavePositionToIndex(goodSubtree);
    }

    /**
     * @brief Find left most position with value less than \a value in range [position; size).
     * @param position inclusive left side of query range
     * @param value search for position less than this
     * @return returns the position with searched property or -1 if there is no such position.
     */
    int leftMostLessThan(size_t position, int value)
    {
        auto isGood = [&](size_t pos) {
            return nodes[pos] < value;
        };
        // right side exclusive range [l;r)
        size_t goodSubtree = 0;
        for (size_t l = leaveIndexToPosition(position), r = leaveIndexToPosition(size); l < r;
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
        return leavePositionToIndex(goodSubtree);
    }
};

/**
 * \brief Tree that supports lazily applying an operation to range.
 *
 * Each inner node has a promise value describing an operation that needs to be applied to corresponding subtree.
 *
 * Child classes are expected to implement to pushDown(size_t nodePosition) method. Which applies the applies the
 * operation stored in \a promise for nodePosition to the direct children nodes.
 *
 * \tparam NodeType type of tree nodes
 * \tparam PromiseType type describing operation that needs to be applied to subtree
 * \tparam FinalType child class type for CRTP. See SegmentTreeBase
 */
template <class NodeType, class PromiseType, class FinalType>
class LazySegmentTreeBase : public SegmentTreeBase<NodeType, FinalType>
{
    using BaseType = SegmentTreeBase<NodeType, FinalType>;
public:
    /**
     * @param size Number of tree leaves.
     * @param neutralPromise Promise value that doesn't modify tree nodes.
     */
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

    /**
     * @brief Calculate the tree operation over the range [\a l, \a r)
     * @param l inclusive range left side
     * @param r exclusive range right side
     * @param initialValue Initial value for aggregate operation.
     * @return Tree operation calculated over the range.
     */
    NodeType rangeOperation(size_t l, size_t r, NodeType initialValue)
    {
        NodeType result = initialValue;
        l = this->leaveIndexToPosition(l);
        r = this->leaveIndexToPosition(r);
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
    /**
     * @brief Ensure that all the parents of node \a p have the operation applied.
     * @param p Node position
     */
    void pushDownFromRoot(typename BaseType::NodePosition p)
    {
        for (size_t i = h; i > 0; i--) {
            This().pushDown(p >> i);
        }
    }

    /**
     * @brief Update all the inner nodes in path from \a p to root.
     * @param p node position
     */
    void updateUntilRoot(typename BaseType::NodePosition p)
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

    int h; //< Tree height
    const PromiseType neutralPromiseElement;
    std::vector<PromiseType> promise;
};


/**
 * @brief Structure supporting range assignment and range maximum operations.
 */
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

    /**
     * @brief Change all the elements in range [\a left, \a right) to \a value.
     * @param left inclusive range left side
     * @param right exclusive right side of range
     * @param value value to be assigned
     */
    void setRange(size_t left, size_t right, NodeType value)
    {
        left = leaveIndexToPosition(left);
        right = leaveIndexToPosition(right);
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

    /**
     * @brief Calculate biggest value in the range [l, r)
     * @param l inclusive left side of range
     * @param r exclusive right side of range
     * @return biggest value in given range
     */
    int rangeMaximum(size_t l, size_t r)
    {
        return rangeOperation(l, r, std::numeric_limits<ValueType>::min());
    }
};

#endif // BINARY_TREES_H
