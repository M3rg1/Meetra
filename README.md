#### Build & run

##### Build requirements:
- GCC 11 or newer
- CMake 3.00 or newer


Navigate to the root folder and run these commands:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cd build
make
./Meetra
```

#### Usage
The engine is not supposed to be run from the command line. To use it, download any GUI that supports the [UCI protocol](http://wbec-ridderkerk.nl/html/UCIProtocol.html), and proceed with engine installation according to their respective guides.

For example, some free GUIs with UCI support:
- [Arena](http://www.playwitharena.de/) (recommended)
- [Cute Chess](https://cutechess.com/)
- [Banksia](https://banksiagui.com/)
- ... and many more

#### Supported features
- Chess 960 (Fisher random chess)
- Multi PV
- Up to 64 threads
- Up to 32GB transposition table
- Infinite search, limited nodes search, fixed depth search
- Own book (the books currently available arent very good though)
- **Additional UCI commands**
    - Mute plies - the engine won't be sending any output to the GUI up to the specified depth
    - Move overhead - how much time should be set aside for communication delay between the engine and GUI, and any other potential delays (best kept at default value)
    - Clear hash - clears any stored information from previous searches (the GUI will usually handle this)
    - Show current move - whether the engine will be sending information about which move in the root position is currently being searched


#### Internal engine architecture
- **Board representation**
    - [8x8 mailbox board](https://www.chessprogramming.org/8x8_Board)
    - [Bitboards](https://www.chessprogramming.org/Bitboards) (1x all pieces, 2x color, 6x piece)
    - [Little endian rank-file mapping](https://www.chessprogramming.org/Square_Mapping_Considerations)
    - [Zobrist hashing](https://www.chessprogramming.org/Zobrist_Hashing)
- **Move generation**
    - [Staged  move generator](https://www.chessprogramming.org/Move_Generation#Staged_move_generation) (TT move, promotions, captures, quiet moves)
    - Precomputed moves for all pieces
    - [Fancy Magic bitboards](https://www.chessprogramming.org/Magic_Bitboards) (initialized via [Hyperbola Quintessence](https://www.chessprogramming.org/Hyperbola_Quintessence))
- **Move ordering**
    - [MVV-LVA](https://www.chessprogramming.org/MVV-LVA)
    - [Killer heuristic](https://www.chessprogramming.org/Killer_Heuristic) (with 2 slots)
- **Search**
    - [Iterative deepening framework](https://www.chessprogramming.org/Iterative_Deepening)
    - [Fail soft alpha-beta framework](https://www.chessprogramming.org/Alpha-Beta)
    - [Principal variation search](https://www.chessprogramming.org/Principal_Variation_Search)
    - [Parallel search](https://www.chessprogramming.org/Parallel_Search)
        - [Lazy SMP](https://www.chessprogramming.org/Lazy_SMP)
    - [Transposition table](https://www.chessprogramming.org/Transposition_Table)
        - 8 bytes per entry
        - 4 entries per bucket
        - age, depth and node type replacement scheme
        - [lockless shared hash table](https://www.chessprogramming.org/Shared_Hash_Table) (no xor checks either, data races can and do occur, every move is validated for pseudo legality)
    - [Null move pruning](https://www.chessprogramming.org/Null_Move_Pruning)
    - [Reverse futility pruning](https://www.chessprogramming.org/Reverse_Futility_Pruning)
    - [Late move reductions](https://www.chessprogramming.org/Late_Move_Reductions)
    - [Quiesence search](https://www.chessprogramming.org/Quiescence_Search) (no TT probing, with check extensions)
- **Evaluation**
    - [Piece-square tables](https://www.chessprogramming.org/Piece-Square_Tables) with [tapered evaluation](https://www.chessprogramming.org/Tapered_Eval) using the [PeSTO's Evaluation Function](https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function)
- **Opening book**
    - Custom binary book format 
    - Binary search for book probing
