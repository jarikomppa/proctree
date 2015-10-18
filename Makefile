CXX=g++
CPPFLAGS=-std=c++11 -Wall -Wextra -pedantic -O3
PROCTREE_LDFLAGS=
HAPPYTREE_LDFLAGS=
PROCTREE_DIR=proctree
HAPPYTREE_DIR=happytree
RM=rm -f


all: proctree
	@echo "complete"

proctree: $(PROCTREE_DIR)/main.o $(PROCTREE_DIR)/proctree.o
	$(CXX) $(CPPFLAGS) $(PROCTREE_DIR)/main.o $(PROCTREE_DIR)/proctree.o -o $(PROCTREE_DIR)/proctree $(PROCTREE_LDFLAGS)
	@echo "proctree built"

$(PROCTREE_DIR)/main.o: $(PROCTREE_DIR)/main.cpp
	$(CXX) $(CPPFLAGS) -c $(PROCTREE_DIR)/main.cpp -o $(PROCTREE_DIR)/main.o

$(PROCTREE_DIR)/proctree.o: $(PROCTREE_DIR)/proctree.cpp $(PROCTREE_DIR)/proctree.h
	$(CXX) $(CPPFLAGS) -c $(PROCTREE_DIR)/proctree.cpp -o $(PROCTREE_DIR)/proctree.o

clean:
	$(RM) $(PROCTREE_DIR)/*.o $(PROCTREE_DIR)/proctree $(HAPPYTREE_DIR)/*.o $(HAPPYTREE_DIR)/happytree
	@echo "done"
