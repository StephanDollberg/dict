Performance Tests
===
This folder contains the performance tests. For measuring [nonius](https://code.google.com/p/sparsehash/) is used. 

So far these tests are only micro-benchmarks. Especially on larger maps they show a speedup of about a factor of two to a factor of 10 in comparison to `std::unordered_map`. Performance is about the same as google-densehashmap.

I would highly appreciate feedback and input on further benchmarks and especially on ideas/places for application benchmarks.

#### Running
First run a 

```
git submodule init
git submodule update
```
to fetch nonius.

To build:

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