
// pst.cpp

// includes

#include "option.h"
#include "piece.h"
#include "pst.h"
#include "util.h"

#define PESTO 0

#if PESTO

// piece/square tables
// values from Rofchade: http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&start=10

int mg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};

int eg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0,
};

int mg_knight_table[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};

int eg_knight_table[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64,
};

int mg_bishop_table[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};

int eg_bishop_table[64] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3, -13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17,
};

int mg_rook_table[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};

int eg_rook_table[64] = {
    13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
     7,  7,  7,  5,  4,  -3,  -5,  -3,
     4,  3, 13,  1,  2,   1,  -1,   2,
     3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20,
};

int mg_queen_table[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};

int eg_queen_table[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41,
};

int mg_king_table[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};

int eg_king_table[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43
};


#endif


// macros

#define P(piece_12,square_64,stage) (Pst[(piece_12)][(square_64)][(stage)])

// constants

static const int A1=000, B1=001, C1=002, D1=003, E1=004, F1=005, G1=006, H1=007;
static const int A2=010, B2=011, C2=012, D2=013, E2=014, F2=015, G2=016, H2=017;
static const int A3=020, B3=021, C3=022, D3=023, E3=024, F3=025, G3=026, H3=027;
static const int A4=030, B4=031, C4=032, D4=033, E4=034, F4=035, G4=036, H4=037;
static const int A5=040, B5=041, C5=042, D5=043, E5=044, F5=045, G5=046, H5=047;
static const int A6=050, B6=051, C6=052, D6=053, E6=054, F6=055, G6=056, H6=057;
static const int A7=060, B7=061, C7=062, D7=063, E7=064, F7=065, G7=066, H7=067;
static const int A8=070, B8=071, C8=072, D8=073, E8=074, F8=075, G8=076, H8=077;

// constants and variables

static /* const */ int PieceSquareWeight = 256; // 100%
//static /* const */ int PieceActivityWeight = 256; // 100%
static /* const */ int KingSafetyWeight = 256; // 100%
static /* const */ int PawnStructureWeight = 256; // 100%

static const int PawnFileOpening = 5;
static const int KnightCentreOpening = 5;
static const int KnightCentreEndgame = 5;
static const int KnightRankOpening = 5;
static const int KnightBackRankOpening = 0;
static const int KnightTrapped = 100;
static const int BishopCentreOpening = 2;
static const int BishopCentreEndgame = 3;
static const int BishopBackRankOpening = 10;
static const int BishopDiagonalOpening = 4;
static const int RookFileOpening = 3;
static const int QueenCentreOpening = 0; // was 0
static const int QueenCentreEndgame = 4;
static const int QueenBackRankOpening = 5;
static const int KingCentreEndgame = 12;
static const int KingFileOpening = 10;
static const int KingRankOpening = 10;

// "constants"

static const int PawnFile[8] = {
   -3, -1, +0, +1, +1, +0, -1, -3,
};

static const int KnightLine[8] = {
   -4, -2, +0, +1, +1, +0, -2, -4,
};

static const int KnightRank[8] = {
   -2, -1, +0, +1, +2, +3, +2, +1,
};

static const int BishopLine[8] = {
   -3, -1, +0, +1, +1, +0, -1, -3,
};

static const int RookFile[8] = {
   -2, -1, +0, +1, +1, +0, -1, -2,
};

static const int QueenLine[8] = {
   -3, -1, +0, +1, +1, +0, -1, -3,
};

static const int KingLine[8] = {
   -3, -1, +0, +1, +1, +0, -1, -3,
};

static const int KingFile[8] = {
   +3, +4, +2, +0, +0, +2, +4, +3,
};

static const int KingRank[8] = {
   +1, +0, -2, -3, -4, -5, -6, -7,
};

// variables

sint16 Pst[12][64][StageNb];

// prototypes

static int square_make (int file, int rank);

static int square_file (int square);
static int square_rank (int square);
static int square_opp  (int square);

// functions

// pst_init()

void pst_parameter() {

   // UCI options

   PieceSquareWeight = (option_get_int("Piece Square Activity") * 256 + 50) / 100;
   KingSafetyWeight    = (option_get_int("King Safety")    * 256 + 50) / 100;
   PawnStructureWeight = (option_get_int("Pawn Structure") * 256 + 50) / 100;

}

