# lbvh-cpu
An implementation of LBVH from [Karras 2012](https://doi.org/10.2312/EGGH/HPG12/033-037). Parallel on the CPU.

For sequential and parallel radix sort implementation, see [here](https://github.com/yduanmu/radix-sort).

## Table of contents
- [Usage](#usage)
- [TODO](#todo)
- [Dependencies](#dependencies)

## Usage
Assumes geometry is already loaded and represented in memory. `util/normalize.cpp` uses the triangulated output from [rapidobj](https://github.com/guybrush77/rapidobj), and discards everything aside from triangle positions and indices.

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

## Dependencies
I use [rapidobj](https://github.com/guybrush77/rapidobj) to load and parse obj files for testing, but you should be able to use any obj parser as long as you write your own utility function to clean the output into something usable.
