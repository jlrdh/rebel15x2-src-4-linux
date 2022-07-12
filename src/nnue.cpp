

//
// * This program is free software : you can redistribute itand /or modify
// * it under the terms of the GNU General Public License as published by
// * the Free Software Foundation, either version 3 of the License, or
// *(at your option) any later version.
// *
// *This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// * GNU General Public License for more details.
// *
// * You should have received a copy of the GNU General Public License
// * along with this program.If not, see < http://www.gnu.org/licenses/>.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>

#include "board.h"
#include "move_do.h"

// switch me off please ...
#define DEBUG_NNUE 0

#define UPDATE_ALWAYS 0


#include "nnue.h"
#include "globals.h"
#include "simd.h"

POSITIONDATA posdata[64][256];


#define TO_SHIFT 6
#define PROMO_SHIFT 12
#define CAPTYPE_SHIFT 16	// NB empty = 0b0111 shifted
#define MOVETYPE_SHIFT 19

// lowest 16 bits form hash16
// 29 wasteful bits at the moment
#define FROM_MASK		0b0000000000000000000111111ULL
#define TO_MASK			0b0000000000000111111000000ULL
#define PROMO_MASK		0b0000000000111000000000000ULL
#define CAPTURE_FLAG	0b0000000001000000000000000ULL
// above this points exceeds 16 bits
#define CAPTYPE_MASK	0b0000001110000000000000000ULL
#define MOVETYPE_MASK	0b0001110000000000000000000ULL
#define EP_MOVE_FLAG	0b0010000000000000000000000ULL
#define CASTLES_FLAG	0b0100000000000000000000000ULL
#define PAWNMOVE_FLAG	0b1000000000000000000000000ULL

inline int from_sq(int move)
{
	return (move & FROM_MASK);
}
inline int to_sq(int move)
{
	return ((move & TO_MASK) >> TO_SHIFT);
}
inline int cap_type(int move)
{
	return ((move & CAPTYPE_MASK) >> CAPTYPE_SHIFT);
}
inline int move_type(int move)
{
	return ((move & MOVETYPE_MASK) >> MOVETYPE_SHIFT);
}
inline int promo_type(int move)
{
	return ((move & PROMO_MASK) >> PROMO_SHIFT);
}

inline bool IsCastles(int move)
{
	return (move & CASTLES_FLAG);
}
inline bool IsKingSideCastles(int move)
{
	return (IsCastles(move) && ((to_sq(move) & 7) == 2));
}
inline bool IsEpCapture(int move)
{
	return (move & EP_MOVE_FLAG);
}
inline bool IsCapture(int move)
{
	return (move & CAPTURE_FLAG);
}
inline bool IsPromo(int move)
{
	return (move & PROMO_MASK);
}


inline u64 get_queens(POSITIONDATA* pos)
{
	return (pos->diagonals & pos->manhattans);
}

inline u64 get_bishops(POSITIONDATA* pos)
{
	return (pos->diagonals & ~pos->manhattans);
}

inline u64 get_rooks(POSITIONDATA* pos)
{
	return (pos->manhattans & ~pos->diagonals);
}

// 4 bit nibbles
// write piece type to minboard entry
// for empty entry, type = 0b0111
#define EMPTY 0b0111ULL

inline void piece_to_square(POSITIONDATA* pos, u64 s1, u64 type)
{
	int ix = s1 & 0b11ULL;
	u64 entry = pos->miniboard[ix] & (~(EMPTY << (s1 & 0b111100ULL)));
	pos->miniboard[ix] = entry | (type << (s1 & 0b111100ULL));
}

inline void clear_square(POSITIONDATA* pos, u64 s1)
{
	pos->miniboard[s1 & 0b11ULL] |= (EMPTY << (s1 & 0b111100ULL));
}

inline int square_to_piece(POSITIONDATA* pos, u64 s1)
{
	int type = (pos->miniboard[s1 & 0b11ULL] >> (s1 & 0b111100ULL)) & EMPTY;
	return type;
}

#if DEBUG_NNUE
bool IntegrityCheckPosition(POSITIONDATA* pos)
{
	bool status = TRUE;
	u64 all_pieces = pos->pieces[0] | pos->pieces[1];
	u64 pawns = pos->pawns;
	u64 nites = pos->nites;
	u64 bishops = get_bishops(pos);
	u64 rooks = get_rooks(pos);
	u64 queens = get_queens(pos);
	u64 kings = ((ONE << pos->king_sq[0]) | (ONE << pos->king_sq[1]));

	// test miniboard and piece bitboards match up

	for (u64 s1 = 0; s1 < 64; s1++)
	{
		u64 type = square_to_piece(pos, s1);
		if (type != EMPTY)
		{
			all_pieces ^= (ONE << s1);
			switch (type)
			{
			case NN_PAWN/2:
				pawns ^= (ONE << s1);
				break;
			case NN_KNIGHT/2:
				nites ^= (ONE << s1);
				break;
			case NN_BISHOP/2:
				bishops ^= (ONE << s1);
				break;
			case NN_ROOK/2:
				rooks ^= (ONE << s1);
				break;
			case NN_QUEEN/2:
				queens ^= (ONE << s1);
				break;
			case NN_KING/2:
				kings ^= (ONE << s1);
				break;
			default:
				status = FALSE;
				assert(1 == 2);
			}
		}
	}
	status = !(all_pieces | pawns | nites | bishops | rooks | queens | kings);
	return status;
}
#endif

// ================================================


inline int transform_square(int s1, int side)
{
	return (side == BLACK) ? s1 ^ 56 : s1;
}

// piece = 2*type + stm=BLACK, eg black_king = 11 (2*5 + 1)
static int CalculateFeatureIndex(int sq, int piece, int king_sq, int side)
{
	sq = transform_square(sq, side);
	piece ^= side;
#if (NUM_PT == 11)
	if (piece == 11)
		piece -= 1;
#endif
	return (sq + (piece * NUM_SQ) + NUM_PT * NUM_SQ * king_sq);
}


#if (USE_AUXILIARY > 0)
static int CalculateQCheckFeatureIndex(int check_sq, int side)
{
	int index;
	check_sq = transform_square(check_sq, side);
	index = NUM_INPUT_FEATURES + (side * NUM_SQ) + check_sq;
	return index;
}
#endif


static void FindActiveFeatures(POSITIONDATA* pos, int side,
								 struct feature_list *list)
{
	int king_sq;
	int sq;
	u32 index;
	u64 bb;

	list->size = 0;

	king_sq = transform_square(pos->king_sq[side], side);

	// bitboard of all pieces excluding the two kings if HalfKP
	bb = (pos->pieces[WHITE] | pos->pieces[BLACK]);
	if (NUM_PT == 10)
	{
		bb &= (~((ONE << pos->king_sq[WHITE]) | (ONE << pos->king_sq[BLACK])));
	}

	// feature index
	while (bb)
	{
		sq = scanforward64(bb);
		bb ^= (ONE << sq);
		
		int t1 = square_to_piece(pos, sq) * 2 + (!((ONE << sq) & pos->pieces[WHITE]));
		index = CalculateFeatureIndex(sq, t1, king_sq, side);
		list->features[list->size++] = index;
	}
#if (USE_AUXILIARY > 0)
	bb = pos->safeQchecks[side ^ 1];
	while (bb)
	{
		sq = scanforward64(bb);
		bb ^= (ONE << sq);
		// calculate the feature index for this Qcheck_sq and add it to the list
		index = CalculateQCheckFeatureIndex(sq, side);
		list->features[list->size++] = index;
	}
#endif
	assert(list->size <= MAX_ACTIVE_FEATURES);
}

