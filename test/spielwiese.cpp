#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <functional>
#include <cassert>
#include <fstream>

#include "util/time.hpp"

#include "waveblocks/tiny_multi_index.hpp"
#include "waveblocks/shape_hyperbolic.hpp"
#include "waveblocks/shape_enumeration_base.hpp"
#include "waveblocks/shape_enumeration_default.hpp"

#pragma GCC diagnostic ignored "-Wunused-parameter"

/*
template<class T, std::size_t S>
std::ostream &operator<<(std::ostream &out, const std::array<T,S> &index)
{
    std::cout << "(";
    for (std::size_t i = 0; i < S-1; i++)
        std::cout << index[i] << ", ";
    if (S != 0)
        std::cout << index[S-1];
    std::cout << ")";
    return out;
}*/

    
//     NodeSize build_(const std::vector<NodeSize>& data, std::size_t begin, std::array<int,D> &index, int depth, int acc_sum, int tot_sum)
//     {
//         if (depth == D-1) {
//             index[depth] = tot_sum - acc_sum;
//             acc_sum = tot_sum;
//         }
//         
//         NodeSize size;
//         if (acc_sum == tot_sum) {
//             // write leaf
//             size.length = sizeof(NodeSize);
//             size.n_leafs = 1;
//         }
//         else if (empty(index, depth)) {
//             // write empty subtree
//             size.length = sizeof(NodeSize);
//             size.n_leafs = 0;
//         }
//         else {
//             // write node & subtrees
//             size.length = sizeof(NodeSize);
//             size.n_leafs = 0;
//             
//             //write subtrees
//             for (int i = 0; i <= tot_sum - acc_sum; i++) {
//                 index[depth] = i;
//                 size += build_(data, begin + size.length, index, depth+1, acc_sum+i, tot_sum);
//             }
//         }
//         data[begin] = size;
//         
//         index[depth] = 0;
//     }

template<int D>
struct RLimitedHyperbolicCutShape
{
private:
    int K_;
    std::array<int,D> limit_;
    
public:
    RLimitedHyperbolicCutShape(int K, const std::array<int,D>& bbox)
        : K_(K)
        , limit_(bbox)
    { }
    
    struct stack_entry_type
    {
        int product = 1;
    };
    
    inline bool empty(const std::array<int,D> &index, int dim, int value, stack_entry_type &top, int remain) const
    {
        if (value >= limit_[dim]) {
            return true;
        }
        
        // update accumulated product
        top.product *= value+1;
        
        // 
        int lowerbound = top.product * (remain+1);
        
        //std::cout << dim << ": " << index << " : " << top.product << " lowerbound: " << lowerbound << std::endl;
        
        return lowerbound > K_;
    }
    
    inline bool accept(const std::array<int,D> &index, stack_entry_type &top) const
    {
        if (index[D-1] >= limit_[D-1])
            return false;
        else
            return index[D-1]*top.product <= K_;
    }
    
    inline bool contains(const std::array<int,D> &index)
    {
        int prod = 1;
        for (int i = 0; i < D; i++) {
            prod *= index[i]+1;
        }
        return prod < K_;
    }
};

template<int D>
struct RHyperCubicShape
{
private:
    std::array<int,D> bbox_;
    
public:
    struct stack_entry_type { };
    
    inline bool empty(const std::array<int,D> &index, int dim, int value, stack_entry_type &top, int remain) const
    {
        return value > bbox_[dim];
    }
};

template<int D, class S>
struct ExtendedShape
{
private:
    S nested_;
    
public:
    ExtendedShape(const S& nested)
        : nested_(nested)
    { }
    
    struct stack_entry_type
    {
        typename S::stack_entry_type previous;
        typename S::stack_entry_type current;
        
        stack_entry_type()
            : previous()
            , current()
        { }
    };
    
    inline bool empty(const std::array<int,D> &index, int dim, int value, stack_entry_type &top, int remain) const
    {
        bool empty_prev = nested_.empty(index, dim, value-1, top.previous, remain);
        bool empty_curr = value == 0 ? false : nested_.empty(index, dim, value-1, top.previous, remain);
        
        if (empty_curr) {
            // check whether neighbour in previous slice is feasible
            if (value > 0)
                return empty_prev;
            else
                return true;
        }
        else {
            assert(!empty_prev);
            
            // nested shape contains at least one entry => extended shape too
            return true;
        }
    }
    
