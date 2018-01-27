ARGS= -march=native -std=c++14 -pthread -O3 -g -fno-omit-frame-pointer -DNDEBUG -Wall -W -Wextra -pedantic-errors $(INCLUDES) $(DEFINES)

SOURCES = $(wildcard *.cpp)
OBJS = $(addprefix ./, $(notdir $(SOURCES:.cpp=.o)))

all: test_main

%.o: %.cpp
	$(CXX) $(standard_library) $(ARGS) $(CXXFLAGS) $(ADD_INC_FLAGS) -c $< -o $@

test_main: $(OBJS)
	$(CXX) $(standard_library) $(ARGS) $(LDFLAGS) $(OBJS) -o $@ $(LIBS) $(ADD_LIB_FLAGS) -lbenchmark

with_google: DEFINES = -DWITH_GOOGLE_BENCH
with_google: test_main

run:
	./test_main

clean:
	rm *.o test_main
