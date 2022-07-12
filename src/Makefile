CXX = g++                     # compiler
CXXFLAGS = -g -Wall -MMD -mavx2 -std=c++17 # compiler flags
OBJECTS = attack.o board.o book.o eval.o fen.o hash.o list.o main.o material.o move_check.o move_do.o move_evasion.o move_gen.o move_legal.o move.o nnue.o option.o pawn.o piece.o posix.o protocol.o pst.o pv.o random.o recog.o search_full.o search.o see.o simd.o sort.o square.o trans.o useful.o util.o value.o vector.o # object files forming executable
DEPENDS = attack.d board.d book.d eval.d fen.d hash.d list.d main.d material.d move_check.d move_do.d move_evasion.d move_gen.d move_legal.d move.d nnue.d option.d pawn.d piece.d posix.d protocol.d pst.d pv.d random.d recog.d search_full.d search.d see.d simd.d sort.d square.d trans.d useful.d util.d value.d vector.d # object files forming executable
EXEC = rebel15x2                  # executable name

${EXEC} : ${OBJECTS}          # link step
	${CXX} ${OBJECTS} -o ${EXEC} -lpthread

-include ${DEPENDS}           # copies files x.d, y.d, z.d (if they exist)

clean:
	rm ${OBJECTS} ${EXEC} ${DEPENDS}
