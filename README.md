# podder
## A Vector for Pod's

* [WIP](https://en.wikipedia.org/wiki/Work_in_process)
* Extended `std::vector`-like API, f.e. `value_type pop_back_get ( )` and some 'unsafe' stuff, likely to be frowned upon, but they save cycles, we take the learning wheels of now;
* Excellent inter-operability with the STL, efficient construction and assignment from and comparison with all STL container types (including c-arrays);
* Custom allocation with `std::realloc` and friends, ~~does not~~ **cannot** support `std::allocator`s (the main reason I started this project);
* Not any checking of any return values from `std::malloc` or std::realloc` is implemented, if you run out of memory, close FireFox or head of to the shop to buy some DIMM's;
* Pod's only (`std::trivially_copyable`);
* Implementation takes advantage of bit-fields, which is possibly non-portable (that's what the gurus keep saying), but they make for clear code and I'm convinced, nowadays, the compiler knows best;
* No macros, except for one that fixes a VC-silliness (fix on the way!), `#define small char` and a useful one for Clang/LLVM;
* Small Vector Optimization, small vector capacities by value_type (size_type: std::uint64_t (std::uint32_t) ):
    * `std::uint8_t` : 23 (15);
    * `std::uint16_t`: 11 ( 7);
    * `std::uint32_t`:  5 ( 3);
    * `std::uint64_t`:  2 ( 1);
    * 64b < `sizeof ( value_type )` <= 128b : 1 (no SVO);
    * `sizeof ( value_type )` > 128b : no SVO (no SVO);
* **Small Vector Optimization Caveat**: not allowed by the standard (23.2.1/p10/b6):

    "Unless otherwise specified ...
     no `swap()` function invalidates any references, pointers, or 
     iterators referring to the elements of the containers being swapped."

    So I specify otherwise, `std::swap ( value_type & a, value_type & b )` has not been overloaded and `swap ( podder & a, podder & b )` lives in the `pdr` namespace;
* Comparision operators, also with all STL containers and c-arrays (rhs). Overload `std::less<your_type>` and `std::greater<your_type>` for your `value_type`, and provide an `operator == ( your_class & rhs )` if non-trivial comparison is required;
* Growth policy, customizable and extendible; 
* Customizable size_type, `std::uint32_t` or `std::uint64_t`;
* C++17 and moving;
* Limited to 64-bit linux and windows (for now);
* Some quick testing of `emplace_back()` with small vectors up to 256 values of `std::uint8_t` and `std::uint16_t` indicates a speedup of 35-40% as compared to the MSVC `std::vector`. I have not yet looked at things to optimize or bottle-necks, so looks promising.
* License: MIT, warranty till checkout;

`podder` is a header only library, with no dependencies other than the STL.