    inline bool contains(std::array<int,D> index)
    {
        //check whether nested shape already contains node
        if (nested_.contains(index))
            return true;
        
        //check whether nested shape contains at least one neighbor
        for (int i = 0; i < D; i++) {
            if (index[i] > 0) {
                index[i] -= 1;
                if (nested_.contains(index))
                    return true;
                index[i] += 1;
            }
        }
        
        return false;
    }
};

template<int D, class S, typename Func>
void enumerate(const S& shape, Func func, 
               typename S::stack_entry_type user,
               std::array<int,D> &index,
               int depth, int remain)
{
    if (depth == D-1 || remain == 0) {
        // LEAF
        index[depth] = remain;
        if (shape.accept(index, user)) {
            func(index);
        }
    }
    else {
        // TREE
        for (int i = 0; i <= remain; i++) {
            index[depth] = i;
            
            typename S::stack_entry_type top = user;
            if (!shape.empty(index, depth, i, top, remain-i)) {
                enumerate<D,S,Func>(shape, func, top, index, depth+1, remain-i);
            }
        }
    }
    
    index[depth] = 0;
}

template<int D, class S, typename Func>
void enumerate(const S& shape, Func func, int slice)
{
    std::array<int,D> index{};
    
    typename S::stack_entry_type user{};
    
    enumerate<D,S,Func>(shape, func, user, index, 0, slice);
}

/**
 * There exist 3 types of nodes:
 *  - white leaf: represents a node that is NOT part of the shape
 *  - black leaf: represents a node that is part of the shape
 *  - inner node: contains subtrees
 */
struct Node
{
    /**
     * \brief stores number of nodes (of all types) in its subset inclusive itself
     * 
     * value is 1 if this node is a leaf
     * value is >1 if this nodes is an inner node
     */
    std::size_t n_nodes;
    
    /**
     * \brief stores number of black leaves in its subset
     * 
     * value is 1 if this node is a black leaf
     * value is 0 if this node is a white leaf
     * value is N if this node is an inner node that contains N black leaves
     */
    std::size_t n_fill;
    
    Node &operator+=(const Node& rhs)
    {
        n_nodes += rhs.n_nodes;
        n_fill += rhs.n_fill;
        return *this;
    }
};

template<int D>
class ShapeSliceTree;

template<int D, class S>
class ShapeSliceBuilder
{
    friend class ShapeSliceTree<D>;
    
private:
    const S& shape_;
    int slice_;
    std::vector<Node> tree_; // number of nodes
    
    /**
     * \return 2-tuple (#nodes, #leaves)
     */
    Node enumerate_(typename S::stack_entry_type userstack,
                    std::array<int,D> &index,
                    int depth, int remain)
    {
        Node stats = {1,0};
        
        // current node is leaf
        if (depth == D-1 || remain == 0) {
            index[depth] = remain;
            if (shape_.accept(index, userstack)) {
                stats = {1, 1}; //mark leaf as black
            }
        }
        // current node is an inner node
        else {
            stats = {1,0};
            
            std::size_t mark = tree_.size();
            
            for (int i = 0; i <= remain; i++) {
                index[depth] = i;
                
                std::size_t submark = tree_.size();
                tree_.emplace_back(); // reserve space
                
                Node substats = {1,0};
                
                typename S::stack_entry_type top = userstack; // push entry to user stack
                if (!shape_.empty(index, depth, i, top, remain-i)) {
                    substats = enumerate_(top, index, depth+1, remain-i);
                }
                
                tree_[submark] = substats;
                stats += substats;
            }
            
            if (stats.n_fill == 0) {
                // Optimization: all subtrees are empty, no need to keep them => delete them
                tree_.resize(mark);
                stats.n_nodes = 1;
            }
        }
        
        index[depth] = 0; // dont forget!
        
        return stats;
    }
public:
    ShapeSliceBuilder(const S& shape, int slice)
        : shape_(shape)
        , slice_(slice)
        , tree_()
    {
        std::array<int,D> index{}; //zero initialize
        
        tree_.emplace_back(); // reserve space
        
        typename S::stack_entry_type userstack{};
        Node xxx = enumerate_(userstack, index, 0, slice);
        tree_[0] = xxx;
    }
};

std::string indent(int count)
{
    std::string txt = "";
    while (count-- > 0) {
        txt += "   ";
    }
    return txt;
}

template<int D>
class ShapeSliceTree
{
private:
    int slice;
    std::vector<Node> tree_;
    
public:
    template<class S>
    ShapeSliceTree(ShapeSliceBuilder<D,S>& builder)
        : slice(builder.slice_)
        , tree_(std::move(builder.tree_))
    { }
    
    ShapeSliceTree(ShapeSliceTree&& that) = default;
    
