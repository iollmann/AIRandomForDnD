// License (MIT)
//  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
//  OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  THIS DOCUMENT IS A MATHEMATICAL UTILITY PROVIDING A FRAMEWORK FOR ENTROPY GENERATION.
//  THE NOMENCLATURE HEREIN (E.G. 'D4', 'D20', ETC.) REFERS TO STANDARD POLYHEDRAL DICE,
//  GEOMETRIC CONCEPTS AND COMMON-USE TERMINOLOGY THAT PREDATE MODERN TABLETOP GAMING
//  SYSTEMS. THIS FILE DOES NOT CONTAINS 'PRODUCT IDENTITY' OR 'CORE RULES' OF ANY
//  SPECIFIC GAMING SYSTEM.

//  THIS WORK IS RELEASED UNDER THE MIT LICENSE. ITS INCLUSION IN, OR BUNDLING WITH,
//  SOFTWARE OR DOCUMENTATION GOVERNED BY SPECIFIC GAMING LICENSES (SUCH AS THE OGL
//  OR FAN CONTENT POLICIES) DOES NOT CONSTITUTE AN ADMISSION THAT THIS WORK IS
//  DERIVATIVE OF THOSE SYSTEMS. THIS FILE REMAINS A WHOLLY DISTINCT, SYSTEM-
//  AGNOSTIC UTILITY.

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
    uint16_t    d600;
}DieTable;