void pst_init() {

   int i;
   int piece, sq, stage;

   // UCI options

   pst_parameter();

   // init

   for (piece = 0; piece < 12; piece++) {
      for (sq = 0; sq < 64; sq++) {
         for (stage = 0; stage < StageNb; stage++) {
            P(piece,sq,stage) = 0;
         }
      }
   }

   // pawns

   piece = WhitePawn12;

   // file

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += PawnFile[square_file(sq)] * PawnFileOpening;
   }

   // centre control

   P(piece,D3,Opening) += 10;
   P(piece,E3,Opening) += 10;

   P(piece,D4,Opening) += 20;
   P(piece,E4,Opening) += 20;

   P(piece,D5,Opening) += 10;
   P(piece,E5,Opening) += 10;

      // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * PawnStructureWeight) / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PawnStructureWeight) / 256;
   }

   // knights

   piece = WhiteKnight12;

   // centre

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += KnightLine[square_file(sq)] * KnightCentreOpening;
      P(piece,sq,Opening) += KnightLine[square_rank(sq)] * KnightCentreOpening;
      P(piece,sq,Endgame) += KnightLine[square_file(sq)] * KnightCentreEndgame;
      P(piece,sq,Endgame) += KnightLine[square_rank(sq)] * KnightCentreEndgame;
   }

   // rank

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += KnightRank[square_rank(sq)] * KnightRankOpening;
   }

   // back rank

   for (sq = A1; sq <= H1; sq++) { // HACK: only first rank
      P(piece,sq,Opening) -= KnightBackRankOpening;
   }

   // "trapped"

   P(piece,A8,Opening) -= KnightTrapped;
   P(piece,H8,Opening) -= KnightTrapped;

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * PieceSquareWeight) / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PieceSquareWeight) / 256;
   }

   // bishops

   piece = WhiteBishop12;

   // centre

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += BishopLine[square_file(sq)] * BishopCentreOpening;
      P(piece,sq,Opening) += BishopLine[square_rank(sq)] * BishopCentreOpening;
      P(piece,sq,Endgame) += BishopLine[square_file(sq)] * BishopCentreEndgame;
      P(piece,sq,Endgame) += BishopLine[square_rank(sq)] * BishopCentreEndgame;
   }

   // back rank

   for (sq = A1; sq <= H1; sq++) { // HACK: only first rank
      P(piece,sq,Opening) -= BishopBackRankOpening;
   }

   // main diagonals

   for (i = 0; i < 8; i++) {
      sq = square_make(i,i);
      P(piece,sq,Opening) += BishopDiagonalOpening;
      P(piece,square_opp(sq),Opening) += BishopDiagonalOpening;
   }

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * PieceSquareWeight) / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PieceSquareWeight) / 256;
   }

   // rooks

   piece = WhiteRook12;

   // file

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += RookFile[square_file(sq)] * RookFileOpening;
   }

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * PieceSquareWeight) / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PieceSquareWeight) / 256;
   }

   // queens

   piece = WhiteQueen12;

   // centre

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += QueenLine[square_file(sq)] * QueenCentreOpening;
      P(piece,sq,Opening) += QueenLine[square_rank(sq)] * QueenCentreOpening;
      P(piece,sq,Endgame) += QueenLine[square_file(sq)] * QueenCentreEndgame;
      P(piece,sq,Endgame) += QueenLine[square_rank(sq)] * QueenCentreEndgame;
   }

   // back rank

   for (sq = A1; sq <= H1; sq++) { // HACK: only first rank
      P(piece,sq,Opening) -= QueenBackRankOpening;
   }

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * PieceSquareWeight) / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PieceSquareWeight) / 256;
   }

   // kings

   piece = WhiteKing12;

   // centre

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Endgame) += KingLine[square_file(sq)] * KingCentreEndgame;
      P(piece,sq,Endgame) += KingLine[square_rank(sq)] * KingCentreEndgame;
   }

   // file

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += KingFile[square_file(sq)] * KingFileOpening;
   }

   // rank

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += KingRank[square_rank(sq)] * KingRankOpening;
   }

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * KingSafetyWeight)    / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PieceSquareWeight) / 256;
   }

   // symmetry copy for black

   for (piece = 0; piece < 12; piece += 2) { // HACK
      for (sq = 0; sq < 64; sq++) {
         for (stage = 0; stage < StageNb; stage++) {
            P(piece+1,sq,stage) = -P(piece,square_opp(sq),stage); // HACK
         }
      }
   }

#define SKIP_OUTPUT 0

#if PESTO
FILE *fp; int x;

double perc=1.0;


int convert[64] = {
      56, 57, 58, 59, 60, 61, 62, 63,
	  48, 49, 50, 51, 52, 53, 54, 55,
	  40, 41, 42, 43, 44, 45, 46, 47,
	  32, 33, 34, 35, 36, 37, 38, 39,
	  24, 25, 26, 27, 28, 29, 30, 31,
	  16, 17, 18, 19, 20, 21, 22, 23,
	   8,  9, 10, 11, 12, 13, 14, 15,
	   0,  1,  2,  3,  4,  5,  6,  7
};

#if SKIP_OUTPUT==0

