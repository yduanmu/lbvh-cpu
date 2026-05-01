# lbvh-cpu
An implementation of LBVH from [Karras 2012a](https://research.nvidia.com/sites/default/files/pubs/2012-06_Maximizing-Parallelism-in/karras2012hpg_paper.pdf). Parallel on the CPU.

## Table of contents
- [Usage](#usage)
- [TODO](#todo)
- [Notes](#notes)
- [References](#references)

## Usage
Assumes geometry is already loaded and represented in memory. `util/normalize.cpp` uses the triangulated output from [rapidobj](https://github.com/guybrush77/rapidobj), and discards everything aside from triangle positions and indices. Files parsed using this cannot have references to `mtl`. `util/normalize.cpp` returns a struct `PrimitiveData` of arrays (centroids, primitive ids, and min/max for bounding box computation).

```
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --target lbvh_cpu
cmake --build build --target test
```

```
./build/bin/lbvh_cpu
./build/bin/test -f "utah_teapot_6k.obj" -t 10
./build/bin/test -f "powerplant_13m/powerplant.obj" -t 10
```

```
Usage. All flags default TRUE (parallel). Toggle for FALSE (sequential):
-f <file name>
-t <number of threads>
    (default 16. 1 means sequential)
-c Z-order (Morton) code computation
-s radix sort
-r binary radix tree construction
-b BVH construction
-l toggle logging to FALSE (improves execution time)
-h print this message
```

```
perf stat -e L1-dache-loads,L1-dcache-load-misses,L1-icache-load_misses ./build/bin/test -f "utah_teapot_33k.obj" -t 30
```

## TODO

- [x] normalize input for testing & debugging, compute Z-order codes.
- [x] parallel sort; verify correctness & efficiency.
- [ ] parallel binary radix tree construction.
- [ ] AABB fitting & BVH construction.
- [ ] correctness test w/ ray tracing OR write a correctness "proof" per step.
- [ ] performance debugging and writeup. Keep in mind [benchmarking pitfalls](https://stackoverflow.com/a/60293070/32655769).

### Optimization TODO

- [ ] Align the `vector<float>`s within `PrimitiveData` with a custom 32-byte aligned allocator and benchmark using `_mm256_load_ps` instead of `_mm256_loadu_ps` in `comp_zorder.cpp`. "On most modern CPUs there isn't a difference, so unless you know your data is aligned it's better to use unaligned versions" ([Vulkan Guide](https://vkguide.dev/docs/extra-chapter/intro_to_simd/)).

> [!NOTE]
> `normalize.cpp` is completely sequential and unoptimized since it won't be benchmarked. I will go back for it if I have time.

## Notes

Testing machine is [Intel Xeon Gold 5218](https://www.intel.com/content/www/us/en/products/sku/192444/intel-xeon-gold-5218-processor-22m-cache-2-30-ghz/specifications.html). 32 physical cores (2x16), 64 hardware threads. Optimizing for `x86_64`. Targeting AVX2.

Since 2-socket NUMA, remember to pin threads per socket and allocate memory per socket.

`normalize.cpp`: centroid coordinates are **min-max normalized** to prepare for later *quantization* and Z-order encoding. Normalization ensures that the minimum value is $`0`$ and maximum is $`1`$, and all other values scaled proportionally in between. This is done using $`x' = \frac{x-min(x)}{max(x) - min(x)}`$ where $`x`$ is the original value and $`x'`$ the normalized value ([Wikipedia](https://en.wikipedia.org/wiki/Feature_scaling#Rescaling_(min-max_normalization))).

`comp_zorder.cpp`: inner loop vectorized (SIMD) and outer loop with OpenMP. By this stage, the centroids have been normalized, and now we want to **quantize** them in order to be able to place them discretely into the Z-order curve grid. We do this by choosing a *bit resolution* for the Z-order codes. For example, $`10`$ bits per dimension allows for $`1024`$ discrete values per dimension, and a $`30`$-bit Z-order code that can be encoded as a $`32`$-bit integer. (Another option is $`21`$ bits per dimension for a $`63`$-bit Z-order code encoded as a $`64`$-bit integer. However, a $`30`$-bit Morton code already allows for $`1024^3`$ possible positions in $`3`$-D space\*.) Quantize the `float`s to `int`s by $`x_{int} = \lfloor x_{fl} * (2^{n} - 1) \rfloor`$ where $`n`$ is the bit resolution. In practice, I use `_mm256_cvtps_epi32` which rounds to nearest and ties to even.

> \*We use the Birthday paradox [square approximation](https://en.wikipedia.org/wiki/Birthday_problem#Square_approximation) to calculate the expected number of colliding pairs. For $`n`$ points and $`N = 1024^3`$, the approximation is $`\frac{n^2}{2N}`$.
> - For [`utah_teapot_6k`](https://graphics.cs.utah.edu/teapot/), with $`3,493`$ vertices, the possibility of collision is quite low.
> - For [`utah_teapot_33k`](https://graphics.cs.utah.edu/teapot/), with $`17,456`$ vertices, the expected number of collision is still acceptably low at ~$`0.14`$ pairs.
> - For [`bunny_144k`](https://casual-effects.com/data/), with $`72,378`$ vertices, the expected number of collision is ~$`2.4`$ pairs.
> - For [`hairball_3m`](https://casual-effects.com/data/), with $`1,441,098`$ vertices, the expected number of collision is ~$`967`$ pairs.
> - For [`powerplant_13m`](https://casual-effects.com/data/), with $`10,614,919`$ vertices, the expected number of collision is ~$`52469`$ pairs.

`sort_zorder.cpp`: see [this repo](https://github.com/yduanmu/parallel_radix_sort).

`cons_radix_seq.cpp`: with $`N`$ leaf nodes in total, the root covers range $`[0, N-1]`$. For some appropriate $`\gamma`$, left child covers $`[0, \gamma]`$ while right child covers $`[\gamma + 1, N-1]`$. This is the top-down recursive algorithm for constructing binary radix tree, which terminates when all ranges covered contain only one item (leaf nodes). $`\gamma`$ is chosen according to the highest differing bit within the Morton codes within its given range; this can be done using binary search ([Kar12b](https://developer.nvidia.com/blog/thinking-parallel-part-iii-tree-construction-gpu/)). This uses `__builtin_clz`, which is available on GCC and Clang.

`cons_radix.cpp:`: uses the invariant that any binary tree with $`N`$ leaf nodes always has exactly $`N-1`$ internal nodes. Determine which range of objects any given node corresponds to, without knowing anything else about the tree ([Kar12b](https://developer.nvidia.com/blog/thinking-parallel-part-iii-tree-construction-gpu/)).

> $`n`$-bit Z-order code of a $`3`$-D vector $`v = (v_{x}, v_{y}, v_{z}) \in \langle 0, 1 \rangle ^{3}`$ is computed by first determining the quantized coordinates $`v^{*} = {v^{*}_{x}, v^{*}_{y}, v^{*}_{z}} \in \langle 0, 2^{n/3} \rangle \times \langle 0, 2^{n/3} \rangle \times \langle 0, 2^{n/3} \rangle`$. The Z-order code is then evaluated by interleaving bits of the components of $`v^{*}`$. ([VBH17](https://oi.org/10.1145/3105762.3105782)).

In binary radix tree construction, should focus on reducing branches, precomputing prefix lengths, and cache locality. This step uses longest common prefixes and binary search over ranges. On CPU, this has the cons of being branchy, irregular memory access, and difficulty of vectorization.

Benchmark both `clang` and `gcc`; Clang might be more aggressive in loop unrolling.

## References
- Tero Karras. "Maximizing Parallelism in the Construction of BVHs, Octrees, and k-d Trees." *High Performance Graphics*, 2012. ([Link](https://research.nvidia.com/sites/default/files/pubs/2012-06_Maximizing-Parallelism-in/karras2012hpg_paper.pdf)).
- Tero Karras. "Thinking Parallel, Part III: Tree Construction on the GPU." *NVIDIA Developer Technical Blog*, 2012. ([Link](https://developer.nvidia.com/blog/thinking-parallel-part-iii-tree-construction-gpu/)).
    - Referenced for explanation of sequential and parallel binary radix tree construction.
- Takahiro Harada and Lee Howes. "Introduction to GPU Radix Sort." *Heterogeneous Computing with OpenCL*, 2011. ([Link](https://gpuopen.com/download/Introduction_to_GPU_Radix_Sort.pdf)).
    - Referenced for explanation of parallel radix sort.
- Eddy Jansson (eloj)'s [radix-sorting](https://github.com/eloj/radix-sorting) for sequential radix sort. I converted Jansson's [C implementation](https://github.com/eloj/radix-sorting/blob/master/radix_sort_u32.c) to C++. Then, I parallelized it by referencing [HH11](https://gpuopen.com/download/Introduction_to_GPU_Radix_Sort.pdf)'s method of count -> prefix scan -> reorder.
- Slobodan Pavlic (guybrush77)'s [rapidobj](https://github.com/guybrush77/rapidobj) to load and parse obj files for testing, but you should be able to use any obj parser as long as you write your own utility function to clean the output into something usable.