static bool FindChangedFeatures(POSITIONDATA* pos, int side,
									struct dirty_pieces* dp,
									struct feature_list *added,
									struct feature_list *removed)
{
	int            king_sq;
	int            piece;
	u32            index;

	// Init
	added->size = 0;
	removed->size = 0;

	// king
	king_sq = transform_square(pos->king_sq[side], side);

	// dirties loop
	for (int i=0; i < dp->ndirty; i++) 
	{
		piece = dp->piece[i];

		// ignore the two kings for HalfKP
		if ((VALUE(piece) == NN_KING) && (NUM_PT==10))
		{
			continue;
		}

		// removed or added features
		if (dp->from[i] != NN_NO_SQUARE) 
		{
			index = CalculateFeatureIndex(dp->from[i], piece, king_sq, side);
			removed->features[removed->size++] = index;
		}
		if (dp->to[i] != NN_NO_SQUARE) 
		{
			index = CalculateFeatureIndex(dp->to[i], piece, king_sq, side);
			added->features[added->size++] = index;
		}
	}
#if (USE_AUXILIARY > 0)
	u64 bb_add;
	u64 bb_sub;
	int sq;
	bb_add = pos->safeQchecks[side ^ 1] & ~(pos - 1)->safeQchecks[side ^ 1];
	bb_sub = ~pos->safeQchecks[side ^ 1] & (pos - 1)->safeQchecks[side ^ 1];
	while (bb_add)
	{
		sq = scanforward64(bb_add);
		bb_add ^= (ONE << sq);
		// calculate the feature index for this Qcheck_sq and add it to the list
		index = CalculateQCheckFeatureIndex(sq, side);
		added->features[added->size++] = index;
	}
	while (bb_sub)
	{
		sq = scanforward64(bb_sub);
		bb_sub ^= (ONE << sq);
		// calculate the feature index for this Qcheck_sq and add it to the list
		index = CalculateQCheckFeatureIndex(sq, side);
		removed->features[removed->size++] = index;
	}
#endif
	assert(added->size <= MAX_ACTIVE_FEATURES);
	assert(removed->size <= MAX_ACTIVE_FEATURES);
	return false;
}

#if (USE_AVX2==true)
static void FullUpdate(POSITIONDATA* pos, int side)
{
	struct feature_list active_features;
	u32            index;

	// Find all active features
	FindActiveFeatures(pos, side, &active_features);

#if (N_PSQT>0)
	__m256i psqtacc;
	int32_t* psqtweights = layers[0].psqtweights.i32;

	// we can load[0] without testing because it is not possible to have no active_features
	// not true, might be lone king HALFKV
	// ALSO DO SSE VERSION !!!!!!!!!!! ???
	if (active_features.size)
		psqtacc = _mm256_load_si256((__m256i*) & psqtweights[active_features.features[0] * N_PSQT]);
	else
		psqtacc = _mm256_setzero_si256();

	for (int i = 1; i < active_features.size; i++)
	{
		index = active_features.features[i];
#if (USE_AUXILIARY > 0)
		if (index >= NUM_INPUT_FEATURES)
			continue;
#endif
		psqtacc = _mm256_add_epi32(psqtacc, (__m256i) * ((__m256i*) & psqtweights[index * N_PSQT]));
	}
	vec_store((__m256i*) & pos->eval_stack.state.psqt_acc[side][0], psqtacc);
#endif
	constexpr int register_width = 256 / 16;
#if 1
	int16_t* accumulator;
	int16_t* biases;
	int16_t* weights;

	int n_chunks = (HALFKX_LAYER_SIZE > 256) ? 2 : 1;
	for (int chunk = 0; chunk < n_chunks; chunk++)
	{
		biases = &layers[0].biases.i16[0];
		biases += chunk * 16 * register_width;
		__m256i reg0 = _mm256_load_si256((__m256i*) & biases[0 * register_width]);
		__m256i reg1 = _mm256_load_si256((__m256i*) & biases[1 * register_width]);
		__m256i reg2 = _mm256_load_si256((__m256i*) & biases[2 * register_width]);
		__m256i reg3 = _mm256_load_si256((__m256i*) & biases[3 * register_width]);
		__m256i reg4 = _mm256_load_si256((__m256i*) & biases[4 * register_width]);
		__m256i reg5 = _mm256_load_si256((__m256i*) & biases[5 * register_width]);
		__m256i reg6 = _mm256_load_si256((__m256i*) & biases[6 * register_width]);
		__m256i reg7 = _mm256_load_si256((__m256i*) & biases[7 * register_width]);
#if (HALFKX_LAYER_SIZE > 128)
		__m256i reg8 = _mm256_load_si256((__m256i*) & biases[8 * register_width]);
		__m256i reg9 = _mm256_load_si256((__m256i*) & biases[9 * register_width]);
		__m256i reg10 = _mm256_load_si256((__m256i*) & biases[10 * register_width]);
		__m256i reg11 = _mm256_load_si256((__m256i*) & biases[11 * register_width]);
		__m256i reg12 = _mm256_load_si256((__m256i*) & biases[12 * register_width]);
		__m256i reg13 = _mm256_load_si256((__m256i*) & biases[13 * register_width]);
		__m256i reg14 = _mm256_load_si256((__m256i*) & biases[14 * register_width]);
		__m256i reg15 = _mm256_load_si256((__m256i*) & biases[15 * register_width]);
#endif
		// Add the weights for all active features
		for (int a = 0; a < active_features.size; a++)
		{
			weights = &layers[0].weights.i16[HALFKX_LAYER_SIZE * active_features.features[a]];
			weights += chunk * 16 * register_width;
			reg0 = _mm256_add_epi16(reg0, (__m256i) * ((__m256i*) & weights[0 * register_width]));
			reg1 = _mm256_add_epi16(reg1, (__m256i) * ((__m256i*) & weights[1 * register_width]));
			reg2 = _mm256_add_epi16(reg2, (__m256i) * ((__m256i*) & weights[2 * register_width]));
			reg3 = _mm256_add_epi16(reg3, (__m256i) * ((__m256i*) & weights[3 * register_width]));
			reg4 = _mm256_add_epi16(reg4, (__m256i) * ((__m256i*) & weights[4 * register_width]));
			reg5 = _mm256_add_epi16(reg5, (__m256i) * ((__m256i*) & weights[5 * register_width]));
			reg6 = _mm256_add_epi16(reg6, (__m256i) * ((__m256i*) & weights[6 * register_width]));
			reg7 = _mm256_add_epi16(reg7, (__m256i) * ((__m256i*) & weights[7 * register_width]));
#if (HALFKX_LAYER_SIZE > 128)
			reg8 = _mm256_add_epi16(reg8, (__m256i) * ((__m256i*) & weights[8 * register_width]));
			reg9 = _mm256_add_epi16(reg9, (__m256i) * ((__m256i*) & weights[9 * register_width]));
			reg10 = _mm256_add_epi16(reg10, (__m256i) * ((__m256i*) & weights[10 * register_width]));
			reg11 = _mm256_add_epi16(reg11, (__m256i) * ((__m256i*) & weights[11 * register_width]));
			reg12 = _mm256_add_epi16(reg12, (__m256i) * ((__m256i*) & weights[12 * register_width]));
			reg13 = _mm256_add_epi16(reg13, (__m256i) * ((__m256i*) & weights[13 * register_width]));
			reg14 = _mm256_add_epi16(reg14, (__m256i) * ((__m256i*) & weights[14 * register_width]));
			reg15 = _mm256_add_epi16(reg15, (__m256i) * ((__m256i*) & weights[15 * register_width]));
#endif
		}
		// finally, write into accumulator
		accumulator = &pos->eval_stack.state.accumulator[side][0];
		accumulator += chunk * 16 * register_width;
		_mm256_store_si256((__m256i*) & accumulator[0 * register_width], reg0);
		_mm256_store_si256((__m256i*) & accumulator[1 * register_width], reg1);
		_mm256_store_si256((__m256i*) & accumulator[2 * register_width], reg2);
		_mm256_store_si256((__m256i*) & accumulator[3 * register_width], reg3);
		_mm256_store_si256((__m256i*) & accumulator[4 * register_width], reg4);
		_mm256_store_si256((__m256i*) & accumulator[5 * register_width], reg5);
		_mm256_store_si256((__m256i*) & accumulator[6 * register_width], reg6);
		_mm256_store_si256((__m256i*) & accumulator[7 * register_width], reg7);
#if (HALFKX_LAYER_SIZE > 128)
		_mm256_store_si256((__m256i*) & accumulator[8 * register_width], reg8);
		_mm256_store_si256((__m256i*) & accumulator[9 * register_width], reg9);
		_mm256_store_si256((__m256i*) & accumulator[10 * register_width], reg10);
		_mm256_store_si256((__m256i*) & accumulator[11 * register_width], reg11);
		_mm256_store_si256((__m256i*) & accumulator[12 * register_width], reg12);
		_mm256_store_si256((__m256i*) & accumulator[13 * register_width], reg13);
		_mm256_store_si256((__m256i*) & accumulator[14 * register_width], reg14);
		_mm256_store_si256((__m256i*) & accumulator[15 * register_width], reg15);
#endif
	}
#else
	constexpr int num_chunks = HALFKX_LAYER_SIZE / register_width;
	__m256i regs[num_chunks];

	for (int i = 0; i < num_chunks; i++)
	{
		regs[i] = _mm256_load_si256((__m256i*) & layers[0].biases.i16[i * register_width]);
	}
	// Add the weights for all active features
	for (int a = 0; a < active_features.size; a++)
	{
		index = active_features.features[a];
		offset = HALFKX_LAYER_SIZE * index;
		for (int i = 0; i < num_chunks; i++)
		{
			regs[i] = _mm256_add_epi16(regs[i], (__m256i) * ((__m256i*) & layers[0].weights.i16[offset + i * register_width]));
		}
	}
	// finally, write into accumulator
	for (int i = 0; i < num_chunks; i++)
	{
		// nnue_input_state&
		_mm256_store_si256((__m256i*) & pos->eval_stack.state.accumulator[side][i * register_width], regs[i]);
	}
#endif
}
#endif


