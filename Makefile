CC=gcc
CCPAR=mpicc

CCFLAGS=  -Wall -Wextra -Werror -pedantic -O0 -std=c99 
CCLIBS=-lm -fopenmp
DCCFLAGS=-g

VFLAGS=--leak-check=full --show-leak-kinds=all --track-origins=yes

SEQ_SRCS := pvc-seq.c
PAR_SRCS := pvc-par.c

SEQ_TARGET=pvcSeq
PAR_TARGET=pvcPar

all: seq par

seq: $(SEQ_SRCS)
	$(CC) $(CCFLAGS) $^ -o $(SEQ_TARGET) $(CCLIBS)

par: $(PAR_SRCS)
	$(CCPAR) $(CCFLAGS) $^ -o $(PAR_TARGET) $(CCLIBS)

debugSeq: $(SEQ_SRCS)
	$(CC) $(CCFLAGS) $(DCCFLAGS) $^ -o $(SEQ_TARGET) $(CCLIBS) -DDEBUG

debugPar: $(PAR_SRCS)
	$(CCPAR) $(CCFLAGS) $(DCCFLAGS) $^ -o $(PAR_TARGET) $(CCLIBS) -DDEBUG

clean:
	rm -rf *.o $(SEQ_TARGET) $(PAR_TARGET) vgcore*

run: par seq
	$(info To run the sequential version, execute: ./$(SEQ_TARGET) <n>)
	$(info To run the parallel version, execute: mpirun ./$(PAR_TARGET) <n>)

valgrindSeq: debugSeq
	valgrind $(VFLAGS) ./$(SEQ_TARGET) 10

testSeq: seq
	./$(SEQ_TARGET) 10

testPar: par
	mpirun ./$(PAR_TARGET) 10

zip:
	zip -r TA04.zip pvc-seq.c pvc-par.c Makefile resultados-pvc-mpi.pdf pcam-pvc.pdf