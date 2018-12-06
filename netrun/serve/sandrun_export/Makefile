OPTS=-O -Wall
CFLAGS=-I. $(OPTS)
CCC=g++
LIBS=

C=sandsend
S=sandserv
OBJ=sockRoutines.o osl/sha1.o auth_pipe.o

all: $(C) $(S)

$(C): $(C).o $(OBJ)
	$(CCC) $(OPTS) $(C).o $(OBJ) -o $(C) $(LIBS)

$(S): $(S).o $(OBJ) 
	$(CCC) $(OPTS) $(S).o $(OBJ)  -o $(S) $(LIBS)

clean:
	-rm $(OBJ) *.o *~ $(C) $(S)

# Trick gmake into compiling .cpp's into .o's.
%.o: %.cpp 
	$(CCC) $(CFLAGS) -c $< -o $@

# Trick other makes into compiling .cpp's into .o's.
.SUFFIXES: .cpp

.cpp.o:
	$(CCC) $(CFLAGS) -c $< -o $@

