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
./build/lbvh/lbvh_cpu
./build/lbvh/test_normalize
```

## TODO

Current week: 1/4.

- [ ] **Week 1:** foundational work + sequential LBVH. Normalize input for testing & debugging, compute Z-order codes, implement sequential radix sort & verify correctness using comparison with `std::sort`, build LBVH sequentially. 
- [ ] **Week 2:** parallel sort; verfiy correctness & efficiency. This will probably take the longest.
- [ ] **Week 3:** parallel LBVH construction with binary radix tree and AABB fitting.
- [ ] **Week 4:** performance debugging and writeup.

## Notes

Recall that for SIMD (Single Instruction Multiple Data) programming, we want to use SoA (Structure of Arrays) for cache-efficiency. When you have time, you might want to look into AoSoA and `vector reserve()`.

Normalizing centroids to a specific range can avoid large variations in Z-order codes, leading to a more balanced tree overall. Identify min/max bounds of data, calculate scale factor, quantize centroid coordinates to integers coordinates, and interleave bits to produce Z-order code.

$n$-bit Morton code of a $3$D vector $v = (v_{x}, v_{y}, v_{z}) \in \langle 0, 1 \rangle ^{3}$ is computed by first determining the quantized coordinates $v^{*} = {v^{*}_{x}, v^{*}_{y}, v^{*}_{z}} \in \langle 0, 2^{n/3} \rangle \cross \langle 0, 2^{n/3} \rangle \cross \langle 0, 2^{n/3} \rangle$. The Z-order code is then evaluated by interleaving bits of the components of $v^{*}$ ([VBH17](https://doi.org/10.1145/3105762.3105782)). Not going to use the extended codes of this paper, probably, but it gave a good definition of Z-order codes.

## Dependencies
I use [rapidobj](https://github.com/guybrush77/rapidobj) to load and parse obj files for testing, but you should be able to use any obj parser as long as you write your own utility function to clean the output into something usable.
