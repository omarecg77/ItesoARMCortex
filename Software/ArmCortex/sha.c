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


#include "sha.h"

void displayBytes(const char *text, const unsigned char *bytes, unsigned int size)
{
    unsigned int i;
    xil_printf("%s:\r\n", text);
    for(i=0; i<size; i++)
		{
			xil_printf("%02X ", bytes[i]);
		}
    xil_printf("\r\n");
}

void displayBits(const char *text, const unsigned char *data, unsigned int size, int MSBfirst)
{
    unsigned int i, iByte, iBit;
    char    debugStr[100];
		xil_printf( "%s:\r\n", text);
		xil_printf( debugStr);
    for(i=0; i<size; i++)
		{
			iByte = i/8;
      iBit = i%8;
      if (MSBfirst)
			{
				xil_printf( debugStr, "%d ", ((data[iByte] << iBit) & 0x80) != 0);
			}
      else
			{
        xil_printf(  "%d ", ((data[iByte] >> iBit) & 0x01) != 0);
			}
		}
		xil_printf( "\r\n");

}

void displayStateAsBytes(const char *text, const unsigned char *state, unsigned int width)
{
    displayBytes(text, state, width/8);
}

void displayStateAs32bitWords(const char *text, const unsigned int *state)
{
    unsigned int i;
		xil_printf( "%s:\r\n", text);
    for(i=0; i<25; i++)
		{
			xil_printf(  "%08X:%08X", (unsigned int)state[2*i+0], (unsigned int)state[2*i+1]);
      if ((i%5) == 4)
				xil_printf( "\r\n");
      else
        xil_printf( " ");
    }
}

void displayStateAs64bitLanes( const char *text, void *statePointer)
{
    unsigned int i;
    unsigned long long int *state = statePointer;
    xil_printf( "%s:\r\n", text);
    for(i=0; i<25; i++)
		{
			xil_printf( "%08X", (unsigned int)(state[i] >> 32));
      xil_printf( "%08X", (unsigned int)(state[i] & 0xFFFFFFFFULL));
      if ((i%5) == 4)
				xil_printf(  "\r\n");
      else
        xil_printf( " ");
     }
}

void displayStateAs32bitLanes( const char *text, void *statePointer)
{
    unsigned int i;
    unsigned int *state = statePointer;
		xil_printf(  "%s:\r\n", text);
    for(i=0; i<25; i++)
		{
			xil_printf( "%08X", state[i]);
      if ((i%5) == 4)
				xil_printf( "\r\n");
      else
        xil_printf( " ");
		}
}

void displayStateAs16bitLanes( const char *text, void *statePointer)
{
    unsigned int i;
    unsigned short *state = statePointer;
		xil_printf( "%s:\r\n", text);
    for(i=0; i<25; i++)
		{
			xil_printf("%04X ", state[i]);
      if ((i%5) == 4)
			{
				xil_printf("\r\n");
			}
    }
}

void displayStateAs8bitLanes(const char *text, void *statePointer)
{
    unsigned int i;
    unsigned char *state = statePointer; 
		xil_printf( "%s:\r\n", text);
    for(i=0; i<25; i++)
		{
			xil_printf("%02X ", state[i]);
      if ((i%5) == 4)
			{
       xil_printf("\r\n");
			}
     }
}

void displayStateAsLanes( const char *text, void *statePointer, unsigned int width)
{
    if (width == 1600)
        displayStateAs64bitLanes( text, statePointer);
    if (width == 800)
        displayStateAs32bitLanes(text, statePointer);
    if (width == 400)
        displayStateAs16bitLanes(text, statePointer);
    if (width == 200)
        displayStateAs8bitLanes(text, statePointer);
}

void displayRoundNumber(unsigned int i)
{
      xil_printf( "\r\n");
	    xil_printf( "--- Round %d ---", i);
      xil_printf( "\r\n");
}


