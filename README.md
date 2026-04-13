# lbvh-cpu
An implementation of LBVH from [Karras 2012](https://doi.org/10.2312/EGGH/HPG12/033-037). Parallel on the CPU.

For sequential and parallel radix sort implementation, see [here](https://github.com/yduanmu/radix-sort).

## Table of contents
- [Usage](#usage)
- [TODO](#todo)
- [Notes](#notes)
- [Dependencies](#dependencies)

## Usage
Assumes geometry is already loaded and represented in memory. `util/normalize.cpp` uses the triangulated output from [rapidobj](https://github.com/guybrush77/rapidobj), and discards everything aside from triangle positions and indices. `util/normalize.cpp` returns a struct `PrimitiveData` of arrays (centroids, primitive ids, and min/max for bounding box computation).

```
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --target lbvh_cpu
cmake --build build --target test_normalize
```

```
./build/bin/lbvh_cpu
./build/bin/test_normalize -f "cube.obj"
```

## TODO

Current week: 1/4.

- [ ] **Week 1:** foundational work + sequential LBVH. Normalize input for testing & debugging, compute Z-order codes, implement sequential radix sort & verify correctness using comparison with `std::sort`. 
- [ ] **Week 2:** parallel sort; verfiy correctness & efficiency. This will probably take the longest.
- [ ] **Week 3:** parallel LBVH construction with binary radix tree and AABB fitting.
- [ ] **Week 4:** performance debugging and writeup.

### Optimization TODO

- [ ] Align the `vector<float>`s within `PrimitiveData` with a custom 32-byte aligned allocator and benchmark using `_mm256_load_ps` instead of `_mm256_loadu_ps` in `comp_zorder.cpp`. "On most modern CPUs there isn't a difference, so unless you know your data is aligned it's better to use unaligned versions" ([Vulkan Guide](https://vkguide.dev/docs/extra-chapter/intro_to_simd/)).

### Optimize normalize.cpp for SIMD

The normalize code isn't the point and isn't even timed, but for learning experience, implement the below.

- [ ] Guarantee strict alignment for `vector<float>` for `64 bytes`.
- [ ] Prepare for SIMD batching by restructuring the nested `shape` and `face` for loop into a single for loop based off `num_tris`. Want to process `4–8` triangles at once to benefit from AVX2/AVX-512, but currently there's too much indexing indirection to do so.
- [ ] Restructure loop (again) to operate on `8` (AVX2) or `16` (AVX-512) triangles in parallel using vector registers.
- [ ] Use SIMD instructions such as `_mm256_add_ps` to manually load data (deindex triangles into flat triangle buffer (contiguous memory)), operate on it in vectors, and store it back.

## Notes

Testing machine is **Intel Xeon Gold 5218**. 32 physical cores (2x16), 64 hardware threads. Optimizing for `x86_64`. Either targeting AVX2 (main result) or AVX-512 (upper bound).

Since 2-socket NUMA, remember to pin threads per socket and allocate memory per socket.

`normalize.cpp`: centroid coordinates are **min-max normalized** to prepare for later *quantization* and Z-order encoding. Normalization ensures that the minimum value is $`0`$ and maximum is $`1`$, and all other values scaled proportionally in between. This is done using $`x' = \frac{x-min(x)}{max(x) - min(x)}`$ where $`x`$ is the original value and $`x'`$ the normalized value ([Wikipedia](https://en.wikipedia.org/wiki/Feature_scaling#Rescaling_(min-max_normalization))).

`comp_zorder.cpp`: inner loop vectorized (SIMD) and outer loop with OpenMP. By this stage, the centroids have been normalized, and now we want to **quantize** them in order to be able to place them discretely into the Z-order curve grid. We do this by choosing a *bit resolution* for the Z-order codes. For example, $`10`$ bits per dimension allows for $`1024`$ discrete values, and a $`30`$-bit Z-order code that can be encoded as a $`32`$-bit integer. (Another option is $`21`$ bits per dimension for a $`63`$-bit Z-order code encoded as a $`64`$-bit integer). Quantize the `float`s to `int`s by $`x_{int} = \lfloor x_{fl} * (2^{n} - 1) \rfloor`$ where $`n`$ is the bit resolution. Duplicate keys are handled similarly to Karras 2012, where [INSERT LATER].

> [!WARNING]
> Remember to handle duplicate keys.

Z-order sort: if AVX2, can process 8x32-bit keys at once; if AVX-512, 16x32-bit keys. ISA matters most here.

Parallel radix sort with OpenMP; remember to use cache-friendly bucket layout. ISA doesn't matter as much as NUMA awareness and memory layout.

In binary radix tree construction, should focus on reducing branches, precomputing prefix lengths, and cache locality. This step uses longest common prefixes and binary search over ranges. On CPU, this has the cons of being branchy, irregular memory access, and difficulty of vectorization. ISA matters least here.

Benchmark both `clang` and `gcc`.

- Something about compilers preference for pointer casting vs `memcpy`.
- Clang might be better at vectorizing Z-order code loops and generating clean AVX-512. Include `-fopenmp -lomp`.
- GCC might be better at aggressive inlining and OpenMP performance. Include `-fopenmp`.

```
-O3 -march=x86-64
```

```
-O3 -march=native -ffast-math
```

```
-mavx2
-mavx512f -mavx512bw -mavx512vl
```

Recall that for SIMD (Single Instruction Multiple Data) programming, we want to use SoA (Structure of Arrays) for cache-efficiency.

> [!IMPORTANT]
> When `normalize.cpp` compiles, restructure `PrimitiveData` from SoA to AoSoA.

> [!IMPORTANT]
> Look into manual AVX (or optimally, Eigen or GLM) for speedups in tight loops.

Normalizing centroids to a specific range can avoid large variations in Z-order codes, leading to a more balanced tree overall. Identify min/max bounds of data, calculate scale factor, quantize centroid coordinates to integers coordinates, and interleave bits to produce Z-order code.

> $`n`$-bit Z-order code of a $`3`$-D vector $`v = (v_{x}, v_{y}, v_{z}) \in \langle 0, 1 \rangle ^{3}`$ is computed by first determining the quantized coordinates $`v^{*} = {v^{*}_{x}, v^{*}_{y}, v^{*}_{z}} \in \langle 0, 2^{n/3} \rangle \times \langle 0, 2^{n/3} \rangle \times \langle 0, 2^{n/3} \rangle`$. The Z-order code is then evaluated by interleaving bits of the components of $`v^{*}`$.

([VBH17](https://doi.org/10.1145/3105762.3105782)). Not going to use the extended codes of this paper, probably, but it gave a good definition of Z-order codes. I couldn't find a legible copy of Morton's 1966 paper. Seriously, it was unreadable.

> Using `size_t` appropriately makes your source code a little more self-documenting. When you see an object declared as a `size_t` , you immediately know it represents a size in bytes or an index, rather than an error code or a general arithmetic value.

([Embedded 2015](https://www.embedded.com/why-size_t-matters/)).

## Dependencies
I use [rapidobj](https://github.com/guybrush77/rapidobj) to load and parse obj files for testing, but you should be able to use any obj parser as long as you write your own utility function to clean the output into something usable.
