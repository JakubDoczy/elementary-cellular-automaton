// ==----------------------- automaton.c -----------------------==
// 1D cellular automaton with rule 110
// en.wikipedia.ord/wiki/Rule_110
//
// This implementation is optimized for size -> bit packing
// It is also (somewhat) optimized for speed, but the fastest way
// would be to precompute a table/map with all combinations of 3
// consecutive blocks.
//
// This implementation should work on both -
// big-endian and little-endian architectures
//
// Author: Jakub Doczy
// ==-----------------------------------------------------------==

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>


// Total number of cells
#define STATE_SIZE (3*8)
// Type of a block. The size of chosen type must be <= size_t.
// Cells are packed into blocks and saved in a reverse order:
// position of the first cell in a block -> BLOCK_SIZE - 1
// position of the last cell in a block  -> 0
typedef uint8_t block_t;
// Nuber of cells in one block (== number of bits)
#define BLOCK_SIZE 8 * sizeof(block_t)
// Number of blocks
#define ARRAY_SIZE STATE_SIZE / BLOCK_SIZE + (STATE_SIZE % BLOCK_SIZE != 0)


// Specifies one cell (bit) in a block
typedef struct {
  block_t *pblock;
  uint8_t position;
} bit_view_t;


/*
// Set one bit to a boolean value.
// This is relatively slow.
static void set_bit(const bit_view_t *pview, bool new_state) {
assert(pview != NULL);
if (new_state) {
*(pview->pblock) |= 1 << pview->position;
} else {
*(pview->pblock) &= ~(1 << pview->position);
}
}*/

// Flips one bit.
// This is much faster than set_bit().
static void bit_flip(const bit_view_t *pview) {
  assert(pview != NULL);
  *(pview->pblock) ^= 1 << pview->position;
}

// If flip == true, flips the bit.
// this is still faster than set_bit()
static void conditional_bit_flip(const bit_view_t *pview, bool flip) {
  assert(pview != NULL);
  // Commented code might be good for some processors (or GPU), since it 
  // avoids condition. But it is slower on my PC.
  //*(pview->pblock) ^=  flip << pview->position;
    
  if (flip)
    bit_flip(pview);
}

static bool get_bit(const bit_view_t *pview) {
  assert(pview != NULL);
  // (*(pview->pblock) & (1 << pview->position)) != 0; // slower
  return (*(pview->pblock) >> pview->position) & 1;
}

static bool evaluate(const bit_view_t *pleft_view,
		     const bit_view_t *pcenter_view,
		     const bit_view_t *pright_view,
		     const bool *pflip_rules) {
  assert(pleft_view != NULL);
  assert(pcenter_view != NULL);
  assert(pright_view != NULL);
  assert(pflip_rules != NULL);

  size_t value = (get_bit(pleft_view) << 2) + (get_bit(pcenter_view) << 1) + get_bit(pright_view);
  return pflip_rules[value];
}

// Computes moves celular automation to the new state.
void automaton_step(block_t *state_array, const bool *pflip_rules) {
  assert(state_array != NULL);
  assert(pflip_rules != NULL);
    
  bit_view_t left_view = {state_array, BLOCK_SIZE - 1};
  bit_view_t center_view = {state_array, BLOCK_SIZE - 2};
  bit_view_t right_view = {state_array, BLOCK_SIZE - 3};

  // the first cell will stay the same
  bool prev_result = get_bit(&left_view);
  bool curr_result;
  for (size_t i = 3; i < STATE_SIZE + 1; i++) {
    // compute the middle cell
    curr_result = evaluate(&left_view, &center_view, &right_view, pflip_rules);
    // set the left cell
    conditional_bit_flip(&left_view, prev_result);   
    // move right
    prev_result = curr_result;
    left_view = center_view;
    center_view = right_view;
    right_view.pblock = state_array + i / BLOCK_SIZE;
    right_view.position = BLOCK_SIZE - 1 - (i % BLOCK_SIZE);
  }
  // set the STATE_SIZE - 1 cell
  // (the last cell stays the same)
  conditional_bit_flip(&left_view, curr_result);
}

static const bool RULE110[] = {
  false, // <- 000
  true,  // <- 001
  false, // <- 010
  false, // <- 011
  false, // <- 100
  true,  // <- 101
  false, // <- 110
  true   // <- 111
};

static void print_block(block_t block, uint8_t bits) {
    assert(bits > 0);
    block_t mask = 1 << (BLOCK_SIZE - 1);
    block_t end_mask = 1 << (BLOCK_SIZE - bits);
    
    while (mask > end_mask) {
        printf("%c ", (block & mask) ? '#' : ' ');
        mask = mask >> 1;
    }
    printf("%c", (block & end_mask) ? '#' : ' ');
}

// prints automaton state
static void pretty_print(const block_t *state_array) {
    assert(state_array != NULL);

    for (size_t i = 0; i < ARRAY_SIZE - 1; i++) {
        print_as_characters(state_array[i], BLOCK_SIZE);
        printf(" ");
    }
    int remaining_bits = (STATE_SIZE % BLOCK_SIZE) ? (STATE_SIZE % BLOCK_SIZE) : BLOCK_SIZE;
    print_as_characters(state_array[ARRAY_SIZE - 1], remaining_bits);
    printf("\n");
}

int main() {
  block_t automaton_state[ARRAY_SIZE] = {0, 1, 2};

  // print initial state
  pretty_print(automaton_state);

  // perform first 100 steps
  for (int i = 0; i < 100; i++) {
    automaton_step(automaton_state, RULE110);
    pretty_print(automaton_state);
  }
}

