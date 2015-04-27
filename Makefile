DEP_HEAD := $(wildcard *.h)
SRC_TEST := $(wildcard test/test_*.cpp)
OBJS_TEST := $(subst .cpp,.o,$(SRC_TEST))
OUTS_TEST := $(subst .cpp,,$(SRC_TEST))

all: test


test: $(OUTS_TEST)
	/usr/bin/perl run_tests.pl

test/test_%: test/test_%.o
	$(CXX) $^ -o $@ -L/usr/local/lib -lpthread

test/test_%.o: test/test_%.cpp $(DEP_HEAD)
	$(CXX) -Wall $(INC) -c $< -o $@ --std=c++0x

%.o: %.cpp $(DEP_HEAD)
	$(CXX) -Wall $(INC) -c $< -o $@


clean:
	$(RM) $(OBJS_TEST) $(OUTS_TEST)

