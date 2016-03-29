Performance Tests
===
This folder contains the performance tests. For doing performance measurements [google-benchmark](https://github.com/google/benchmark) is used.

So far these tests are only micro-benchmarks. Especially on larger maps they show a speedup of about a factor of two to a factor of 10 in comparison to `std::unordered_map`. Performance is about the same as google-densehashmap.

I would highly appreciate feedback and input on further benchmarks and especially on ideas/places for application benchmarks.

#### Running
First install `google-benchmark` either via your package manager or manually via cmake:

```
git clone https://github.com/google/benchmark && cd benchmark
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_LTO=true ../
make
make install
```

Once, `google-benchmark` is installed just run:

```
make
```

then to run the tests:

```
make run
```

Alternatively if want to also benchmark [google-sparsehash](https://code.google.com/p/sparsehash/) then you also need to build with

```
make with_google
```

Note that you need to have google-sparsehash installed and in path for it to work.
