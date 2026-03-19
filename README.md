# lbvh-cpu
An implementation of LBVH from [Karras 2012](https://research.nvidia.com/sites/default/files/pubs/2012-06_Maximizing-Parallelism-in/karras2012hpg_paper.pdf). Parallel on the CPU.

## Algorithm & Planning
Karras’s algorithm builds the LBVH using three stages, each parallelizable:
1. **Computation of Morton (Z-order) codes**
  - Sort the input set, which is in this case the (x, y, z) coordinates of each primitive’s centroid, by Z-order. If primitive locations overlap, the algorithm uses “extended” indices where the object index is appended as a bit to the Z-order code.
2. **Construction of binary radix trees**
  - //TBA
3. **AABB fitting for BVH construction**
  - Paths from leaf nodes to root are processed in parallel; threads walk up the tree using parent pointers recorded during radix tree construction. The first thread terminates while the second processes the node. Global atomic counters are used to track the number of threads that visited each internal node, but can reduce the number of these counters by instead tracking in shared-memory whether all leaves covered by one given node are being processed by the same thread blocks. With one thread calculating each node, Karras obtains O(n) time complexity.
    - One node per thread isn’t scalable on the CPU. I plan to use Karras’s atomic counter optimization for threads. That is, I plan to “split” the radix tree on some certain parent nodes, and allow threads to traverse the subtree until they have reached that parent node (fuzzy barrier).
      - **Problem**: I might get an extremely unbalanced tree that leaves threads idle for lengths of time even with a fuzzy barrier.
    - Post-order traversal. I’d have to include a child pointer during radix tree construction so it is a threaded tree ([Istrate slide 31](https://gabrielistrate.weebly.com/uploads/2/5/2/6/2526487/curs7.pdf)). Also conveniently avoids recursion. Post-order traversal keeps Karras’s depth-first order for data locality and cache hit rates.

## Goals:
- Efficiency gained by parallelization at each stage.
- Plot the speedup curve for threads. I expect to see diminishing returns beyond some amount of threads because the algorithm was designed expecting a GPU memory layout.
- If I have time, I plan to implement a sequential (binned SAH) BVH and compare as well. It’s known that SAH BVH > LBVH in terms of tree quality (50% without treelet restructuring and 97% with ([Karras & Aila 2013](https://research.nvidia.com/sites/default/files/pubs/2013-07_Fast-Parallel-Construction/karras2013hpg_paper.pdf)), but I’m interested in timing the two performing both construction and traversal on the same hardware. If I don’t have time to implement SAH BVH, I’ll use a centroid BVH instead. It results in worse trees than SAH BVH.
  - I will also be consulting [Miester & Bittner 2022](https://jcgt.org/published/0011/04/01/paper.pdf), which is “Performance Comparison of Bounding Volume Hierarchies for GPU Ray Tracing”.
  - While a HLBVH (LBVH that uses SAH) results in a better tree, it’s not as parallelizable on account of SAH being sequential. This is why I am instead implementing an LBVH.