#if (USE_SSE==true)
static void FullUpdate(POSITIONDATA* pos, int side)
{
	struct feature_list active_features;
	u32            offset;
	u32            index;
	int16* acc;

	// Find all active features
	FindActiveFeatures(pos, side, &active_features);

	// Setup data pointer
	acc = &pos->eval_stack.state.accumulator[side][0];

	// Add biases
	simd_copy_i16(layers[0].biases.i16, acc, HALFKX_LAYER_SIZE);

	// Summarise the weights for all active features
	for (int i = 0; i < active_features.size; i++)
	{
		index = active_features.features[i];
		offset = HALFKX_LAYER_SIZE * index;
		simd_add_i16(&layers[0].weights.i16[offset], acc, HALFKX_LAYER_SIZE);
	}
#if (N_PSQT>0)
	__m128i psqtacc0;
	__m128i psqtacc1;
	psqtacc0 = vec_zero_psqt();
	psqtacc1 = vec_zero_psqt();
	for (int i = 0; i < active_features.size; i++)
	{
		index = active_features.features[i];
#if (USE_AUXILIARY > 0)

		if (index >= NUM_INPUT_FEATURES)
			continue;
#endif
		offset = index * N_PSQT;
		__m128i temp = vec_load((__m128i*)(&layers[0].psqtweights.i32[offset]));
		psqtacc0 = vec_add_psqt_32(psqtacc0, temp);
		temp = vec_load((__m128i*)(&layers[0].psqtweights.i32[offset + 4]));
		psqtacc1 = vec_add_psqt_32(psqtacc1, temp);
	}
	vec_store((__m128i*) & pos->eval_stack.state.psqt_acc[side][0], psqtacc0);
	vec_store((__m128i*) & pos->eval_stack.state.psqt_acc[side][0 + 4], psqtacc1);
#endif
}
#endif


