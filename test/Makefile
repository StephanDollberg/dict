STANDARD=-std=c++11

ARGS= $(STANDARD) -march=native -g -fno-omit-frame-pointer -Wall -W -Wextra -pedantic-errors  $(INCLUDES) $(DEFINES)

SOURCES = $(wildcard *.cpp)
OBJS = $(addprefix ./, $(notdir $(SOURCES:.cpp=.o)))

%.o: %.cpp ../include/dict/dict.hpp
	$(CXX) $(standard_library) $(ARGS) $(CXXFLAGS) $(ADD_INC_FLAGS) -c $< -o $@

test_main: $(OBJS)
	$(CXX) $(standard_library) $(ARGS) $(LDFLAGS) $(OBJS) -o $@ $(LIBS) $(ADD_LIB_FLAGS)

with_sanitizers: ARGS+= -fsanitize=address -fsanitize=undefined
with_sanitizers: test_main

build14: STANDARD = -std=c++1y
build14: test_main

build17: STANDARD = -std=c++17
build17: test_main

test:
	./test_main

clean:
	rm *.o test_main
