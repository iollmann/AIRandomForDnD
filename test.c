#include "AIRandom.h"
#include <string.h>
#   define NDEBUG 0
#include <assert.h>

#define NODEBUG     __attribute__ ((nodebug))

bool failed = false;

static inline void NODEBUG check( long isTrue )
{
    if( 0 == isTrue)
    {
        failed = true;
#if DEBUG
        __builtin_trap();
#endif
    }
}


// Test code and main()

int main( int argc, const char ** argv)
{
    AIRandom_PrintDieTable(stdout);
    
    
    // test values
    static const uint8_t dieSizes[7] = {4,6,8,10,12,20,100};
    
    // Iterate over die sizes
    for( AIRandom_DieType dieType = AIRandom_D4; dieType <= AIRandom_D100; dieType++)
    {
        uint32_t count[101];    memset( count, 0, sizeof(count));
        uint32_t dieFaces = dieSizes[dieType];
        
        for( unsigned long i = 0;  i < AIRANDOM_TABLE_SIZE; i++)
        {
            AIRandom_DieRoll roll = AIRandom_RollDie(dieType);
            check( roll.index > 0 && roll.index <= AIRANDOM_TABLE_SIZE );
            check( roll.result > 0 && roll.result <= dieFaces);
            count[roll.result]++;
        }
        
        uint32_t correctResult = AIRANDOM_TABLE_SIZE / dieFaces;
        unsigned long i = 0;
        check( count[0] == 0);                     // verify no die rolls 0
        
        // make sure we are uniformly distributed for results [1, dieFaces]
        for( i = 1; i <= dieFaces; i++)
            check( count[i] == correctResult);  // result distribution is incorrect
        
        // make sure no out of range result occurred.
        for( ; i <= 100; i++)
            check( count[i] == 0);           // make sure no out of range results occurred
    }
    
    if( failed )
        printf( "\nTest failed.\n");
    else
        printf( "\nTest passed.\n");

    return 0;
}