#if (USE_AVX2==true)
static void IncrementalUpdate(POSITIONDATA* pos_to, POSITIONDATA* pos_from, struct dirty_pieces* dp, int side)
{
	struct feature_list added;
	struct feature_list removed;

	u32            index;

	if (dp->piece[0] == NN_NO_PIECE)
	{	// null move
		added.size = 0;
		removed.size = 0;
	}
	else
	{
		FindChangedFeatures(pos_to, side, dp, &added, &removed);
	}

#if (N_PSQT>0)
	//
	__m256i psqtacc;
	int32_t* psqtweights = layers[0].psqtweights.i32;
	//psqtacc = vec_zero_psqt();
	// Copy the state from previous position
	psqtacc = _mm256_load_si256((__m256i*) & pos_from->eval_stack.state.psqt_acc[side][0]);
	// Subtract weights for removed features
	for (int i = 0; i < removed.size; i++)
	{
		index = removed.features[i];
#if (USE_AUXILIARY > 0)
		if (index >= NUM_INPUT_FEATURES)
			continue;
#endif
		//offset = index * N_PSQT;
		//__m256i temp = vec_load((__m256i*)(&layers[0].psqtweights.i32[offset]));
		//psqtacc = vec_add_psqt_32(psqtacc, temp);
		psqtacc = _mm256_sub_epi32(psqtacc, (__m256i) * ((__m256i*) & psqtweights[index * N_PSQT]));
	}
	// add weights for added features
	for (int i = 0; i < added.size; i++)
	{
		index = added.features[i];
#if (USE_AUXILIARY > 0)
		if (index >= NUM_INPUT_FEATURES)
			continue;
#endif
		//offset = index * N_PSQT;
		//__m256i temp = vec_load((__m256i*)(&layers[0].psqtweights.i32[offset]));
		//psqtacc = vec_add_psqt_32(psqtacc, temp);
		psqtacc = _mm256_add_epi32(psqtacc, (__m256i) * ((__m256i*) & psqtweights[index * N_PSQT]));
	}
	vec_store((__m256i*) & pos_to->eval_stack.state.psqt_acc[side][0], psqtacc);
#endif

	constexpr int register_width = 256 / 16;
#if 1
	int16_t* accumulator;
	int16_t* weights;
	int n_chunks = (HALFKX_LAYER_SIZE > 256) ? 2 : 1;
	for (int chunk = 0; chunk < n_chunks; chunk++)
	{
		// load previous acc
		accumulator = &pos_from->eval_stack.state.accumulator[side][0];
		accumulator += chunk * 16 * register_width;
		__m256i reg0 = _mm256_load_si256((__m256i*) & accumulator[0 * register_width]);
		__m256i reg1 = _mm256_load_si256((__m256i*) & accumulator[1 * register_width]);
		__m256i reg2 = _mm256_load_si256((__m256i*) & accumulator[2 * register_width]);
		__m256i reg3 = _mm256_load_si256((__m256i*) & accumulator[3 * register_width]);
		__m256i reg4 = _mm256_load_si256((__m256i*) & accumulator[4 * register_width]);
		__m256i reg5 = _mm256_load_si256((__m256i*) & accumulator[5 * register_width]);
		__m256i reg6 = _mm256_load_si256((__m256i*) & accumulator[6 * register_width]);
		__m256i reg7 = _mm256_load_si256((__m256i*) & accumulator[7 * register_width]);
#if (HALFKX_LAYER_SIZE > 128)
		__m256i reg8 = _mm256_load_si256((__m256i*) & accumulator[8 * register_width]);
		__m256i reg9 = _mm256_load_si256((__m256i*) & accumulator[9 * register_width]);
		__m256i reg10 = _mm256_load_si256((__m256i*) & accumulator[10 * register_width]);
		__m256i reg11 = _mm256_load_si256((__m256i*) & accumulator[11 * register_width]);
		__m256i reg12 = _mm256_load_si256((__m256i*) & accumulator[12 * register_width]);
		__m256i reg13 = _mm256_load_si256((__m256i*) & accumulator[13 * register_width]);
		__m256i reg14 = _mm256_load_si256((__m256i*) & accumulator[14 * register_width]);
		__m256i reg15 = _mm256_load_si256((__m256i*) & accumulator[15 * register_width]);
#endif
		// remove the weights of the removed feature
		for (int a = 0; a < removed.size; a++)
		{
			weights = &layers[0].weights.i16[HALFKX_LAYER_SIZE * removed.features[a]];
			weights += chunk * 16 * register_width;
			reg0 = _mm256_sub_epi16(reg0, (__m256i) * ((__m256i*) & weights[0 * register_width]));
			reg1 = _mm256_sub_epi16(reg1, (__m256i) * ((__m256i*) & weights[1 * register_width]));
			reg2 = _mm256_sub_epi16(reg2, (__m256i) * ((__m256i*) & weights[2 * register_width]));
			reg3 = _mm256_sub_epi16(reg3, (__m256i) * ((__m256i*) & weights[3 * register_width]));
			reg4 = _mm256_sub_epi16(reg4, (__m256i) * ((__m256i*) & weights[4 * register_width]));
			reg5 = _mm256_sub_epi16(reg5, (__m256i) * ((__m256i*) & weights[5 * register_width]));
			reg6 = _mm256_sub_epi16(reg6, (__m256i) * ((__m256i*) & weights[6 * register_width]));
			reg7 = _mm256_sub_epi16(reg7, (__m256i) * ((__m256i*) & weights[7 * register_width]));
#if (HALFKX_LAYER_SIZE > 128)
			reg8 = _mm256_sub_epi16(reg8, (__m256i) * ((__m256i*) & weights[8 * register_width]));
			reg9 = _mm256_sub_epi16(reg9, (__m256i) * ((__m256i*) & weights[9 * register_width]));
			reg10 = _mm256_sub_epi16(reg10, (__m256i) * ((__m256i*) & weights[10 * register_width]));
			reg11 = _mm256_sub_epi16(reg11, (__m256i) * ((__m256i*) & weights[11 * register_width]));
			reg12 = _mm256_sub_epi16(reg12, (__m256i) * ((__m256i*) & weights[12 * register_width]));
			reg13 = _mm256_sub_epi16(reg13, (__m256i) * ((__m256i*) & weights[13 * register_width]));
			reg14 = _mm256_sub_epi16(reg14, (__m256i) * ((__m256i*) & weights[14 * register_width]));
			reg15 = _mm256_sub_epi16(reg15, (__m256i) * ((__m256i*) & weights[15 * register_width]));
#endif
		}
		// add the weights of the added features
		for (int a = 0; a < added.size; a++)
		{
			weights = &layers[0].weights.i16[HALFKX_LAYER_SIZE * added.features[a]];
			weights += chunk * 16 * register_width;
			reg0 = _mm256_add_epi16(reg0, (__m256i) * ((__m256i*) & weights[0 * register_width]));
			reg1 = _mm256_add_epi16(reg1, (__m256i) * ((__m256i*) & weights[1 * register_width]));
			reg2 = _mm256_add_epi16(reg2, (__m256i) * ((__m256i*) & weights[2 * register_width]));
			reg3 = _mm256_add_epi16(reg3, (__m256i) * ((__m256i*) & weights[3 * register_width]));
			reg4 = _mm256_add_epi16(reg4, (__m256i) * ((__m256i*) & weights[4 * register_width]));
			reg5 = _mm256_add_epi16(reg5, (__m256i) * ((__m256i*) & weights[5 * register_width]));
			reg6 = _mm256_add_epi16(reg6, (__m256i) * ((__m256i*) & weights[6 * register_width]));
			reg7 = _mm256_add_epi16(reg7, (__m256i) * ((__m256i*) & weights[7 * register_width]));
#if (HALFKX_LAYER_SIZE > 128)
			reg8 = _mm256_add_epi16(reg8, (__m256i) * ((__m256i*) & weights[8 * register_width]));
			reg9 = _mm256_add_epi16(reg9, (__m256i) * ((__m256i*) & weights[9 * register_width]));
			reg10 = _mm256_add_epi16(reg10, (__m256i) * ((__m256i*) & weights[10 * register_width]));
			reg11 = _mm256_add_epi16(reg11, (__m256i) * ((__m256i*) & weights[11 * register_width]));
			reg12 = _mm256_add_epi16(reg12, (__m256i) * ((__m256i*) & weights[12 * register_width]));
			reg13 = _mm256_add_epi16(reg13, (__m256i) * ((__m256i*) & weights[13 * register_width]));
			reg14 = _mm256_add_epi16(reg14, (__m256i) * ((__m256i*) & weights[14 * register_width]));
			reg15 = _mm256_add_epi16(reg15, (__m256i) * ((__m256i*) & weights[15 * register_width]));
#endif
		}
		// finally, write back into current accumulator
		accumulator = &pos_to->eval_stack.state.accumulator[side][0];
		accumulator += chunk * 16 * register_width;
		_mm256_store_si256((__m256i*) & accumulator[0 * register_width], reg0);
		_mm256_store_si256((__m256i*) & accumulator[1 * register_width], reg1);
		_mm256_store_si256((__m256i*) & accumulator[2 * register_width], reg2);
		_mm256_store_si256((__m256i*) & accumulator[3 * register_width], reg3);
		_mm256_store_si256((__m256i*) & accumulator[4 * register_width], reg4);
		_mm256_store_si256((__m256i*) & accumulator[5 * register_width], reg5);
		_mm256_store_si256((__m256i*) & accumulator[6 * register_width], reg6);
		_mm256_store_si256((__m256i*) & accumulator[7 * register_width], reg7);
#if (HALFKX_LAYER_SIZE > 128)
		_mm256_store_si256((__m256i*) & accumulator[8 * register_width], reg8);
		_mm256_store_si256((__m256i*) & accumulator[9 * register_width], reg9);
		_mm256_store_si256((__m256i*) & accumulator[10 * register_width], reg10);
		_mm256_store_si256((__m256i*) & accumulator[11 * register_width], reg11);
		_mm256_store_si256((__m256i*) & accumulator[12 * register_width], reg12);
		_mm256_store_si256((__m256i*) & accumulator[13 * register_width], reg13);
		_mm256_store_si256((__m256i*) & accumulator[14 * register_width], reg14);
		_mm256_store_si256((__m256i*) & accumulator[15 * register_width], reg15);
#endif
	}
#else
	constexpr int num_chunks = HALFKX_LAYER_SIZE / register_width;
	__m256i regs[num_chunks];
	// load previous acc
	for (int i = 0; i < num_chunks; i++)
	{
		regs[i] = _mm256_load_si256((__m256i*) & pos_from->eval_stack.state.accumulator[side][i * register_width]);
	}
	// remove the weights of the removed feature
	for (int a = 0; a < removed.size; a++)
	{
		index = removed.features[a];
		offset = HALFKX_LAYER_SIZE * index;
		for (int i = 0; i < num_chunks; i++)
		{
			regs[i] = _mm256_sub_epi16(regs[i], (__m256i) * ((__m256i*) & layers[0].weights.i16[offset + i * register_width]));
		}
	}
	// add the weights of the added feature
	for (int a = 0; a < added.size; a++)
	{
		index = added.features[a];
		offset = HALFKX_LAYER_SIZE * index;
		for (int i = 0; i < num_chunks; i++)
		{
			regs[i] = _mm256_add_epi16(regs[i], (__m256i) * ((__m256i*) & layers[0].weights.i16[offset + i * register_width]));
		}
	}
	// finally, write to current accumulator
	for (int i = 0; i < num_chunks; i++)
	{
		_mm256_store_si256((__m256i*) & pos_to->eval_stack.state.accumulator[side][i * register_width], regs[i]);
	}
#endif
}
#endif