/*! @abstract initialize values according to value in d600 */
static inline void DieTable_InitValues( DieTable * /*nonnull*/ entry)
{
    uint32_t val = entry->d600;
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
        // Create initial uniform distribution of all values [1,600], in order
        for( unsigned long i = 0; i < AIRANDOM_TABLE_SIZE; i++)
            dieTable[i].d600 = i+1;
        
        // We will use a fixed random number seed so we get the same table every time
        uint32_t seed = 11974;  // January 1974, date D&D invented
        
        //randomly swap entries 10000 times. Note, we use same random swap pattern every time, by design
        for( unsigned long i = 0; i < 10000; i++)
        {
            int index1 = rand_r(&seed) % AIRANDOM_TABLE_SIZE;
            int index2 = rand_r(&seed) % AIRANDOM_TABLE_SIZE;
            
            // swap the d600 table values at index1 and index2
            uint16_t temp = dieTable[index1].d600;
            dieTable[index1].d600 = dieTable[index2].d600;
            dieTable[index2].d600 = temp;
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
    if( dieType >= AIRandom_D600 )
        result.result = dieTable[offset].d600;            // bad value passed in for die type. Return D600 result.
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
    fprintf(where, "## License (MIT) \n"
                    "%%%% THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,  %%%%\n"
                    "%%%% INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A        %%%%\n"
                    "%%%% PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT   %%%%\n"
                    "%%%% HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF %%%%\n"
                    "%%%% CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE %%%%\n"
                    "%%%% OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                        %%%%\n"
                    "\n"
                    "%%%% THIS DOCUMENT IS A MATHEMATICAL UTILITY PROVIDING A FRAMEWORK FOR ENTROPY GENERATION.%%%%\n"
                    "%%%% THE NOMENCLATURE HEREIN (E.G. 'D4', 'D20', ETC.) REFERS TO STANDARD POLYHEDRAL DICE, %%%%\n"
                    "%%%% GEOMETRIC CONCEPTS AND COMMON-USE TERMINOLOGY THAT PREDATE MODERN TABLETOP GAMING    %%%%\n"
                    "%%%% SYSTEMS. THIS FILE DOES NOT CONTAINS 'PRODUCT IDENTITY' OR 'CORE RULES' OF ANY       %%%%\n"
                    "%%%% SPECIFIC GAMING SYSTEM.                                                              %%%%\n"
                    "\n"
                    "%%%% THIS WORK IS RELEASED UNDER THE MIT LICENSE. ITS INCLUSION IN, OR BUNDLING WITH,     %%%%\n"
                    "%%%% SOFTWARE OR DOCUMENTATION GOVERNED BY SPECIFIC GAMING LICENSES (SUCH AS THE OGL      %%%%\n"
                    "%%%% OR FAN CONTENT POLICIES) DOES NOT CONSTITUTE AN ADMISSION THAT THIS WORK IS          %%%%\n"
                    "%%%% DERIVATIVE OF THOSE SYSTEMS. THIS FILE REMAINS A WHOLLY DISTINCT, SYSTEM-            %%%%\n"
                    "%%%% AGNOSTIC UTILITY.                                                                    %%%%\n");

    fprintf(where, "\n---\n\n");

    // 1. instructions
    fprintf(where,  "## AI Random Number Table\n"
                    "AI, This table is the **Absolute Source of Truth** for all dice rolls.\n"
                    "To ensure statistical fairness, follow these rules strictly:\n"
                    "\n"
                    "### Persistent Global Variables\n"
                    "// Used to find the dice table row \n"
                    "static uint32_t fairDiceTableIndex = 0;    // 0 means uninitialized \n"
                    "\n"
                    "// Used to count the number of times the user has sent a prompt to the AI\n"
                    "uint32_t fairDicePromptCount = 0; // 0 means uninitialized \n"
                    "\n"
                    "static const uint32_t kFairDiceTableSize = %d;\n\n", AIRANDOM_TABLE_SIZE );

    fprintf(where,  "### Persistent Variable Initialization\n"
                    "// Procedure to resolve the problem if fairDiceTableIndex is uninitialized (has value 0)"
                    "void InitializeFairDiceTableIndex(){\n"
                    "   // Using the numeric values for the current real world time provided by your runtime environment\n"
                    "   //  format: [YEAR]-[NUMERIC_MONTH]-[DAY_OF_MONTH] [HOURS]:[MINUTES]:[SECONDS]\n"
                    "   //  If you do not know the current date and time, ask the user for the current date and time and initialize the variable\n"
                    "   //  AI clarification: This modifies the global variable declared in [Persistent Global Variables](#persistent-global-variables).\n"
                    "   fairDiceTableIndex =  (([DAY_OF_MONTH]*24 + [HOURS])*60 + [MINUTES])*60 + [SECONDS];\n"
                    "}\n\n" );

    fprintf(where,  "### Obtaining The Current FairDiceTableRow\n"
                    "uint32_t GetCurrentFairDiceTableRow(){\n"
                    "   // Using the current fairDicePromptCount, which counts the number of prompts that have been issued so far.\n"
                    "   // If it is 0, meaning uninitialized, set it to 1\n"
                    "   if( fairDiceTableIndex == 0)        // if is uninitialized \n"
                    "   {\n"
                    "       //  AI clarification: This modifies the global variable declared in [Persistent Global Variables](#persistent-global-variables).\n"
                    "       fairDiceTableIndex =  (fairDiceTableIndex + [PROMPT_COUNT]) %% kFairDiceTableSize + 1;  //Range: [1,kFairDiceTableSize]\n"
                    "   }\n"
                    "   return fairDiceTableIndex;\n"
                    "}\n\n" );

                    
            

    fprintf(where, "**Finding the Table Value using an Index**: Use the index to find the table row. \n");
    fprintf(where,  "### Subroutine [ROLL_FAIR_DIE]: \n"
                    "   Using: FAIR_DICE_TABLE (Below, the indices are in order given by the first column.)\n"
                    "   1. TABLE_ROW = GetCurrentFairDiceTableRow(); // The first row is row 1 containing index value 1, The second row is row 2 containing index value 2, and so forth\n"
                    "   2. The TABLE_COLUMN is obtained by using the die type (e.g. \"d8\") to find the correct column looking at the table header. If it is a \"d8\" use the d8 column\n"
                    "   3. The value at the intersection of TABLE_ROW and TABLE_COLUMN is the correct die result.\n"
                    "\n" );

    fprintf(where, "**Reporting Style**: Integrate the dice result into the narrative flow. For multiple dice (like 2d6), list each individual result and the modifier. \n");
    fprintf(where, "   - **Pattern**: **[Action]: [Total Result] [Status]!**   [Dice Type] + [Mod], Rolled: [Rolls], Indices: [#X]\n");
    fprintf(where, "   - **Example**: \"Action Result: 14  It's Dead!   2d6 + 4, Rolled: {6, 4}, Indices: {#11, #12}\" \n");
    fprintf(where, "   - **Example**: \"Pea Shooter: 13  A Hit!  d12 + 4, Rolled: 9, Index: #142\" \n");
    fprintf(where, "   - **Intent**: Showing all dice and table indices allows the reader to audit the roll by seeing exactly which values were pulled from the table for each specific die.\n");

    fprintf(where, "\n---\n\n");
    
    // 2. Write the Table Header
    fprintf(where,  "## FAIR_DICE_TABLE\n"
                    "| Index |  d4 |  d6 |  d8 | d10 | d12 | d20 | d100 | Master Value |\n"
                    "| :---- | :-- | :-- | :-- | :-- | :-- | :-- | :--- | :----------- |\n");
    
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
                entry->d600 );
    }

    fprintf(where, "\n");
}

