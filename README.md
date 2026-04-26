# lbvh-cpu
An implementation of LBVH from [Karras 2012](https://doi.org/10.2312/EGGH/HPG12/033-037). Parallel on the CPU.

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
cmake --build build --target test
```

```
./build/bin/lbvh_cpu
./build/bin/test -f "cube.obj"
```

> [!IMPORTANT]
> `num_threads()` only guarantees maximum number of threads, so will want to count actual number when benchmarking.

## TODO

- [x] normalize input for testing & debugging, compute Z-order codes.
- [ ] parallel sort; verify correctness & efficiency. **In progress.**
- [ ] parallel LBVH construction with binary radix tree and AABB fitting.
- [ ] performance debugging and writeup.

### Optimization TODO

- [ ] Align the `vector<float>`s within `PrimitiveData` with a custom 32-byte aligned allocator and benchmark using `_mm256_load_ps` instead of `_mm256_loadu_ps` in `comp_zorder.cpp`. "On most modern CPUs there isn't a difference, so unless you know your data is aligned it's better to use unaligned versions" ([Vulkan Guide](https://vkguide.dev/docs/extra-chapter/intro_to_simd/)).
- [ ] In `comp_zorder.cpp`, benchmark whether `_mm256_cvttps_epi32` (truncate) would have any difference than `_mm256_cvtps_epi32` (rounding).

> [!NOTE]
> `normalize.cpp` is completely sequential and unoptimized since it won't be benchmarked. I will go back for it if I have time.

## Notes

Testing machine is [Intel Xeon Gold 5218](https://www.intel.com/content/www/us/en/products/sku/192444/intel-xeon-gold-5218-processor-22m-cache-2-30-ghz/specifications.html). 32 physical cores (2x16), 64 hardware threads. Optimizing for `x86_64`. Targeting AVX2.

Since 2-socket NUMA, remember to pin threads per socket and allocate memory per socket.

`normalize.cpp`: centroid coordinates are **min-max normalized** to prepare for later *quantization* and Z-order encoding. Normalization ensures that the minimum value is $`0`$ and maximum is $`1`$, and all other values scaled proportionally in between. This is done using $`x' = \frac{x-min(x)}{max(x) - min(x)}`$ where $`x`$ is the original value and $`x'`$ the normalized value ([Wikipedia](https://en.wikipedia.org/wiki/Feature_scaling#Rescaling_(min-max_normalization))).

`comp_zorder.cpp`: inner loop vectorized (SIMD) and outer loop with OpenMP. By this stage, the centroids have been normalized, and now we want to **quantize** them in order to be able to place them discretely into the Z-order curve grid. We do this by choosing a *bit resolution* for the Z-order codes. For example, $`10`$ bits per dimension allows for $`1024`$ discrete values, and a $`30`$-bit Z-order code that can be encoded as a $`32`$-bit integer. (Another option is $`21`$ bits per dimension for a $`63`$-bit Z-order code encoded as a $`64`$-bit integer). Quantize the `float`s to `int`s by $`x_{int} = \lfloor x_{fl} * (2^{n} - 1) \rfloor`$ where $`n`$ is the bit resolution. In practice, I use `_mm256_cvtps_epi32` which rounds to nearest and ties to even.

`sort_zorder.cpp`: with AVX2, can process 8x32-bit keys at once. The sequential code is from eloj's [radix-sorting](https://github.com/eloj/radix-sorting#-c-implementation).

> [!IMPORTANT]
> Work-in-progress and microbenchmarking are at [this repo](https://github.com/yduanmu/parallel_radix_sort).

> $`n`$-bit Z-order code of a $`3`$-D vector $`v = (v_{x}, v_{y}, v_{z}) \in \langle 0, 1 \rangle ^{3}`$ is computed by first determining the quantized coordinates $`v^{*} = {v^{*}_{x}, v^{*}_{y}, v^{*}_{z}} \in \langle 0, 2^{n/3} \rangle \times \langle 0, 2^{n/3} \rangle \times \langle 0, 2^{n/3} \rangle`$. The Z-order code is then evaluated by interleaving bits of the components of $`v^{*}`$. ([VBH17](https://oi.org/10.1145/3105762.3105782)).

In binary raix tree construction, should focus on reducing branches, precomputing prefix lengths, and cache locality. This step uses longest common prefixes and binary search over ranges. On CPU, this has the cons of being branchy, irregular memory access, and difficulty of vectorization. ISA matters least here.

Benchmark both `clang` and `gcc`; Clang might be more aggressive in loop unrolling.

## Dependencies
I use [rapidobj](https://github.com/guybrush77/rapidobj) to load and parse obj files for testing, but you should be able to use any obj parser as long as you write your own utility function to clean the output into something usable.

