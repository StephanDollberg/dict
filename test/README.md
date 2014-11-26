Test Readme
===
The tests use the great [Catch](https://github.com/philsquared/Catch) framework.

To fetch catch first do a

```
git submodule init
git submodule update
```

Then to build the tests:

```
make 
```

Followed by a 

```
make test
```
to run the tests.
You can set your compiler to either gcc or clang by setting the `CXX` environment variable.

In addition if your compiler has support for the address- and undefined-sanitizers you can build with

```
make with_sanitizers
``` 
