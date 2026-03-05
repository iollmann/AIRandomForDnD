# AIRandomForDnD
A specialized toolset designed to solve the "Narrative Bias" problem in LLM-driven tabletop roleplaying. This project provides a verifiable, external entropy source for AI Dungeon Masters who tend to favor "Storybook Outcomes" over statistical reality.

## The Problem
Large Language Models (LLMs) are predictive engines. In a D&D context, they often "hallucinate" high dice rolls for players and low rolls for monsters to satisfy the narrative trope of heroic success. This eliminates narrative bias and undermines the mechanical integrity of the game.

## The Solution
This repository contains C-based utilities to generate and manage a **Permutation Table of 500**. 

1. **Entropy Generation**: Generates a set of 500 unique integers ($[1, 500]$) in a randomized sequence. 500 is the least common multiple of D{4,6,8,10,12,20,100}
2. **Uniform Distribution**: Ensures every value in the range appears exactly once, providing a "Perfect Deck" that can be audited for fairness.
3. **Markdown Export**: Formats the sequence into a `.md` table for easy integration into an LLM's context window (e.g., a `DNDAdventureSynopsis.md` file).
4. **Pointer System**: A client-side C function to track the "Current Index," ensuring no roll is reused or skipped.

## How It Works
The LLM (acting as DM) is provided with the table and a "Current Index." For every check, the LLM must:
1. Keep a master "current index" which describes which table row we use next.
2. Read the appropriate roll by die type from the table from the row given by the master index. 
3. Report the Index used for user verification; Report the die roll. A concise format such as die_roll[current_index] is sufficient
4. If the game state is saved, the current index is saved with it

Note: we could ask the LLM to use the D500 value directlty and apply suitable modulo operations with appropriate rounding according to desired die type to reduce the size of the table. Unfortunately, the AI and I both agree that this arithmetic is another opportunity for narrative interference into the value, and it is better to simply have the AI read the value from the table. This also makes the result auditable, in the case you begin to suspect that AI context compaction has reduced the table to "Table of Random values here" and the AI is fudging it all. It may be necessary to periodically reload the table as a .md file to restore it to context so it resumes function again.

## Core Components
- `generator.c`: The core RNG logic and distribution verifier.
- `reader.c`: A lightweight utility for developers to pull values from the generated table.
- `test.c` : Some test code to verify uniform distrubtion and table values are between [1,N] as desired.
- `fair_dice.md`: The output file containing the 500-unit sequence and AI instructions.

## Mathematical Mapping
To derive standard D&D dice from the 500-unit Master Value ($V$):
- **d20**: `(V % 20) + 1`
- **d12**: `(V % 12) + 1`
- **d10**: `(V % 10) + 1`
- **d8**:  `(V % 8) + 1`
- **d6**:  `(V % 6) + 1`
- **d4**:  `(V % 4) + 1`
- **d100**: `(V % 100) + 1`

*Note: Because 500 is a multiple of 4, 5, 10, 20, 100, and 500, the distribution for these dice remains perfectly uniform. For d6, d8, and d12, the slight modulo bias over a 500-sample set is negligible compared to standard LLM narrative bias.*

## License
MIT - Use it to keep your AI honest.
