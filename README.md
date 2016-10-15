# Compressed Grid

Compress small POD objects in a grid.

Suppose the size of each object is 4 bits, previously we may use 4 bytes to store 4 objects, but now we can compress them in one byte.

This is mainly designed for my toy Go AI, in order to store board information. With little sacrifice on set/get performance, the size of object grid could be reduced in a large scale, resulting better cache performance and reducing memory operations.

## Performance
> All with g++ 6.2.1, `-O2`(important)
>  `CompressedGrid<int, 19, 19, 11>` on x86-64 ArchLinux i7-4710HQ

- Random set & get 100M times(with `std::vector` operations):
     - Compressed Grid: 3253ms
     -  Raw array: 3110ms

- Memory usage:
     - Compressed Grid: 520 B
     - Raw Array: 1444 B

- 10M random get/set:
     - Check passed
     - No hash collision

## Example
```cpp
#include "compressed_grid.hpp"

using cg_t = compgrid::CompressedGrid<int, 19, 19, 11>; // An 19x19 grid with 11bits for each item(int)
cg_t cg; // all are 0
cg.set(cg_t::PointType{ 2, 3 }, 2047);
assert(cg.get(cg_t::PointType{ 2, 3 } == 2047));
```
## Prerequisites
- The object must be a POD type(i.e. could be handled as byte stream)
- Your `obj_size` must be smaller than `sizeof(UnderlyingType) - 1` (which is `sizeof(unsigned long long) - 1` by default)
- Your ISA is small endian(big endian might work, but untested)
- Unaligned memory access is allowed and not too slow(i.e. ok on x86(64), 

## More complex example
```cpp
using cg_t = compgrid::CompressedGrid<int, 19, 19, 6, unsigned long>; // Use unsigned long as underlying type. Useful for x86
cg_t cg;

cg_t::PointType p(2,1);
p.down(); // 3, 1
p.left(); // 3, 0. Undefined when y is already 0.
--p; // (2, 18)
assert(p.is_right());

cg.set(p, 15);
cg.clear(); // Set all to 0

unsigned char *raw = cg.raw(); // get raw buffer
std::size_t buffer_length = cg_t::BUF_LEN; // buffer length in bytes

std::size_t bitIndex = cg.getBitIndex(p); // The bit offset of point p to raw
std::size_t byteIdx, bitOffset;
std::tie(byteIdx, bitOffset) = cg.getByteBitOffset(bitIndex); // convert bit index to byteIndex(raw[byteIndex]) and starting offset of the first bit in that byte

std::cout << cg.getRawBits(bitIndex) << std::endl; // This should equal to 15(Underlying Type)
cg.setRawBits(bitIndex, 15UL); // set buf[ bit bitIndex : bit bitIndex+obj_bit_size ] to 15

std::cout << std::hash<cg_t>()(cg) << std::endl; // hash method is implemented so you can use it in some other containers
```

See compressed_grid_example.cpp for detailed usage.

## License
Apache License. See `LICENSE` for more information.
