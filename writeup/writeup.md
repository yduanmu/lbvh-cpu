# Project Writeup

Due 01 May 2026 @5PM.

A bounding volume hierarchy (BVH) is a tree structure used in collision detection and ray tracing.

> Primitives are stored in the leaves, and each node stores a bounding box of the primitives in the nodes beneath it. Thus, as a ray traverses through the tree, any time it doesn’t intersect a node’s bounds, the subtree beneath that node can be skipped. [PJH23](https://pbr-book.org).

A BVH can be constructed meaning that all objects are available before construction begins. Within this class, there are two construction methods: top-down (sequential) and bottom-up (parallelizable). Furthermore, bounding boxes can be determined by computing centroids or by surface area heuristic (SAH). While methods using SAH produces better trees than methods using centroids, SAH is also a greedy algorithm. There are hybrid methods as well, such as the hierarchical LBVH (HLBVH) [PL10](https://research.nvidia.com/sites/default/files/pubs/2010-06_HLBVH-Hierarchical-LBVH/HLBVH-final.pdf), but that is out of the scope of this project.

Primitives need not be triangles, but I have used triangle primitives in this implementation of a BVH.

## References
- [PJH23](https://pbr-book.org) Pharr M., Jakob W., Humphreys G. *Physically Based Rendering: From Theory To Implementation*, 2023.
- [PL10](https://research.nvidia.com/sites/default/files/pubs/2010-06_HLBVH-Hierarchical-LBVH/HLBVH-final.pdf) Pantaleoni J., Luebke D. "HLBVH: Hierarchical LBVH Construction for Real-Time Ray Tracing of Dynamic Geometry." *High Performance Graphics 2010*, 2010.
