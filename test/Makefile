DEFINES=

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	ifeq ($(CXX),clang++)
		LIBS= -lc++abi
	else
		LIBS=
	endif
endif
ifeq ($(UNAME), Darwin)
	LIBS=
endif

STANDARD=-std=c++11

ARGS= $(STANDARD)  -g -Wall -W -Wextra -pedantic-errors  $(INCLUDES) $(DEFINES)

ifeq ($(CXX),clang++-3.5)
	ADD_INC_FLAGS = -nostdinc++ -I/usr/local/lib/llvm-3.5/include/c++/v1 -fsanitize=address -fno-omit-frame-pointer
	ADD_LIB_FLAGS = -L/usr/local/lib/llvm-3.5/usr/lib
else
 	ADD_INC_FLAGS =
 	ADD_LIB_FLAGS =
endif

ifeq ($(CXX),clang++)
	standard_library = -stdlib=libc++
else
	standard_library =
endif

SOURCES = $(wildcard *.cpp)
OBJS = $(addprefix ./, $(notdir $(SOURCES:.cpp=.o)))

%.o: %.cpp
	$(CXX) $(standard_library) $(ARGS) $(CXXFLAGS) $(ADD_INC_FLAGS) -c $< -o $@

test_main: $(OBJS)
	$(CXX) $(standard_library) $(ARGS) $(LDFLAGS) $(OBJS) -o $@ $(LIBS) $(ADD_LIB_FLAGS)

with_sanitizers: ARGS+= -fsanitize=address
with_sanitizers: test_main

build14: STANDARD = -std=c++1y
build14: test_main

test:
	./test_main

clean:
	rm *.o test_main