    std::size_t size() const
    {
        return tree_.front().n_fill;
    }
    
    /**
     * \brief determines memory used in bytes
     */
    std::size_t memory() const
    {
        return sizeof(Node)*tree_.size();
    }
    
    ShapeSliceTree &operator=(ShapeSliceTree&& that) = default;
    
    void enumerate() const
    {
        std::vector<Node>::const_iterator it = tree_.begin() + 1;
        
        int depth = 0;
        std::array<int,D> index{};
        
        int remain = slice;
        std::cout << "----" << std::endl;
        do {
            std::cout << indent(depth) << "(" << it->n_nodes << "," << it->n_fill << ")" << std::endl;
            // current node represents a leaf
            if (it->n_nodes == 1) {
                // leaf is black
                if (it->n_fill == 1) {
                    index[depth+1] = remain;
                    std::cout << indent(depth+1) << "$" << index << std::endl;
                    index[depth+1] = 0;
                } else {
                    index[depth+1] = remain;
                    //std::cout << indent(depth+1) << "!" << index << std::endl;
                    index[depth+1] = 0;
                }
                
                // next entry is sibling
                if (remain > 0) {
                    index[depth] += 1;
                    remain -= 1;
                }
                // next entry is uncle
                else {
                    remain += index[depth];
                    index[depth] = 0;
                    
                    if (depth != 0) {
                        remain -= 1;
                        index[depth-1] += 1;
                    }
                    
                    depth -= 1;
                }
            }
            // current node represents an inner node (is NOT a leaf) => next entry is child or uncle
            else {
                // next entry is a child
                if (remain > 0) {
                    depth += 1;
                }
                // next entry is an uncle
                else {
                    remain += index[depth];
                    index[depth] = 0;
                    
                    if (depth != 0) {
                        remain -= 1;
                        index[depth-1] += 1;
                    }
                    
                    --depth;
                }
            }
            
            //poll next entry
            ++it;
        } while (depth >= 0);
    }
};

using namespace waveblocks;

int main()
{
    //SparseManhattanTree<3> tree(5);
    
    const int D = 7;
    const int K = 1e6;
    std::array<int,D> bbox = {8,8,8,8,8,8,8};
    
    std::cout << bbox << std::endl;
    
    int islice = 6;
    
    // compare
    {
        typedef RLimitedHyperbolicCutShape<D> S;
        typedef ExtendedShape<D, RLimitedHyperbolicCutShape<D> > ES;
        
        S shape{K,bbox};
        
        ES eshape(shape);
        
        double start = getRealTime();
        
        std::size_t count = 0;
        for (int islice = 0; ; islice++) {
            ShapeSliceBuilder<D,S> builder{shape, islice};
            ShapeSliceTree<D> tree{builder};
            
            std::cout << "slice (" << islice << ")" << std::endl;
            std::cout << "  count: " << tree.size() << std::endl;
            std::cout << "  efficiency: " << double(tree.memory())/double(tree.size()) << std::endl;
            
            count += tree.size();
            if (tree.size() == 0)
                break;
        }
        
        double end = getRealTime();
        
        std::cout << "count: " << count << std::endl;
        std::cout << "time: " << (end - start) << std::endl;
        
        //tree.enumerate();
//         
//         double start = getRealTime();
//         
//         int counter = 0;
//         
//         //std::ofstream out("recursive.csv");
//         
//         auto func = [&counter](const std::array<int,D>& index) {
//             ++counter;
//             //out << index << std::endl;
//             std::cout << index << std::endl;
//         };
//         
//         enumerate<D,S,std::function<void(const std::array<int,D>&)> >(shape, func, islice);
//         
//         std::cout << "slice: " << islice << ", count: " << counter << std::endl;
//         
//         double end = getRealTime();
//         
//         std::cout << "time: " << (end - start) << std::endl;
    }
    
    {
        typedef LimitedHyperbolicCutShape<D> S;
        
        S shape(K, bbox);
        
        double start = getRealTime();
        
        ShapeEnumeration<D> *ref_enum = new DefaultShapeEnumeration<D,TinyMultiIndex<std::size_t,D>,S>(shape);
        
        double end = getRealTime();
        
        std::ofstream out("iterative.csv");
        
        for (auto index : ref_enum->slice(islice)) {
            //out << index << std::endl;
            //std::cout << entry << std::endl;
        }
        
        std::cout << "count: " << ref_enum->size() << std::endl;
        std::cout << "slices: " << ref_enum->count_slices() << std::endl;
        std::cout << "time: " << (end - start) << std::endl;
    }
    
    return 0;
}