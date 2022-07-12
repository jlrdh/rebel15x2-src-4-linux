
// pv.cpp

// includes

#include <cstring>

#include "board.h"
#include "move.h"
#include "move_do.h"
#include "pv.h"
#include "util.h"

// functions

// pv_is_ok()

bool pv_is_ok(const mv_t pv[]) {

   int POS;
   int move;

   if (pv == NULL) return false;

   for (POS = 0; true; POS++) {

      if (POS >= 256) return false;
      move = pv[POS];

      if (move == MoveNone) return true;
      if (!move_is_ok(move)) return false;
   }

   return true;
}

// pv_copy()

void pv_copy(mv_t dst[], const mv_t src[]) {

   ASSERT(pv_is_ok(src));
   ASSERT(dst!=NULL);

   while ((*dst++ = *src++) != MoveNone)
      ;
}

// pv_cat()

void pv_cat(mv_t dst[], const mv_t src[], int move) {

   ASSERT(pv_is_ok(src));
   ASSERT(dst!=NULL);

   *dst++ = move;

   while ((*dst++ = *src++) != MoveNone)
      ;
}

// pv_to_string()

bool pv_to_string(const mv_t pv[], char string[], int size) {

   int POS;
   int move;

   ASSERT(pv_is_ok(pv));
   ASSERT(string!=NULL);
   ASSERT(size>=512);

   // init

   if (size < 512) return false;

   POS = 0;

   // loop

   while ((move = *pv++) != MoveNone) {

      if (POS != 0) string[POS++] = ' ';

      move_to_string(move,&string[POS],size-POS);
      POS += strlen(&string[POS]);
   }

   string[POS] = '\0';

   return true;
}

// end of pv.cpp

