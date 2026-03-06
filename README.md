# AIRandomForDnD
A specialized toolset designed to solve the "Narrative Bias" problem in LLM-driven tabletop roleplaying. This project provides a verifiable, external entropy source for AI Dungeon Masters who tend to favor "Storybook Outcomes" over statistical reality.

## The Problem
Large Language Models (LLMs) are predictive engines. In a D&D context, they often "hallucinate" high dice rolls for players and low rolls for monsters to satisfy the narrative trope of heroic success. They do this because they are forced to "predict" the outcome of a die roll with no basis for calculating it outside of common narrative tropes. This eliminates narrative bias that undermines the mechanical integrity of the game.

## The Solution
This repository contains C-based utilities to generate and manage a **Entropy Table of 600**. 

1. **Entropy Generation**: Generates a set of 600 unique integers ($[1, 600]$) in a randomized sequence. 600 is the least common multiple of D{4,6,8,10,12,20,100} which allows for each possible outcome to be sampled a uniform number of times.
2. **Uniform Distribution**: Ensures every value in the range appears exactly once (on average), providing a perfect uniform distribution of random outcomes that can be audited for fairness.
3. **Predictable Sequence**: The size of the table, 600, gives the minimum length of non-repeating results. Sequence predictability enables the AI to avoid making up die rolls to suit the narrative, since it knows the answer and doesn't need to guess. With 600 rolls before repeating, the author feels this is enough chaos the average game session is unlikely to detect the issue or care. 
3. **Markdown Export**: Formats the sequence into a `.md` table for easy integration into an LLM's context window.
4. **C library interface**: This was unnecessary since the AI will not call it, but was handy for testing.

## How It Works
The LLM (acting as DM) is provided with the table and a "Current Index." For every check, the LLM must:
1. Keep a master "current index" which describes which table row we use next.
2. Read the appropriate roll by die type from the table from the row given by the master index. 
3. Report the Index used for user verification; Report the die roll. A concise format such as die_roll[current_index] is sufficient
4. If the game state is saved, the current index is saved with it  (nice to have)
5. If the table wants a bit more randomness at start, they can ask the AI to adjust the dice index to some other number [1,600]

Note: we could ask the LLM to use the D600 value directly and apply suitable modulo operations with appropriate rounding according to desired die type to reduce the size of the table. Unfortunately, the AI and I both agree that this arithmetic is another opportunity for narrative interference into the value, and it is better to simply have the AI read the value from the table. This also makes the result auditable, in the case you begin to suspect that AI context compaction has reduced the table to "Table of Random values here" and the AI is fudging it all. It may be necessary to periodically reload the table as a .md file to restore it to context so it resumes function again.

## Known Bugs
1. AI context compaction will probably eventually force the table out of the AI context to be replaced by a summary, such as "Table of Dice Values Appears Here" with no accompanying table, leading to more narrative hallucination by the AI. It is expected the user, suspicious that the game is getting too easy, can just send the fair_dice.md file to the AI again to refresh the table and restore fidelity.

## Core Components
- `AIRandom.h`: Header declaring threadsafe public interfaces (in the unlikely event this is packaged as a library)
- `AIRandom.c`: Implementation
- `test.c` : Some test code to print out the table verify uniform distrubtion and table values are between [1,N] as desired.
- `fair_dice.md`: Suitable for passing to AI for use with Table top RPGs

## Mathematical Mapping
To derive standard D&D dice from the 500-unit Master Value ($V$):  V is in [1,600]
- **d20**: `((V-1) % 20) + 1`
- **d12**: `((V-1) % 12) + 1`
- **d10**: `((V-1) % 10) + 1`
- **d8**:  `((V-1) % 8) + 1`
- **d6**:  `((V-1) % 6) + 1`
- **d4**:  `((V-1) % 4) + 1`
- **d100**: `(v % 100) + 1`

*Note: Because 600 is a multiple of 4, 6. 8, 10, 12, 20, 100, and 600, the distribution for these dice remains perfectly uniform. *

## License
MIT - Use it to keep your AI honest.