/*
fp = fopen("fruit.txt","w");

	 for (sq = 0; sq < 64; sq++) { P(0,sq,0)*=perc; fprintf(fp,"%4d",P(0,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wp-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(0,sq,1)*=perc; fprintf(fp,"%4d",P(0,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wp-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { P(1,sq,0)*=perc; fprintf(fp,"%4d",P(1,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bp-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(1,sq,1)*=perc; fprintf(fp,"%4d",P(1,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bp-eg\n"); }   
     fprintf(fp,"\n\n");

     for (sq = 0; sq < 64; sq++) { P(2,sq,0)*=perc; fprintf(fp,"%4d",P(2,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wn-mg\n"); }	
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(2,sq,1)*=perc; fprintf(fp,"%4d",P(2,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wn-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { P(3,sq,0)*=perc; fprintf(fp,"%4d",P(3,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bn-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(3,sq,1)*=perc; fprintf(fp,"%4d",P(3,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bn-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { P(4,sq,0)*=perc; fprintf(fp,"%4d",P(4,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wb-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(4,sq,1)*=perc; fprintf(fp,"%4d",P(4,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wb-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { P(5,sq,0)*=perc; fprintf(fp,"%4d",P(5,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bb-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(5,sq,1)*=perc; fprintf(fp,"%4d",P(5,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bb-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { P(6,sq,0)*=perc; fprintf(fp,"%4d",P(6,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wr-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(6,sq,1)*=perc; fprintf(fp,"%4d",P(6,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wr-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { P(7,sq,0)*=perc; fprintf(fp,"%4d",P(7,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  br-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(7,sq,1)*=perc; fprintf(fp,"%4d",P(7,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  br-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { P(8,sq,0)*=perc; fprintf(fp,"%4d",P(8,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wq-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(8,sq,1)*=perc; fprintf(fp,"%4d",P(8,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wq-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { P(9,sq,0)*=perc; fprintf(fp,"%4d",P(9,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bq-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(9,sq,1)*=perc; fprintf(fp,"%4d",P(9,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bq-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { P(10,sq,0)*=perc; fprintf(fp,"%4d",P(10,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wk-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(10,sq,1)*=perc; fprintf(fp,"%4d",P(10,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wk-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { P(11,sq,0)*=perc; fprintf(fp,"%4d",P(11,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bk-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { P(11,sq,1)*=perc; fprintf(fp,"%4d",P(11,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bk-eg\n"); }   
     fprintf(fp,"\n\n");
fclose(fp);
*/


fp = fopen("pesto.txt","w");

    for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(0,sq,0)=mg_pawn_table[x]*perc; }
//	for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(0,sq,1)=eg_pawn_table[x]*perc; }

    for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(2,sq,0)=mg_knight_table[x]*perc; }
//	for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(2,sq,1)=eg_knight_table[x]*perc; }

    for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(4,sq,0)=mg_bishop_table[x]*perc; }
//	for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(4,sq,1)=eg_bishop_table[x]*perc; }

    for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(6,sq,0)=mg_rook_table[x]*perc; }
//	for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(6,sq,1)=eg_rook_table[x]*perc; }

    for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(8,sq,0)=mg_queen_table[x]*perc; }
//	for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(8,sq,1)=eg_queen_table[x]*perc; }

    for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(10,sq,0)=mg_king_table[x]*perc; }
//	for (sq = 0; sq < 64; sq++) { x=convert[sq]; P(10,sq,1)=eg_king_table[x]*perc; }

   // symmetry copy for black

   for (piece = 0; piece < 12; piece += 2) { 
      for (sq = 0; sq < 64; sq++) {
         for (stage = 0; stage < StageNb; stage++) {
            P(piece+1,sq,stage) = -P(piece,square_opp(sq),stage); } } }
                     
	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(0,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wp-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(0,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wp-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(1,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bp-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(1,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bp-eg\n"); }   
     fprintf(fp,"\n\n");

     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(2,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wn-mg\n"); }	
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(2,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wn-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(3,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bn-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(3,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bn-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(4,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wb-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(4,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wb-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(5,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bb-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(5,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bb-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(6,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wr-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(6,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wr-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(7,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  br-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(7,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  br-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(8,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wq-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(8,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wq-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(9,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bq-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(9,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bq-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(10,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wk-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(10,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  wk-eg\n"); }   
     fprintf(fp,"\n\n");

	 for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(11,sq,0)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bk-mg\n"); }
     fprintf(fp,"\n\n");
     for (sq = 0; sq < 64; sq++) { fprintf(fp,"%4d",P(11,sq,1)); if (sq==7 || sq==15 || sq==23 || sq==31 || sq==39 || sq==47 || sq==55) fprintf(fp,"  bk-eg\n"); }   
     fprintf(fp,"\n\n");

fclose(fp);

#endif
#endif

}

// square_make()

static int square_make(int file, int rank) {

   ASSERT(file>=0&&file<8);
   ASSERT(rank>=0&&rank<8);

   return (rank << 3) | file;
}

// square_file()

static int square_file(int square) {

   ASSERT(square>=0&&square<64);

   return square & 7;
}

// square_rank()

static int square_rank(int square) {

   ASSERT(square>=0&&square<64);

   return square >> 3;
}

// square_opp()

static int square_opp(int square) {

   ASSERT(square>=0&&square<64);

   return square ^ 070;
}

// end of pst.cpp