#if (USE_SSE==true)
static void IncrementalUpdate(POSITIONDATA* pos_to, POSITIONDATA* pos_from, struct dirty_pieces* dp, int side)
{
	struct feature_list added;
	struct feature_list removed;
	//struct dirty_pieces *dp = &pos_to->eval_stack.dirty_pieces;
	u32            offset;
	u32            index;

	if (dp->piece[0] == NN_NO_PIECE)
	{
		added.size = 0;
		removed.size = 0;
	}
	else
	{
		FindChangedFeatures(pos_to, side, dp, &added, &removed);
	}

	int16* acc;
	int16* prev_acc;

	acc = &pos_to->eval_stack.state.accumulator[side][0];
	prev_acc = &pos_from->eval_stack.state.accumulator[side][0];

	// Copy the state from previous position
	simd_copy_i16(prev_acc, acc, HALFKX_LAYER_SIZE);

	// Subtract weights for removed features
	for (int i = 0; i < removed.size; i++)
	{
		index = removed.features[i];
		offset = HALFKX_LAYER_SIZE * index;
		simd_sub_i16(&layers[0].weights.i16[offset], acc, HALFKX_LAYER_SIZE);
	}

	// Add weights for added features
	for (int i = 0; i < added.size; i++)
	{
		index = added.features[i];
		offset = HALFKX_LAYER_SIZE * index;
		simd_add_i16(&layers[0].weights.i16[offset], acc, HALFKX_LAYER_SIZE);
	}

#if (N_PSQT>0)
	__m128i psqtacc0;
	__m128i psqtacc1;
	// Copy the state from previous position
	psqtacc0 = vec_load((__m128i*) (&pos_from->eval_stack.state.psqt_acc[side][0]));
	psqtacc1 = vec_load((__m128i*) (&pos_from->eval_stack.state.psqt_acc[side][0 + 4]));
	// Subtract weights for removed features
	for (int i = 0; i < removed.size; i++)
	{
		index = removed.features[i];
#if (USE_AUXILIARY > 0)
		if (index >= NUM_INPUT_FEATURES)
			continue;
#endif
		offset = N_PSQT * index;
		__m128i temp = vec_load((__m128i*)(&layers[0].psqtweights.i32[offset]));
		psqtacc0 = vec_sub_psqt_32(psqtacc0, temp);
		temp = vec_load((__m128i*)(&layers[0].psqtweights.i32[offset + 4]));
		psqtacc1 = vec_sub_psqt_32(psqtacc1, temp);
	}

	// Add weights for added features
	for (int i = 0; i < added.size; i++)
	{
		index = added.features[i];
#if (USE_AUXILIARY > 0)
		if (index >= NUM_INPUT_FEATURES)
			continue;
#endif
		offset = N_PSQT * index;
		__m128i temp = vec_load((__m128i*)(&layers[0].psqtweights.i32[offset]));
		psqtacc0 = vec_add_psqt_32(psqtacc0, temp);
		temp = vec_load((__m128i*)(&layers[0].psqtweights.i32[offset + 4]));
		psqtacc1 = vec_add_psqt_32(psqtacc1, temp);
	}
	vec_store((__m128i*) (&pos_to->eval_stack.state.psqt_acc[side][0]), psqtacc0);
	vec_store((__m128i*) (&pos_to->eval_stack.state.psqt_acc[side][0 + 4]), psqtacc1);
#endif
}
#endif

static void HalfKxLayerForward(POSITIONDATA *pos, int move, struct net_data *data, int us, int nply)
{
	int      perspectives[2];
	int      side;
	uint32_t offset;
	uint8_t  *temp;
	int16_t  *features;


	if (!nply)
	{	// root, always full refresh, both sides accumulator
		for (side = 0; side < 2; side++)
		{
			FullUpdate(pos, side);
}
	}
	else
	{
		for (side = 0; side < 2; side++)
		{
			if (pos->eval_stack.state.valid[side])
			{
				continue;
			}
			if (pos->eval_stack.dirty_pieces.piece[0] == (side + NN_KING))
			{	// if king of stm has moved, refresh accumulator
				FullUpdate(pos, side);
			}
			else
			{
				if ((pos - 1)->eval_stack.state.valid[side])
				{	// if we find a valid accumulator, do incremental update
					struct dirty_pieces* dp = &pos->eval_stack.dirty_pieces;
					IncrementalUpdate(pos, pos - 1, dp, side);
				}
				else
				{
					FullUpdate(pos, side);
				}
			}
		}
	}
	// flag accumulator state as valid
	pos->eval_stack.state.valid[WHITE] = true;
	pos->eval_stack.state.valid[BLACK] = true;

#if 0
	// heavy duty testing ....
	// write acc to temp buffer (psqt and normal one)
	// rebuild accumulator
	// compare to temp buffer
	int16 temp_acc[2][HALFKX_LAYER_SIZE];
#if (N_PSQT>0)
	int32 temp_psqt[2][N_PSQT];
#endif

	memcpy(&temp_acc[0][0], &pos->eval_stack.state.accumulator[0][0], 2 * HALFKX_LAYER_SIZE * sizeof(int16));
#if (N_PSQT>0)
	memcpy(&temp_psqt[0][0], &pos->eval_stack.state.psqt_acc[0][0], 2 * N_PSQT * sizeof(int32));
#endif
	for (side = 0; side < 2; side++)
	{
		FullUpdate(pos, side);
	}
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < HALFKX_LAYER_SIZE; j++)
		{
			assert(temp_acc[i][j] == pos->eval_stack.state.accumulator[i][j]);
		}
	}
