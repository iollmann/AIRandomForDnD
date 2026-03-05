#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 500
#define INDEX_FILE ".fate_index"
#define DECK_FILE "deck.md"

/**
 * reader.c
 * Tracks the current position in the Deck of Fate.
 * Usage: 
 * ./reader next      - Get the next roll and increment index.
 * ./reader peek      - See the current roll without incrementing.
 * ./reader reset <n> - Reset the index to n.
 */

typedef struct {
    int index;
    int master_v;
} Roll;

int get_current_index() {
    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) return 1; // Start at Index 1
    int idx;
    fscanf(f, "%d", &idx);
    fclose(f);
    return idx;
}

void save_index(int idx) {
    FILE *f = fopen(INDEX_FILE, "w");
    if (f) {
        fprintf(f, "%d", idx);
        fclose(f);
    }
}

Roll fetch_roll(int target_idx) {
    Roll r = {target_idx, -1};
    FILE *f = fopen(DECK_FILE, "r");
    if (!f) {
        perror("Could not open deck.md");
        exit(1);
    }

    char line[256];
    int current_row = 0;
    
    while (fgets(line, sizeof(line), f)) {
        // Skip header rows (lines starting with #, |, or space)
        if (line[0] != '|') continue;
        if (strstr(line, "Index")) continue;
        if (strstr(line, ":---")) continue;

        int idx, v;
        // Parse the Markdown table row: | Index | Master | d20 | d100 |
        if (sscanf(line, "| %d | %d |", &idx, &v) == 2) {
            if (idx == target_idx) {
                r.master_v = v;
                break;
            }
        }
    }
    fclose(f);
    return r;
}

void print_dice(