void displayText( const char *text)
{
				xil_printf( "\r\n");
        xil_printf("%s", text);
        xil_printf("\r\n");
}

/******/


static uint32_t KeccakRoundConstants[maxNrRounds][2];
static unsigned int KeccakRhoOffsets[nrLanes];




void toBitInterleaving(uint32_t low, uint32_t high, uint32_t *even, uint32_t *odd)
{
    unsigned int i;

    *even = 0;
    *odd = 0;
    for(i=0; i<64; i++)
		{
        unsigned int inBit;
        if (i < 32)
				{
            inBit = (low >> i) & 1;
				}
        else
				{
            inBit = (high >> (i-32)) & 1;
				}
        if ((i % 2) == 0)
				{
            *even |= inBit << (i/2);
				}
        else
				{
            *odd |= inBit << ((i-1)/2);
				}
    }
}

void fromBitInterleaving(uint32_t even, uint32_t odd, uint32_t *low, uint32_t *high)
{
    unsigned int i;

    *low = 0;
    *high = 0;
    for(i=0; i<64; i++) {
        unsigned int inBit;
        if ((i % 2) == 0)
            inBit = (even >> (i/2)) & 1;
        else
            inBit = (odd >> ((i-1)/2)) & 1;
        if (i < 32)
            *low |= inBit << i;
        else
            *high |= inBit << (i-32);
    }
}

static int LFSR86540(uint8_t *LFSR)
{
    int result = ((*LFSR) & 0x01) != 0;
    if (((*LFSR) & 0x80) != 0)
        /* Primitive polynomial over GF(2): x^8+x^6+x^5+x^4+1 */
        (*LFSR) = ((*LFSR) << 1) ^ 0x71;
    else
        (*LFSR) <<= 1;
    return result;
}

void KeccakP1600_InitializeRoundConstants(void)
{
    uint8_t LFSRstate = 0x01;
    unsigned int i, j, bitPosition;
    uint32_t low, high;

    for(i=0; i<maxNrRounds; i++) {
        low = high = 0;
        for(j=0; j<7; j++) {
            bitPosition = (1<<j)-1; /* 2^j-1 */
            if (LFSR86540(&LFSRstate)) {
                if (bitPosition < 32)
                    low ^= (uint32_t)1 << bitPosition;
                else
                    high ^= (uint32_t)1 << (bitPosition-32);
            }
        }
        toBitInterleaving(low, high, &(KeccakRoundConstants[i][0]), &(KeccakRoundConstants[i][1]));
    }
}

void KeccakP1600_InitializeRhoOffsets(void)
{
    unsigned int x, y, t, newX, newY;

    KeccakRhoOffsets[0] = 0;
    x = 1;
    y = 0;
    for(t=0; t<24; t++) {
        KeccakRhoOffsets[5*y+x] = ((t+1)*(t+2)/2) % 64;
        newX = (0*x+1*y) % 5;
        newY = (2*x+3*y) % 5;
        x = newX;
        y = newY;
    }
}

void KeccakP1600_StaticInitialize(void)
{
    KeccakP1600_InitializeRoundConstants();
    KeccakP1600_InitializeRhoOffsets();
}