#if (N_PSQT>0)
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < N_PSQT; j++)
		{
			assert(temp_psqt[i][j] == pos->eval_stack.state.psqt_acc[i][j]);
		}
	}
#endif
#endif

	perspectives[0] = us;
	perspectives[1] = us ^ 1;
	for (side=0; side < 2; side++) 
	{
		offset = HALFKX_LAYER_SIZE*side;
		temp = &data->output[offset];
		features = pos->eval_stack.state.accumulator[perspectives[side]];
		simd_clamp(features, temp, HALFKX_LAYER_SIZE);
	}

#if (N_PSQT>0)
	// we are assuming N_PSQT=8
	int psqt_phase = (int)((popcount64(pos->pieces[0] | pos->pieces[1]) - 1) / 4);
	data->mtrl = (pos->eval_stack.state.psqt_acc[0][psqt_phase] - pos->eval_stack.state.psqt_acc[1][psqt_phase]) / 2;
	data->mtrl = data->mtrl * (us ? -1 : 1);
#endif
}

static void LinearLayerForward(int idx, struct net_data *data, bool output)
{
	simd_linear_forward(data->output, data->intermediate, layer_sizes[idx-1],
						layer_sizes[idx], layers[idx].biases.i32,
						layers[idx].weights.i8);
	if (!output) 
	{
		simd_scale_and_clamp(data->intermediate, data->output,
							 ACTIVATION_SCALE_BITS, layer_sizes[idx]);
	}
}

static void NetworkForward(POSITIONDATA* pos, int move, struct net_data *data, int us, int nply)
{
	int i;
	HalfKxLayerForward(pos, move, data, us, nply);
	for (i=1; i < NUM_LAYERS-1; i++) 
	{
		LinearLayerForward(i, data, false);
	}
	LinearLayerForward(i, data, true);
}


static bool ParseHeader(u8** data)
{
	u8* iter = *data;
#if 1
	u8 version;
	u8 n_psqt;
	u8 unused1;
	u8 unused2;
	version = *iter++;
	n_psqt = *iter++;
	unused1 = *iter++;
	unused2 = *iter++;
#else
	u32 version;
	// NNUE version number
	version = read_u32_le(iter);
	iter += 4;
#endif
	* data = iter;
	return ((version == NET_VERSION) && (n_psqt == N_PSQT));
}


static int ParseNetwork(u8** data)
{
	u8* iter = *data;
	int     i, j;
	int count = NET_HEADER_SIZE;

	// Read biases for the HalfKX layer
	for (i = 0; i < HALFKX_LAYER_SIZE; i++, iter += 2)
	{
		layers[0].biases.i16[i] = read_u16_le(iter);
		count += 2;
	}
	// Read weights for the HalfKX layer
	for (i = 0; i < HALFKX_LAYER_SIZE * (NUM_INPUT_FEATURES + NUM_AUXILIARY_FEATURES); i++, iter += 2)
	{
		layers[0].weights.i16[i] = read_u16_le(iter);
		count += 2;
	}
#if (N_PSQT>0)
	// Read weights for PSQT layer
	for (i = 0; i < (N_PSQT * NUM_INPUT_FEATURES); i++, iter += 4)
	{
		layers[0].psqtweights.i32[i] = read_u32_le(iter);
		count += 4;
	}
#endif

	// Read biases and weights for each linear layer
	for (i = 1; i < NUM_LAYERS; i++)
	{
		for (j = 0; j < layer_sizes[i]; j++, iter += 4)
		{
			layers[i].biases.i32[j] = read_u32_le(iter);
			count += 4;
		}
		for (j = 0; j < layer_sizes[i] * layer_sizes[i - 1]; j++, iter++)
		{
			layers[i].weights.i8[j] = (int8)*iter;
			count++;
		}
	}
	*data = iter;
	return count;
}


static u32 CalculateNetSize(u32 actual)
{
	u32 size = 0;
	u32 a, b, c, d, e, f;
	// header
	a = NET_HEADER_SIZE;
	size += a;
	// bias
	b = (HALFKX_LAYER_SIZE) * sizeof(int16);
	size += b;
	// weights
	c = (HALFKX_LAYER_SIZE) * (NUM_INPUT_FEATURES + NUM_AUXILIARY_FEATURES) * sizeof(int16);
	size += c;
#if (N_PSQT>0)
	// psqt weights
	d = (N_PSQT) * (NUM_INPUT_FEATURES) * sizeof(int32);
	size += d;
#endif

	for (int i = 1; i < NUM_LAYERS; i++)
	{
		e = layer_sizes[i] * sizeof(int32);
		size += e;
		f = layer_sizes[i] * layer_sizes[i - 1] * sizeof(int8);
		size += f;
	}
	return size;
}


void InitNnue(void)
{
	// mallocs
#ifdef _WIN32
	layers[0].weights.i16 = (int16*) _aligned_malloc(HALFKX_LAYER_SIZE*NUM_INPUT_FEATURES*sizeof(int16), 64);
	layers[0].biases.i16 = (int16*)_aligned_malloc(HALFKX_LAYER_SIZE*sizeof(int16), 64);
#else
	layers[0].weights.i16 = (int16*) aligned_alloc(64,HALFKX_LAYER_SIZE*NUM_INPUT_FEATURES*sizeof(int16));
	layers[0].biases.i16 = (int16*)aligned_alloc(64,HALFKX_LAYER_SIZE*sizeof(int16));
#endif
#if (N_PSQT>0)
#ifdef _WIN32
	layers[0].psqtweights.i32 = (int32*)_aligned_malloc(N_PSQT * (NUM_INPUT_FEATURES) * sizeof(int32), 64);
#else
	layers[0].psqtweights.i32 = (int32*)aligned_alloc(64,N_PSQT * (NUM_INPUT_FEATURES) * sizeof(int32));
#endif
#endif
	for (int i=1; i < NUM_LAYERS; i++) 
	{
#ifdef _WIN32
		layers[i].weights.i8 = (int8*)_aligned_malloc(layer_sizes[i]*layer_sizes[i-1]*sizeof(int8), 64);
		layers[i].biases.i32 = (int32*)_aligned_malloc(layer_sizes[i]*sizeof(int32), 64);
#else
		layers[i].weights.i8 = (int8*)aligned_alloc(64,layer_sizes[i]*layer_sizes[i-1]*sizeof(int8));
		layers[i].biases.i32 = (int32*)aligned_alloc(64,layer_sizes[i]*sizeof(int32));
#endif
	}
}

