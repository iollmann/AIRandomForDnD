#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TABLE_SIZE 500

/**
 * generator.c
 * * Generates a "Deck of Fate" for LLM tabletop gaming.
 * Creates a permutation of 1-500 to ensure uniform distribution
 * across d100, d20, d10, d5, d4, and d2.
 */

void shuffle(int *array, int n) {
    if (n > 1) {
        for (int i = 0; i < n - 1; i++) {
            int j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

int main() {
    int deck[TABLE_SIZE];
    int check[TABLE_SIZE + 1] = {0};
    
    // Seed the RNG with current time for unique sessions
    srand((unsigned int)time(NULL));

    // 1. Initialize the deck with values 1 to 500
    for (int i = 0; i < TABLE_SIZE; i++) {
        deck[i] = i + 1;
    }

    // 2. Perform Fisher-Yates Shuffle
    shuffle(deck, TABLE_SIZE);

    // 3. Verify Uniformity (Internal Audit)
    int duplicates = 0;
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (check[deck[i]] == 1) duplicates++;
        check[deck[i]] = 1;
    }

    if (duplicates > 0) {
        fprintf(stderr, "Error: Mathematical anomaly detected. Duplicate values present.\n");
        return 1;
    }

    // 4. Output as Markdown Table
    printf("# THE DECK OF FATE\n\n");
    printf("| Index | Master Value (V) | d20 (V%%20+1) | d100 (V%%100+1) |\n");
    printf("| :--- | :--- | :--- | :--- |\n");

    for (int i = 0; i < TABLE_SIZE; i++) {
        int v = deck[i];
        // We provide the raw V, but pre-calculate common dice to save LLM compute
        printf("| %d | %03d | %02d | %02d |\n", 
                i + 1, 
                v, 
                (v % 20 == 0) ? 20 : (v % 20), 
                (v % 100 == 0) ? 100 : (v % 100));
    }

    fprintf(stderr, "\nSuccess: Generated %d unique values [1-500].\n", TABLE_SIZE);
    
    return 0;
}
