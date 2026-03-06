// License (MIT)
//  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
//  OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "AIRandom.h"
#include "stdatomic.h"
#include <time.h>
#include <dispatch/dispatch.h>  // libdispatch ("GCD")

#if ! DEBUG
#   define NDEBUG 1
#endif
#include <assert.h>


typedef struct
{
    uint8_t    values[7];   // {D4, D6, D8, D10, D12, D20, D100
    uint16_t    d500;
}DieTable;

/*! @abstract initialize values according to value in d500 */
static inline void DieTable_InitValues( DieTable * /*nonnull*/ entry)
{
    uint32_t val = entry->d500;
    assert( 1 <= val && val <= AIRANDOM_TABLE_SIZE);
    
    val = val - 1;   // convert 1 based numbers to 0 based numbers so that the modulo operator does what we want here. 
    entry->values[0] = val % 4 + 1;        //D4:   [1, 4]
    entry->values[1] = val % 6 + 1;        //D6:   [1, 6]
    entry->values[2] = val % 8 + 1;        //D8:   [1, 8]
    entry->values[3] = val % 10 + 1;       //D10:  [1, 10]
    entry->values[4] = val % 12 + 1;       //D12:  [1, 12]
    entry->values[5] = val % 20 + 1;       //D20:  [1, 20]
    entry->values[6] = val % 100 + 1;      //D100: [1, 100]
}

static DieTable dieTable[AIRANDOM_TABLE_SIZE];   // Initialized to 0 by C language rules


/*! @abstract Get the next table index.  Thread safe. Reentrant safe. */
static inline int GetTableIndex(void)
{
    static atomic_int gMasterIndex;    // initialized to 0 by C rules

    int index, newIndex;
    do
    {
        index = atomic_load_explicit( &gMasterIndex, memory_order_acquire);
        if( 0 == index )    // uninitialized
            newIndex = (int)(time(NULL) % AIRANDOM_TABLE_SIZE + 1);
        else
        {   // proceed to the next index, modulo table size
            newIndex = index+1;
            if(newIndex > AIRANDOM_TABLE_SIZE)
                newIndex = 1;
        }
    }while( 0 == atomic_compare_exchange_weak_explicit(&gMasterIndex, &index, newIndex, memory_order_acq_rel, memory_order_relaxed) );
    
    return newIndex;
}

/* @abstract */
static inline void InitializeTable( void )
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{    // Make sure we only do this once. Avoid reentrancy.
        // Create initial uniform distribution of all values [1,500], in order
        for( unsigned long i = 0; i < AIRANDOM_TABLE_SIZE; i++)
            dieTable[i].d500 = i+1;
        
        // We will use a fixed random number seed so we get the same table every time
        uint32_t seed = 11974;  // January 1974, date D&D invented
        
        //randomly swap entries 10000 times. Note, we use same random swap pattern every time, by design
        for( unsigned long i = 0; i < 10000; i++)
        {
            int index1 = rand_r(&seed) % AIRANDOM_TABLE_SIZE;
            int index2 = rand_r(&seed) % AIRANDOM_TABLE_SIZE;
            
            // swap the d500 table values at index1 and index2
            uint16_t temp = dieTable[index1].d500;
            dieTable[index1].d500 = dieTable[index2].d500;
            dieTable[index2].d500 = temp;
        }
        
        // Fill out table values
        for( unsigned long i = 0; i < AIRANDOM_TABLE_SIZE; i++)
            DieTable_InitValues(&dieTable[i]);
    });
}

static inline AIRandom_DieRoll ReadDie( AIRandom_DieType dieType, uint32_t index )
{
    if( dieTable[0].values[0] == 0) // uninitialized
        InitializeTable();
    
    AIRandom_DieRoll result;
    result.index = index;
    
    uint32_t offset = result.index - 1;  // deal with C 0-based indexing
    if( dieType >= AIRandom_D500 )
        result.result = dieTable[offset].d500;            // bad value passed in for die type. Return D500 result.
    else
        result.result = dieTable[offset].values[dieType];    // return die roll of correct result
    
    return result;
}

AIRandom_DieRoll AIRandom_RollDie( AIRandom_DieType type )
{
    uint32_t index = GetTableIndex();
    return ReadDie( type, index);
}


bool AIRandom_AuditRoll( AIRandom_DieRoll allegedRoll,  AIRandom_DieType type )
{
    AIRandom_DieRoll validRoll = ReadDie( type, allegedRoll.index);
    return validRoll.result == allegedRoll.result;
}


extern void AIRandom_PrintDieTable( FILE * /* nonnull*/ where )
{
    // Make sure the table is initialized
    InitializeTable();

    fprintf(where, "# fair_dice.md \n\n");

    // 0. License
    fprintf(where, "## License (MIT) \n");
    fprintf(where, "%%%% THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,  %%%%\n");
    fprintf(where, "%%%% INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A        %%%%\n");
    fprintf(where, "%%%% PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT   %%%%\n");
    fprintf(where, "%%%% HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF %%%%\n");
    fprintf(where, "%%%% CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE %%%%\n");
    fprintf(where, "%%%% OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                        %%%%\n");

    fprintf(where, "\n---\n\n");

    // 1. instructions
    fprintf(where, "## AI Random Number Table\n");
    fprintf(where, "A table of random numbers for use with common table top rollplaying systems.\n");
    fprintf(where, "**Table Size**: %d\n", AIRANDOM_TABLE_SIZE );
    fprintf(where, "This table is the **Absolute Source of Truth** for all dice rolls.\n");
    fprintf(where, "To ensure statistical fairness, follow these rules strictly:\n\n");
    fprintf(where, "1. **Pointer System**: Start at Index 0. Every time a die roll is required (Attack, Save, Damage, etc.), first increase the index by 1. If the index exceeds the table size, set the index to 1. Use the table value **at the new Index**.\n");
    fprintf(where, "2. **Finding the Table Value using an Index**: Use the index to find the table row. (They are in order, and the index is labelled in the first column.) If the index is 1, use the first row. If the index is 2, use the second row. Use the die type to find the table column. If it is a D4, use the 'd4' column. The value at intersection of row and column in the table is the correct die result.\n");

    fprintf(where, "3. **Reporting Style**: Integrate the dice result into the narrative flow. For multiple dice (like 2d6), list each individual result and the modifier. \n");
    fprintf(where, "   - **Pattern**: **[Action]: [Total Result] [Status]!**   [Dice Type] + [Mod], Rolled: [Rolls], Indices: [#X]\n");
    fprintf(where, "   - **Example**: \"Maul Damage (2d6: {6, 4} + 4)= 14 [Index #11, #12]\" \n");
    fprintf(where, "   - **Intent**: Showing all dice allows the reader to audit the math directly by seeing exactly which values were pulled from the table for each specific die. \n");

    fprintf(where, "\n---\n\n");
    
    // 2. Write the Table Header
    fprintf(where, "| Index |  d4 |  d6 |  d8 | d10 | d12 | d20 | d100 | Master Value |\n");
    fprintf(where, "| :---- | :-- | :-- | :-- | :-- | :-- | :-- | :--- | :----------- |\n");
    
    for( unsigned long i = 0; i < AIRANDOM_TABLE_SIZE; i++ )
    {
        DieTable * entry = &dieTable[i];
        fprintf(where, "| %5lu | %3d | %3d | %3d | %3d | %3d | %3d |  %3d |     %3d      |\n",
                i+1,
                entry->values[0],
                entry->values[1],
                entry->values[2],
                entry->values[3],
                entry->values[4],
                entry->values[5],
                entry->values[6],
                entry->d500 );
    }

    fprintf(where, "\n");
}