void NnueDestroy(void)
{
#ifdef _WIN32
	_aligned_free(layers[0].weights.i16);
	_aligned_free(layers[0].biases.i16);
#else
	free(layers[0].weights.i16);
	free(layers[0].biases.i16);
#endif
#if (N_PSQT>0)
#ifdef _WIN32
	_aligned_free(layers[0].psqtweights.i32);
#else
	free(layers[0].psqtweights.i32);
#endif
#endif
	for (int i=1; i < NUM_LAYERS; i++) 
	{
#ifdef _WIN32
		_aligned_free(layers[i].weights.i8);
		_aligned_free(layers[i].biases.i32);
#else
		free(layers[i].weights.i8);
		free(layers[i].biases.i32);
#endif
	}
}


// #include "rebel-15.txt"  

#include "rebel-15x2.txt"

bool NnueLoadEmbeddedNet()
{
	u8* iter;
	bool	ret = true;

	int size = 0;
	int i = 0;
	while (sizes[i])
	{
		size += sizes[i++];
	}
	void* mem = malloc((size_t)size);
	u8* embedded_weights = (u8*)mem;
	u8* dest = embedded_weights;

#define N_SPLITS (N_BUCKET+1)

#if (N_BUCKET == 5)
	u8* cpp[N_SPLITS] = { embedded_weights_0,
				embedded_weights_1,
				embedded_weights_2,
				embedded_weights_3,
				embedded_weights_4,
				embedded_weights_5 };
#elif (N_BUCKET == 7)
	u8* cpp[N_SPLITS] = { embedded_weights_0,
				embedded_weights_1,
				embedded_weights_2,
				embedded_weights_3,
				embedded_weights_4,
				embedded_weights_5,
				embedded_weights_6,
				embedded_weights_7 };
#elif (N_BUCKET == 11)
	u8* cpp[N_SPLITS] = { embedded_weights_0,
				embedded_weights_1,
				embedded_weights_2,
				embedded_weights_3,
				embedded_weights_4,
				embedded_weights_5,
				embedded_weights_6,
				embedded_weights_7,
				embedded_weights_8,
				embedded_weights_9,
				embedded_weights_10,
				embedded_weights_11
	};
#endif
	for (i = 0; i < N_SPLITS; i++)
	{
		u8* source = cpp[i];
		memcpy(dest, source, (size_t)sizes[i]);
		dest += (u64)sizes[i];
	}

	assert((dest - embedded_weights) == size);
	assert(NET_VERSION == *embedded_weights);

	// size = sizeof(embedded_weights);
	if ((u32)size != CalculateNetSize(size))
	{
		ret = false;
	}
	else
	{
		// Parse network header
		iter = embedded_weights;
		if (!ParseHeader(&iter))
		{
			ret = false;
		}
		else
		{
			// Parse network
			if (ParseNetwork(&iter) != size)
			{
				ret = false;
			}
		}
	}
	free(mem);
	return ret;
}



void NnueMakeMove(POSITIONDATA *pos, int move, int us, int nply)
{
	assert(pos != NULL);
	pos->eval_stack.state.valid[0] = false;
	pos->eval_stack.state.valid[1] = false;

	struct dirty_pieces* dp;
	dp = &(pos->eval_stack.dirty_pieces);

	if (!move)
	{	// null move
		dp->ndirty = 0;
		dp->piece[0] = NN_NO_PIECE;

#if 0
		if ((nply > 0) && (pos - 1)->eval_stack.state.valid[0] && (pos - 1)->eval_stack.state.valid[1])
		{
			pos->eval_stack.state = (pos - 1)->eval_stack.state;
		}
#endif
		return;
	}

#if 1

	int	from = from_sq(move);
	int	to = to_sq(move);

	assert(pos->pieces[us] & (ONE << to));

	int	piece = move_type(move) * 2 + us;
	
	dp->ndirty = 1;

	if (IsCastles(move))
	{
		dp->ndirty = 2;
		dp->piece[0] = NN_KING + us;
		dp->from[0] = from;
		dp->to[0] = to;
		dp->piece[1] = NN_ROOK + us;
		if (IsKingSideCastles(move))
		{
			dp->from[1] = to + 1;
			dp->to[1] = to - 1;
		}
		else
		{
			dp->from[1] = to - 2;
			dp->to[1] = to + 1;
		}
		return;
	}
	if (IsEpCapture(move))
	{
		dp->ndirty = 2;
		dp->piece[0] = piece;
		dp->from[0] = from;
		dp->to[0] = to;
		dp->piece[1] = NN_PAWN + (us^1);
		dp->from[1] = (us == WHITE) ? to-8 : to+8;
		dp->to[1] = NN_NO_SQUARE;
		return;
	} 

	dp->piece[0] = piece;
	dp->from[0] = from;
	dp->to[0] = to;

	if (IsCapture(move)) 
	{
		dp->ndirty = 2;
		dp->piece[1] = cap_type(move) * 2 + (us ^ 1);
		dp->from[1] = to;
		dp->to[1] = NN_NO_SQUARE;
	}
	if (IsPromo(move)) 
	{
		dp->to[0] = NN_NO_SQUARE;
		dp->piece[dp->ndirty] = promo_type(move) * 2 + us;
		dp->from[dp->ndirty] = NN_NO_SQUARE;
		dp->to[dp->ndirty] = to;
		dp->ndirty++;
	}
#endif
}

//POSITIONDATA posdata[256];

int16 NnueEvaluate(POSITIONDATA* pos, int us, int nply)
{
	assert(nply > 0);
	struct net_data data;

	NnueMakeMove(pos, pos->move, us ^ 1, nply);
	NetworkForward(pos, pos->move, &data, us, nply);
	int32 v;
#if (N_PSQT>0)
	v = (data.intermediate[0] + data.mtrl) / OUTPUT_SCALE_FACTOR;
#else
	v = data.intermediate[0] / OUTPUT_SCALE_FACTOR;
#endif
	v = clamp_int(v, -12000, 12000);
	return (int16)v;

	//return data.intermediate[0] / OUTPUT_SCALE_FACTOR;
}
//

// 
// 
// at root (nply=0)
// build our posdata structure
// build accumulator
// acc_status=good

