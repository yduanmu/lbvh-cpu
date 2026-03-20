# lbvh-cpu
An implementation of LBVH from [Karras 2012](https://research.nvidia.com/sites/default/files/pubs/2012-06_Maximizing-Parallelism-in/karras2012hpg_paper.pdf). Parallel on the CPU.

Linear bounding volume hierarchy (LBVH) construction reduces bounding volume hierarchy (BVH) construction to a sorting problem, while Karras’s addition in 2012 \[1] maximizes parallelization by generating the entire binary radix tree in parallel, which is used as a building block for other trees (such as BVH). This allows the BVH to be constructed in two kernel launches, one for the binary radix tree and one for axis-aligned bounding box (AABB) fitting. Though there have been improvements regarding both bottom-up BVH construction time and tree quality optimization \[2], Karras 2012 provides a uniquely clear four-stage approach while maximizing parallelism.

By adapting Karras’s algorithm for, and implementing it on, the CPU, I intend to analyze the performance implications of each parallelizable stage of the algorithm. Furthermore, I plan to compare construction and traversal time against binned surface area heuristic (SAH) BVH, which is a top-down algorithm traditionally implemented sequentially.

I have outlined the four parallelizable stages of Karras 2012 below:

**1. Computation of Morton (Z-order) codes**
  - Sort the input set, which is in this case the $(x, y, z)$ coordinates of each primitive’s centroid, by Z-order. If primitive locations overlap, the algorithm uses “extended” indices where the object index is appended as a bit to the Z-order code.
  - This is embarassingly parallel.

**3. Sorting of Z-order codes**
  - When analyzing the parallelization implications of this stage, I plan to compare parallel radix sort against (sequential) `std::sort`.

**4. Construction of binary radix tree**
  - Determine range of keys (Z-order codes) covered by each internal node, as well as its children. This section is parallel on each inner node.

**5. AABB fitting for BVH construction**
  - In the original GPU algorithm, paths from leaf nodes to root are processed in parallel where threads walk up the tree using parent pointers recorded during radix tree construction. The first thread terminates while the second processes the node. Global atomic counters are used to track the number of threads that visited each internal node, but can reduce the number of these counters by instead tracking in shared-memory whether all leaves covered by one given node are being processed by the same thread blocks. With one thread calculating each node, Karras obtains $O(n)$ time complexity.
    - One node per thread isn’t scalable on the CPU. I plan to divide-and-conquer the binary radix tree. First a breadth-first scan to identify subtrees. Allow threads to traverse the subtrees belonging to each sub-root node until they have reached it (fuzzy barrier). I will use iterative post-order traversal to keep Karras’s depth-first order for data locality and cache hit rates.
    - In the case of an extremely unbalanced tree, threads should be able to peek at the traversals of other threads in order to determine whether work stealing will be efficient.

**Goals of this project**:
- Analyze the efficiency gained by parallelization at each stage.
- Plot the speedup curve for threads and measure memory bandwidth. I expect to see diminishing returns beyond some amount of threads because the algorithm was designed expecting a GPU memory layout.
- Other than testing big and small meshes, the LBVH should be tested against meshes with primitives that are uniform, clustered, or overlapping.
- Comparison against the traditional CPU BVH-building algorithm. It’s known that a sweeping SAH BVH produces trees of good quality, though binning is an acceptable tradeoff to improve speed. Meanwhile, building with an LBVH results in sub-optimal tree quality \[2].

## References:

\[1] https://doi.org/10.2312/EGGH/HPG12/033-037

\[2] https://doi.org/10.1111/cgf.142662
