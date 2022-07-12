
// main.cpp

// includes

#include <cstdio>
#include <cstdlib>

#include "attack.h"
#include "book.h"
#include "hash.h"
#include "move_do.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "protocol.h"
#include "random.h"
#include "square.h"
#include "trans.h"
#include "util.h"
#include "value.h"
#include "vector.h"

// functions
bool NnueLoadEmbeddedNet();
void InitNnue();

// main()

int main(int argc, char * argv[]) {

   // init

   util_init();
   my_random_init(); // for opening book

// printf("Toga IV 1.1 by Fabien Letouzey, Thomas Gaksch, Jerry Donald Watson\n");
   printf("Rebel 15 UCI by Fabien Letouzey, Thomas Gaksch, Jerry Donald Watson, Chris Whittington and Ed Schroder\n");
      
   option_init();

   square_init();
   piece_init();
   pawn_init_bit();
   value_init();
   vector_init();
   attack_init();
   move_do_init();

   random_init();
   
   trans_init(Trans);
   hash_init();
   
   book_init();

#define NNUE_LOAD	1		// 0=HCE | 1=NNUE	@ed

#if NNUE_LOAD
   InitNnue();
   bool status = NnueLoadEmbeddedNet();
   if (!status)
   {
	   exit(1); // @ed

	   return EXIT_SUCCESS;
   }
#endif
   // loop

   loop();

   return EXIT_SUCCESS;
}

// end of main.cpp