void KeccakP1600_Initialize(KeccakP1600_plain32_state *state)
{
    memset(state, 0, 1600/8);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_AddBytes(KeccakP1600_plain32_state *state, const unsigned char *data, unsigned int offset, unsigned int length);

void KeccakP1600_AddByte(KeccakP1600_plain32_state *state, unsigned char byte, unsigned int offset)
{
    unsigned char data[1];
    data[0] = byte;
    KeccakP1600_AddBytes(state, data, offset, 1);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_AddBytesInLane(KeccakP1600_plain32_state *state, unsigned int lanePosition, const unsigned char *data, unsigned int offset, unsigned int length)
{
    if ((lanePosition < 25) && (offset < 8) && (offset+length <= 8)) {
        uint8_t laneAsBytes[8];
        uint32_t low, high;
        uint32_t lane[2];

        memset(laneAsBytes, 0, 8);
        memcpy(laneAsBytes+offset, data, length);
        low = laneAsBytes[0]
            | ((uint32_t)(laneAsBytes[1]) << 8)
            | ((uint32_t)(laneAsBytes[2]) << 16)
            | ((uint32_t)(laneAsBytes[3]) << 24);
        high = laneAsBytes[4]
            | ((uint32_t)(laneAsBytes[5]) << 8)
            | ((uint32_t)(laneAsBytes[6]) << 16)
            | ((uint32_t)(laneAsBytes[7]) << 24);
        toBitInterleaving(low, high, lane, lane+1);
        state->A[lanePosition*2+0] ^= lane[0];
        state->A[lanePosition*2+1] ^= lane[1];
    }
}

void KeccakP1600_AddBytes(KeccakP1600_plain32_state *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    unsigned int lanePosition = offset/8;
    unsigned int offsetInLane = offset%8;
    while(length > 0) {
        unsigned int bytesInLane = 8 - offsetInLane;
        if (bytesInLane > length)
            bytesInLane = length;
				//displayBytes("data", data, bytesInLane);
        KeccakP1600_AddBytesInLane(state, lanePosition, data, offsetInLane, bytesInLane);
        length -= bytesInLane;
        lanePosition++;
        offsetInLane = 0;
        data += bytesInLane;
    }
}

/* ---------------------------------------------------------------- */

void KeccakP1600_ExtractBytesInLane(const KeccakP1600_plain32_state *state, unsigned int lanePosition, unsigned char *data, unsigned int offset, unsigned int length);

void KeccakP1600_OverwriteBytesInLane(KeccakP1600_plain32_state *state, unsigned int lanePosition, const unsigned char *data, unsigned int offset, unsigned int length)
{
    if ((lanePosition < 25) && (offset < 8) && (offset+length <= 8)) {
        uint8_t laneAsBytes[8];
        uint32_t low, high;
        uint32_t lane[2];

        KeccakP1600_ExtractBytesInLane(state, lanePosition, laneAsBytes, 0, 8);
        memcpy(laneAsBytes+offset, data, length);
        low = laneAsBytes[0]
            | ((uint32_t)(laneAsBytes[1]) << 8)
            | ((uint32_t)(laneAsBytes[2]) << 16)
            | ((uint32_t)(laneAsBytes[3]) << 24);
        high = laneAsBytes[4]
            | ((uint32_t)(laneAsBytes[5]) << 8)
            | ((uint32_t)(laneAsBytes[6]) << 16)
            | ((uint32_t)(laneAsBytes[7]) << 24);
        toBitInterleaving(low, high, lane, lane+1);
        state->A[lanePosition*2+0] = lane[0];
        state->A[lanePosition*2+1] = lane[1];
    }
}

void KeccakP1600_OverwriteBytes(KeccakP1600_plain32_state *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    unsigned int lanePosition = offset/8;
    unsigned int offsetInLane = offset%8;
    while(length > 0) {
        unsigned int bytesInLane = 8 - offsetInLane;
        if (bytesInLane > length)
            bytesInLane = length;
        KeccakP1600_OverwriteBytesInLane(state, lanePosition, data, offsetInLane, bytesInLane);
        length -= bytesInLane;
        lanePosition++;
        offsetInLane = 0;
        data += bytesInLane;
    }
}

/* ---------------------------------------------------------------- */

void KeccakP1600_OverwriteWithZeroes(KeccakP1600_plain32_state *state, unsigned int byteCount)
{
    uint8_t laneAsBytes[8];
    unsigned int lanePosition = 0;
    memset(laneAsBytes, 0, 8);
    while(byteCount > 0) {
        if (byteCount < 8) {
            KeccakP1600_OverwriteBytesInLane(state, lanePosition, laneAsBytes, 0, byteCount);
            byteCount = 0;
        }
        else {
            state->A[lanePosition*2+0] = 0;
            state->A[lanePosition*2+1] = 0;
            byteCount -= 8;
            lanePosition++;
        }
    }
}

/* ---------------------------------------------------------------- */

void KeccakP1600_PermutationOnWords(uint32_t *state, unsigned int nrRounds);
static void theta(uint32_t *A);
static void rho(uint32_t *A);
static void pi(uint32_t *A);
static void chi(uint32_t *A);
static void iota(uint32_t *A, unsigned int indexRound);
void KeccakP1600_ExtractBytes(const KeccakP1600_plain32_state *state, unsigned char *data, unsigned int offset, unsigned int length);

void KeccakP1600_Permute_Nrounds(KeccakP1600_plain32_state *state, unsigned int nrounds)
{
    {
        uint8_t stateAsBytes[1600/8];
        KeccakP1600_ExtractBytes(state, stateAsBytes, 0, 1600/8);
        //displayStateAsBytes("Input of permutation", stateAsBytes, 1600);
    }
    KeccakP1600_PermutationOnWords(state->A, nrounds);
    {
        uint8_t stateAsBytes[1600/8];
        KeccakP1600_ExtractBytes(state, stateAsBytes, 0, 1600/8);
        //displayStateAsBytes("State after permutation", stateAsBytes, 1600);
    }
}


void KeccakP1600_Permute_12rounds(KeccakP1600_plain32_state *state)
{
    {
        uint8_t stateAsBytes[1600/8];
        KeccakP1600_ExtractBytes(state, stateAsBytes, 0, 1600/8);
        //displayStateAsBytes("Input of permutation", stateAsBytes, 1600);
    }
    KeccakP1600_PermutationOnWords(state->A, 12);
    {
        uint8_t stateAsBytes[1600/8];
        KeccakP1600_ExtractBytes(state, stateAsBytes, 0, 1600/8);
        //displayStateAsBytes("State after permutation", stateAsBytes, 1600);
    }
}

void KeccakP1600_Permute_24rounds(KeccakP1600_plain32_state *state)
{
    KeccakP1600_PermutationOnWords(state->A, 24);
}

void KeccakP1600_PermutationOnWords(uint32_t *state, unsigned int nrRounds)
{
    unsigned int i;
    //displayStateAs32bitWords( "state", state);

    for(i=(maxNrRounds-nrRounds); i<maxNrRounds; i++) {
        //displayRoundNumber(i);

        theta(state);
        //displayStateAs32bitWords("After theta", state);

        rho(state);
        //displayStateAs32bitWords("After rho", state);

        pi(state);
        //displayStateAs32bitWords("After pi", state);

        chi(state);
        //displayStateAs32bitWords("After chi", state);

        iota(state, i);
        //displayStateAs32bitWords("After iota", state);
    }
		//displayStateAs32bitWords( "state", state);
}

#define index(x, y,z) ((((x)%5)+5*((y)%5))*2 + z)
#define ROL32(a, offset) ((offset != 0) ? ((((uint32_t)a) << offset) ^ (((uint32_t)a) >> (32-offset))) : a)

void ROL64(uint32_t inEven, uint32_t inOdd, uint32_t *outEven, uint32_t *outOdd, unsigned int offset)
{
    if ((offset % 2) == 0) {
        *outEven = ROL32(inEven, offset/2);
        *outOdd = ROL32(inOdd, offset/2);
    }
    else {
        *outEven = ROL32(inOdd, (offset+1)/2);
        *outOdd = ROL32(inEven, (offset-1)/2);
    }
}

static void theta(uint32_t *A)
{
    unsigned int x, y, z;
    uint32_t C[5][2], D[5][2];

    for(x=0; x<5; x++) {
        for(z=0; z<2; z++) {
            C[x][z] = 0;
            for(y=0; y<5; y++)
                C[x][z] ^= A[index(x, y, z)];
        }
    }
    for(x=0; x<5; x++) {
        ROL64(C[(x+1)%5][0], C[(x+1)%5][1], &(D[x][0]), &(D[x][1]), 1);
        for(z=0; z<2; z++)
            D[x][z] ^= C[(x+4)%5][z];
    }
    for(x=0; x<5; x++)
        for(y=0; y<5; y++)
            for(z=0; z<2; z++)
                A[index(x, y, z)] ^= D[x][z];
}

static void rho(uint32_t *A)
{
    unsigned int x, y;

    for(x=0; x<5; x++) for(y=0; y<5; y++)
        ROL64(A[index(x, y, 0)], A[index(x, y, 1)], &(A[index(x, y, 0)]), &(A[index(x, y, 1)]), KeccakRhoOffsets[5*y+x]);
}

static void pi(uint32_t *A)
{
    unsigned int x, y, z;
    uint32_t tempA[50];

    for(x=0; x<5; x++) for(y=0; y<5; y++) for(z=0; z<2; z++)
        tempA[index(x, y, z)] = A[index(x, y, z)];
    for(x=0; x<5; x++) for(y=0; y<5; y++) for(z=0; z<2; z++)
        A[index(0*x+1*y, 2*x+3*y, z)] = tempA[index(x, y, z)];
}

static void chi(uint32_t *A)
{
    unsigned int x, y, z;
    uint32_t C[5][2];

    for(y=0; y<5; y++) {
        for(x=0; x<5; x++)
            for(z=0; z<2; z++)
                C[x][z] = A[index(x, y, z)] ^ ((~A[index(x+1, y, z)]) & A[index(x+2, y, z)]);
        for(x=0; x<5; x++)
            for(z=0; z<2; z++)
                A[index(x, y, z)] = C[x][z];
    }
}

static void iota(uint32_t *A, unsigned int indexRound)
{
    A[index(0, 0, 0)] ^= KeccakRoundConstants[indexRound][0];
    A[index(0, 0, 1)] ^= KeccakRoundConstants[indexRound][1];
}

/* ---------------------------------------------------------------- */

void KeccakP1600_ExtractBytesInLane(const KeccakP1600_plain32_state *state, unsigned int lanePosition, unsigned char *data, unsigned int offset, unsigned int length)
{
    if ((lanePosition < 25) && (offset < 8) && (offset+length <= 8)) {
        uint32_t lane[2];
        uint8_t laneAsBytes[8];
        fromBitInterleaving(state->A[lanePosition*2], state->A[lanePosition*2+1], lane, lane+1);
        laneAsBytes[0] = lane[0] & 0xFF;
        laneAsBytes[1] = (lane[0] >> 8) & 0xFF;
        laneAsBytes[2] = (lane[0] >> 16) & 0xFF;
        laneAsBytes[3] = (lane[0] >> 24) & 0xFF;
        laneAsBytes[4] = lane[1] & 0xFF;
        laneAsBytes[5] = (lane[1] >> 8) & 0xFF;
        laneAsBytes[6] = (lane[1] >> 16) & 0xFF;
        laneAsBytes[7] = (lane[1] >> 24) & 0xFF;
        memcpy(data, laneAsBytes+offset, length);
    }
}

void KeccakP1600_ExtractBytes(const KeccakP1600_plain32_state *state, unsigned char *data, unsigned int offset, unsigned int length)
{
	
    unsigned int lanePosition = offset/8;
    unsigned int offsetInLane = offset%8;
    while(length > 0) {
        unsigned int bytesInLane = 8 - offsetInLane;
        if (bytesInLane > length)
            bytesInLane = length;
        KeccakP1600_ExtractBytesInLane(state, lanePosition, data, offsetInLane, bytesInLane);
        length -= bytesInLane;
        lanePosition++;
        offsetInLane = 0;
        data += bytesInLane;
    }
}

/* ---------------------------------------------------------------- */

void KeccakP1600_ExtractAndAddBytesInLane(const KeccakP1600_plain32_state *state, unsigned int lanePosition, const unsigned char *input, unsigned char *output, unsigned int offset, unsigned int length)
{
    if ((lanePosition < 25) && (offset < 8) && (offset+length <= 8)) {
        uint8_t laneAsBytes[8];
        unsigned int i;

        KeccakP1600_ExtractBytesInLane(state, lanePosition, laneAsBytes, offset, length);
        for(i=0; i<length; i++)
            output[i] = input[i] ^ laneAsBytes[i];
    }
}

void KeccakP1600_ExtractAndAddBytes(const KeccakP1600_plain32_state *state, const unsigned char *input, unsigned char *output, unsigned int offset, unsigned int length)
{
    unsigned int lanePosition = offset/8;
    unsigned int offsetInLane = offset%8;
    while(length > 0) {
        unsigned int bytesInLane = 8 - offsetInLane;
        if (bytesInLane > length)
            bytesInLane = length;
        KeccakP1600_ExtractAndAddBytesInLane(state, lanePosition, input, output, offsetInLane, bytesInLane);
        length -= bytesInLane;
        lanePosition++;
        offsetInLane = 0;
        input += bytesInLane;
        output += bytesInLane;
    }
}

/* ---------------------------------------------------------------- */

void KeccakP1600_DisplayRoundConstants()
{
    unsigned int i;
		//char debugStr[30];

    for(i=0; i<maxNrRounds; i++)
		{
					    
			//xil_printf( "\r\n");
            //xil_printf("RC[%02i][0][0] = ", i);
			//xil_printf("%08X:%08X", (unsigned int)(KeccakRoundConstants[i][0]), (unsigned int)(KeccakRoundConstants[i][1]));
			//xil_printf(debugStr);
            //xil_printf("\r\n");
    }

}

void KeccakP1600_DisplayRhoOffsets()
{
    unsigned int x, y;
		//char debugStr[30];
    for(y=0; y<5; y++) for(x=0; x<5; x++)
		{
        //xil_printf("RhoOffset[%i][%i] = ", x, y);
				//xil_printf(debugStr);
        //xil_printf("%2i", KeccakRhoOffsets[5*y+x]);
				//xil_printf(debugStr);
        //xil_printf("\r\n");
    }
    //xil_printf("\r\n");
}

unsigned int appendSuffixToMessage(unsigned char *out, unsigned char *in, unsigned int inputLengthInBits, unsigned char delimitedSuffix)
{
    memcpy(out, in, (inputLengthInBits+7)/8);
    if (delimitedSuffix == 0x00)
        abort();
    while(delimitedSuffix != 0x01) {
        unsigned char bit = delimitedSuffix & 0x01;
        out[inputLengthInBits/8] |= (bit << (inputLengthInBits%8));
        inputLengthInBits++;
        delimitedSuffix >>= 1;
    }
    return inputLengthInBits;
}

typedef  KeccakP1600_plain32_state state_t[50];



typedef struct SpongeInstanceStruct32 { \
    KeccakP1600_state state[1]; \
    unsigned int rate; \
    unsigned int byteIOIndex; \
    int squeezing; \
} SpongeInstance32;

int SpongeAbsorb32(SpongeInstance32 *instance, const unsigned char *data, size_t dataByteLen)
{
    size_t i, j;
    unsigned int partialBlock;
    const unsigned char *curData;
		
    unsigned int rateInBytes = instance->rate/8;
    if (instance->squeezing)
        return 1; /* Too late for additional input */

    i = 0;
    curData = data;
    while(i < dataByteLen) {
        if ((instance->byteIOIndex == 0) && (dataByteLen-i >= rateInBytes)) {
            for(j=dataByteLen-i; j>=rateInBytes; j-=rateInBytes) {
								//displayBytes("Addbytes_1", curData, rateInBytes);
                KeccakP1600_AddBytes(instance->state, curData, 0, rateInBytes);
                KeccakP1600_Permute_24rounds(instance->state);
                curData+=rateInBytes;
            }
            i = dataByteLen - j;
        }
        else {
            /* normal lane: using the message queue */
            if (dataByteLen-i > rateInBytes-instance->byteIOIndex)
                partialBlock = rateInBytes-instance->byteIOIndex;
            else
                partialBlock = (unsigned int)(dataByteLen - i);

            i += partialBlock;
						//displayBytes("Addbytes_2", curData, partialBlock);
            KeccakP1600_AddBytes(instance->state, curData, instance->byteIOIndex, partialBlock);
            curData += partialBlock;
            instance->byteIOIndex += partialBlock;
            if (instance->byteIOIndex == rateInBytes) {
                KeccakP1600_Permute_24rounds(instance->state);
                instance->byteIOIndex = 0;
            }
        }
    }
    return 0;
}

int SpongeAbsorbLastFewBits32(SpongeInstance32 *instance, unsigned char delimitedData)
{
    unsigned int rateInBytes = instance->rate/8;
	  unsigned char delimitedData1[1];
    if (delimitedData == 0)
        return 1;
    if (instance->squeezing)
        return 1; /* Too late for additional input */
    {
        delimitedData1[0] = delimitedData;
    }
    	KeccakP1600_AddByte(instance->state, delimitedData, instance->byteIOIndex);
    	if ((delimitedData >= 0x80) && (instance->byteIOIndex == (rateInBytes-1)))
        KeccakP1600_Permute_24rounds(instance->state);
    KeccakP1600_AddByte(instance->state, 0x80, rateInBytes-1);

    {
        unsigned char block[1600/8];
        memset(block, 0, 1600/8);
        block[rateInBytes-1] = 0x80;
    }

    KeccakP1600_Permute_24rounds(instance->state);
    instance->byteIOIndex = 0;
    instance->squeezing = 1;
    return 0;
}


int SpongeSqueeze32(SpongeInstance32 *instance, unsigned char *data, size_t dataByteLen)
{
	
    size_t i, j;
    unsigned int partialBlock;
    unsigned int rateInBytes = instance->rate/8;
    unsigned char *curData;
    if (!instance->squeezing)
        SpongeAbsorbLastFewBits32(instance, 0x01);

    i = 0;
    curData = data;
    while(i < dataByteLen) {
        if ((instance->byteIOIndex == rateInBytes) && (dataByteLen-i >= rateInBytes)) {
            for(j=dataByteLen-i; j>=rateInBytes; j-=rateInBytes) {
                KeccakP1600_Permute_24rounds(instance->state);
                KeccakP1600_ExtractBytes(instance->state, curData, 0, rateInBytes);
                curData+=rateInBytes;
            }
            i = dataByteLen - j;
        }
        else {
            if (instance->byteIOIndex == rateInBytes) {
                KeccakP1600_Permute_24rounds(instance->state);
                instance->byteIOIndex = 0;
            }
            if (dataByteLen-i > rateInBytes-instance->byteIOIndex)
                partialBlock = rateInBytes-instance->byteIOIndex;
            else
                partialBlock = (unsigned int)(dataByteLen - i);
            i += partialBlock;

            KeccakP1600_ExtractBytes(instance->state, curData, instance->byteIOIndex, partialBlock);
            curData += partialBlock;
            instance->byteIOIndex += partialBlock;
        }
    }
    return 0;
}


void sha3_function32(unsigned char *message, unsigned int messageLength, unsigned char delimitedSuffix, unsigned int rate, unsigned int capacity, unsigned int outputLengthInBits, unsigned char *output)
{
    SpongeInstance32 sponge;
    unsigned char *messageWithSuffix;
    unsigned int messageLengthWithSuffix;
	  messageWithSuffix = malloc((messageLength+15)/8);
    messageLengthWithSuffix = appendSuffixToMessage(messageWithSuffix, message, messageLength, delimitedSuffix);
		if (rate + capacity != 1600)
			return;
		if ((rate <= 0) || (rate > 1600) || (rate%8) != 0)
			return;
		KeccakP1600_StaticInitialize();
		KeccakP1600_Initialize(sponge.state);
		sponge.rate = rate;
		sponge.byteIOIndex = 0;
		sponge.squeezing = 0;
		SpongeAbsorb32(&sponge, messageWithSuffix, messageLengthWithSuffix/8);
   if ((messageLengthWithSuffix % 8) != 0)
   {
        SpongeAbsorbLastFewBits32(&sponge, messageWithSuffix[messageLengthWithSuffix/8] | (1 << (messageLengthWithSuffix % 8)));
   }
	 if (outputLengthInBits <= 512)
   {
        SpongeSqueeze32(&sponge, output, (outputLengthInBits+7)/8);
   }
	 //displayBytes("output", output, 64);
}

typedef struct simplesponge { \
    uint8_t state[72]; \
    unsigned int rate; \
    unsigned int byteIOIndex; \
    int squeezing; \
} simplesponge;

void AddBytes(simplesponge *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    unsigned int i;
    for(i=0; i<length; i++)
        state->state[offset+i] ^= data[i];
}

void AddByte(simplesponge *state, unsigned char byte, unsigned int offset)
{
    state->state[offset] ^= byte;
}

int padding(unsigned char *Msg, unsigned int messageLength, uint32_t *sha3_data)
{
	  
    simplesponge sponge;
    unsigned char *message = Msg;
	
    unsigned char *messageWithSuffix;
    unsigned int messageLengthWithSuffix;
    unsigned char delimitedSuffix=0x6;
	  unsigned int partialBlock;
    const unsigned char *curData;
	  int dataByteLen;
	  size_t i;
	  unsigned int bytecount;
    int index;
	  unsigned char delimitedData;
    unsigned int rateInBytes;
	  messageWithSuffix = malloc((messageLength+15)/8);
    messageLengthWithSuffix = appendSuffixToMessage(messageWithSuffix, message, messageLength, delimitedSuffix);
    memset(sponge.state, 0, sizeof(sponge.state));
    sponge.squeezing = 0;
    sponge.byteIOIndex = 0;
    sponge.rate = 576;
		rateInBytes = sponge.rate/8;
    i = 0;
    curData = messageWithSuffix;
    dataByteLen = messageLengthWithSuffix/8;
		while(i < dataByteLen)
    {
		    if (dataByteLen-i > rateInBytes-sponge.byteIOIndex)
				{
		        partialBlock = rateInBytes-sponge.byteIOIndex;
				}
        else
				{
		        partialBlock = (unsigned int)(dataByteLen - i);
				}
        i = i +  partialBlock;
		    AddBytes(&sponge, curData, sponge.byteIOIndex, partialBlock);
        curData += partialBlock;
        sponge.byteIOIndex += partialBlock;
		}
    if ((messageLengthWithSuffix % 8) != 0)
    {
        delimitedData = messageWithSuffix[messageLengthWithSuffix/8] | (1 << (messageLengthWithSuffix % 8));
        if (delimitedData == 0)
            return 1;
		    AddByte(&sponge, delimitedData, sponge.byteIOIndex);
		    AddByte(&sponge, 0x80, rateInBytes-1);
        sponge.byteIOIndex = 0;
        sponge.squeezing = 1;
				
        //for (i=0; i < 72; i++)
				//{
        //    xil_printf("%02x", sponge.state[i]);
				//}
        //xil_printf("\n\r");
        bytecount = 18 * 4;
        index = 0;
		    for (i = 0; i < bytecount - 3; i += 4)
        {
            sha3_data[index] = (sponge.state[i + 3] << 24) | (sponge.state[i + 2] << 16) |
                                   (sponge.state[i + 1] << 8) | sponge.state[i];
            index++;
        }
    }
    return 0;
}



	