// void Init_Nnue_Root(board_t* board, int nply)
void Init_Nnue_Root(board_t* board, int nply, int ThreadId)	// @chris
{
	assert(nply == 0);
//	POSITIONDATA* pos = &posdata[0][0];
	POSITIONDATA* pos = &posdata[ThreadId][0];				// @chris
//	posdata[0][0] = { 0 };
	posdata[ThreadId][0] = { 0 };							// @chris

	pos->pieces[WHITE] = pos->pieces[BLACK] = 0ull;		// whites, blacks
	pos->pawns = pos->nites = pos->diagonals = pos->manhattans = 0ull;
	pos->king_sq[WHITE] = pos->king_sq[BLACK] = -1;
	for (int s1 = 0; s1 < 64; s1++)
	{
		clear_square(pos, s1);
	}
	for (int r = 0; r < 8; r++)
	{
		for (int f = 0; f < 8; f++)
		{
			int fruit_sq = 0x44 + f + r*16;	// fruit format
			int fruit_pt = board->square[fruit_sq];
			if (fruit_pt == 0) continue;
			int pt12 = PIECE_TO_12(fruit_pt);
			int t1 = pt12 >> 1;
			int side = pt12 & 1;
			int s1 = r * 8 + f;				// 0-63
			u64 bb = (ONE << s1);
			pos->pieces[side] |= bb;
			switch (t1)
			{
			case (NN_PAWN >> 1):
				pos->pawns |= bb;
				piece_to_square(pos, s1, NN_PAWN/2);
				break;
			case (NN_KNIGHT >> 1):
				pos->nites |= bb;
				piece_to_square(pos, s1, NN_KNIGHT / 2);
				break;
			case (NN_BISHOP >> 1):
				pos->diagonals |= bb;
				piece_to_square(pos, s1, NN_BISHOP / 2);
				break;
			case (NN_ROOK >> 1):
				pos->manhattans |= bb;
				piece_to_square(pos, s1, NN_ROOK / 2);
				break;
			case (NN_QUEEN >> 1):
				pos->diagonals |= bb;
				pos->manhattans |= bb;
				piece_to_square(pos, s1, NN_QUEEN / 2);
				break;
			case (NN_KING >> 1):
				pos->king_sq[side] = s1;
				piece_to_square(pos, s1, NN_KING / 2);
				break;
			default:
				assert(1 == 2);
			}
		}
	}
	assert((pos->king_sq[WHITE] >= 0) && (pos->king_sq[BLACK] >= 0));
#if DEBUG_NNUE
	assert(IntegrityCheckPosition(pos));
#endif
	// full update both sides of accumulator
	for (int side = 0; side < 2; side++)
	{
		FullUpdate(pos, side);
	}
	// state is now valid
	pos->eval_stack.state.valid[0] = true;
	pos->eval_stack.state.valid[1] = true;
	return;
}
//
#include "move.h"
//#define MOVE_FROM(move)                (SQUARE_FROM_64(((move)>>6)&077))
//#define MOVE_TO(move)                  (SQUARE_FROM_64((move)&077))
//const int MoveNone = 0;  // HACK: a1a1 cannot be a legal move
//const int MoveNull = 11; // HACK: a1d2 cannot be a legal move

// file = (sq-0x44) & 7
// rank = ((sq-0x44) / 16) & 7
#define SQ64(sq) ((((sq)-0x44) & 7) | ((((sq)-0x44)>>1) & 0x38))


// just prior to move()
// can be nullmove=0
// modify posdata+1 for move
void Nnue_Housekeeping(board_t* board, int fruit_move, undo_t* undo, int nply, int ThreadId)
{
	POSITIONDATA* pos = &posdata[ThreadId][nply];
	POSITIONDATA* newpos = pos + 1;
	memcpy(newpos, pos, sizeof(POSITIONDATA));
	
	int move = 0;
	if (!((fruit_move == MoveNone) || (fruit_move == MoveNull)))
	{
		int s1 = SQ64(MOVE_FROM(fruit_move));
		int s2 = SQ64(MOVE_TO(fruit_move));

		int fruit_t1 = PIECE_TO_12(board->square[MOVE_FROM(fruit_move)]);

		int us = fruit_t1 & 1;
		int t1 = fruit_t1 >> 1;
		move |= (s1 | (s2 << TO_SHIFT) | (t1 << MOVETYPE_SHIFT));

		int t2 = 0;
		if (move_is_capture(fruit_move, board))
		{
			int s3 = s2;
			t2 = PIECE_TO_12(move_capture(fruit_move, board)) >> 1;	// accounts for ep sq
			move |= (CAPTURE_FLAG | (t2 << CAPTYPE_SHIFT));
			if (MOVE_IS_EN_PASSANT(fruit_move))
			{
				move |= EP_MOVE_FLAG;
				// capture sq = fileof(s2), rankof(s1)
				s3 = (s2 & 7) | (s1 & (7<<3));
			}
			// remove piece on s3
			clear_square(newpos, s3);
			newpos->pieces[us ^ 1] ^= (ONE << s3);
			assert(t2 != NN_KING / 2);
			switch (t2)
			{
			case NN_PAWN / 2:
				newpos->pawns ^= (ONE << s3);
				break;
			case NN_KNIGHT / 2:
				newpos->nites ^= (ONE << s3);
				break;
			case NN_BISHOP / 2:
				newpos->diagonals ^= (ONE << s3);
				break;
			case NN_ROOK / 2:
				newpos->manhattans ^= (ONE << s3);
				break;
			case NN_QUEEN / 2:
				newpos->diagonals ^= (ONE << s3);
				newpos->manhattans ^= (ONE << s3);
				break;
			default:
				assert(1 == 2);
			}
		}

		if (MOVE_IS_CASTLE(fruit_move))
		{	// move the rook
			move |= CASTLES_FLAG;
			int s4, s5;
			s4 = s1 & 0x38;	// A1 or A8
			s5 = s4 + 3;	// D1 or D8
			if ((s2 & 7) == 6)
			{
				s4 += 7;	// H1 or H8
				s5 += 2;	// F1 or F8
			}
			clear_square(newpos, s4);
			newpos->pieces[us] ^= ((ONE << s4) | (ONE << s5));
			piece_to_square(newpos, s5, NN_ROOK / 2);
			newpos->manhattans ^= ((ONE << s4) | (ONE << s5));
		}
		// remove from piece
		clear_square(newpos, s1);
		newpos->pieces[us] ^= (ONE << s1);
		switch (t1)
		{
		case NN_PAWN / 2:
			newpos->pawns ^= (ONE << s1);
			break;
		case NN_KNIGHT / 2:
			newpos->nites ^= (ONE << s1);
			break;
		case NN_BISHOP / 2:
			newpos->diagonals ^= (ONE << s1);
			break;
		case NN_ROOK / 2:
			newpos->manhattans ^= (ONE << s1);
			break;
		case NN_QUEEN / 2:
			newpos->diagonals ^= (ONE << s1);
			newpos->manhattans ^= (ONE << s1);
			break;
		case NN_KING /2:
			break;
		default:
			assert(1 == 2);
		}

		if (MOVE_IS_PROMOTE(fruit_move))
		{	// make promo piece
			t1 = PIECE_TO_12(move_promote(fruit_move)) >> 1;
			move |= (t1 << PROMO_SHIFT);
		}
		// add moving piece
		newpos->pieces[us] ^= (ONE << s2);
		piece_to_square(newpos, s2, t1);
		switch (t1)
		{
		case NN_PAWN / 2:
			newpos->pawns ^= (ONE << s2);
			break;
		case NN_KNIGHT / 2:
			newpos->nites ^= (ONE << s2);
			break;
		case NN_BISHOP / 2:
			newpos->diagonals ^= (ONE << s2);
			break;
		case NN_ROOK / 2:
			newpos->manhattans ^= (ONE << s2);
			break;
		case NN_QUEEN / 2:
			newpos->diagonals ^= (ONE << s2);
			newpos->manhattans ^= (ONE << s2);
			break;
		case NN_KING / 2:
			newpos->king_sq[us] = s2;
			break;
		default:
			assert(1 == 2);
		}
	}
	newpos->move = move;
	newpos->eval_stack.state.valid[0] = false;
	newpos->eval_stack.state.valid[1] = false;
#if DEBUG_NNUE
	assert(IntegrityCheckPosition(newpos));
#endif
	return;
}

