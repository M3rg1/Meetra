# Meetra

UCI compatible chess engine.

This is a hobby project of mine, started on June 16th 2021. I do have some very limited chess engine experience already - I've made a chess GUI for networking games
with an integrated, custom engine in Java and another engine in C#. However this is my first serious project in the world of chess programming, that i intend to take all
the way to the top (whatever that means :D ).

The main philosophy behind this project is to make as bug-free engine as possible and to maximize the ELO gain from every single feature that gets implemented, in as little
lines of code as possible, while still maintaining readable, expressive and easy to understend codebase. I'm in no rush to clutter the code with dozens of features and half of
them only barely working.

Because i'm a negative ELO player in real life (randomly moving player would probably beat me), i've decided not to attempt to make any custom evaluation for now. I will
be using the publicly available Pesto piece-square tables until i implement my own neural net. Originally i intended to keep this project private until i make the net.
Using someone eleses evaluation tables doesnt feel the best, but seeing as the net is still quite a while away, ive decided to make the project public because ... why not?
And by being public, i'm at least more motivated to progress with the work faster.

The roadmap for developement is as follows:

1. Create a solid framework to work with.
This includes basic stuff like efficient move generator, board representation, multithreading support, transposition table, chess 960 support and some more advanced UCI
options like MultiPv.

2. Make the search as good as possible.
Implement various techniques like LMR, Futility pruning, razoring ... The dream would be to hit 3000 ELO (CCRL) with only the Pesto tables for evaluation. This isnt a strict
requirement though. Basically whenever i feel like the search is good enough, i'll consider this phase done.

3. Implement custom NNUE.
This goal to complete might take a bit of time and some trial-and-error approch, as my experience with NNUEs is pretty much non-existent. I'll have to learn and research
things from zero.

4. Experiment with differnt search methods (Monte Carlo Tree Search)
While i've already implemented MCTS for other board games before, this would be the first time i'd attempt it in such a large project. I really like the idea and even if
the result might not be as good as with the more classical alpha-beta approach, i'd like to have this perhaps as an UCI option.

5. Improvements to playing strength.
There's no particular target ELO. Basically try to see how far on the rating list can we make it.

6. Create a custom GUI
Now we are coming to the dream land. I like playing around with OpenGL and I also like chess engines. Wouldn't it be cool to combine these two things? I'd like to
attempt to create at least a very basic 3D chess game using OpenGL. Just something where you could move pieces around and nothing more. But again, this is still very
far away and such a project would be potentially very time consuming as well. I'm still not totally convinced whether this is even a good idea in the first place.

7. Be the first engine to reach 4000 ELO
Ok, it's time to calm down.


This is what i'd like to achieve. But of course, these goals are just a very rough outlines of how i imagine the progression will be. Reality is often more difficult.
Currently the project is in it's early 2nd phase - search improvements. I'm also doing a lot of code refactoring that doesn't necessarily have impact on playing strength but
makes it easier to maintain. Refactoring and rewriting will always be the one constant of this project.

By plying several other engines from the CCRL ratings list, Meetra's estimated ELO is around 2500 right now.

UCI feates:
mute plies
multipv
show currline
show currmove
hash
threads

technical impementation:
board and game state reprensentation:

search improvements:

additional features:



If you are insterested in compiling the sources yourself, you might need to upgrade your compiler. This project is developed under the C++ 23 standard. There's some features that only the newest GCC compiler currently supports.
