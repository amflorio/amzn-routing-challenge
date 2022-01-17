SYSTEM = x86-64_osx
LIBFORMAT = static_pic

CCC = clang++ -O2 -Wall -std=c++11
CCOPT = -m64 -fPIC -fexceptions -DIL_STD -stdlib=libc++ -c

#CCLNDIRS=-L/home/mdflorio/boost/lib
CCLNFLAGS = -m64 -lm -lpthread -framework CoreFoundation -framework IOKit -stdlib=libc++ -lboost_serialization -larmadillo -lmlpack

CCFLAGS = $(CCOPT)

SOURCES=AlgoInput.cpp Algorithm.cpp DatasetBuilder.cpp EntryExit.cpp JSONParser.cpp LassoRegression.cpp Learner.cpp LocalSearch.cpp Package.cpp Route.cpp RoutingPattern.cpp Sequence.cpp SequenceBuilder.cpp SolutionInspector.cpp Stop.cpp Tester.cpp TestRoute.cpp TrainingRoute.cpp TSPHeuristic.cpp TTMatrix.cpp main.cpp

OBJECTS=$(SOURCES:.cpp=.o)
	EXECUTABLE=main_osx

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CCC) $(CCLNDIRS) $(CCLNFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CCC) $(CCFLAGS) $< -o $@

clean:
	/bin/rm -rf *.o

