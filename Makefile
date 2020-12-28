BASEDIR = .

INCLUDES = -I$(BASEDIR)

CXX = g++
CXXFLAGS = $(INCLUDES) -g

.SUFFIXES: .cc

.cc.o:
	$(CXX) $(CXXFLAGS) -c $<

.cc :
	$(CXX) $(CXXFLAGS) $< -o $@

SRC = orderbook.cpp \
      orderbookmanager.cpp \
      main.cpp

OBJ = $(addsuffix .o, $(basename $(SRC)))

orderbook : $(OBJ)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

clean:
	rm -f $(OBJ) orderbook
