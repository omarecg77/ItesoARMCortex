//Original source code from : https://github.com/XKCP/XKCP

/*
The eXtended Keccak Code Package (XKCP)
https://github.com/XKCP/XKCP

Implementation by Gilles Van Assche and Ronny Van Keer, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to the Keccak Team website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "xil_printf.h"

#define maxNrRounds 24
#define nrLanes 25
typedef struct {
    uint32_t A[50];
} KeccakP1600_plain32_state;

typedef KeccakP1600_plain32_state KeccakP1600_state;

void KeccakP1600_StaticInitialize(void);
void KeccakP1600_Initialize(KeccakP1600_plain32_state *state);
void sha3_function32(unsigned char *message, unsigned int messageLength, unsigned char delimitedSuffix, unsigned int rate, unsigned int capacity, unsigned int outputLengthInBits, unsigned char* output);
int padding(unsigned char *Msg, unsigned int messageLength, uint32_t *sha3_data);