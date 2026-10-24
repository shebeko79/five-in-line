# Five-in-line ("Five-in-a-Row", "Free style Gomoku") solving project
Like many others this project was inspired by [Go-Moku and Threat-Space Search](doc/allis1994.pdf)

For the Gomoku, it has been proven that, in case of the perfect game, wins the player who makes the first move.

I wonder if it still the case if we have infinite field.

# Solved tree
Completly solved tree available here: http://93.127.143.124/solved/

It has its own drawbacks:
* It's not a shortest tree. Not even close.
* I also assume that any move, behind five-lines of existing moves, makes position worse. Which, of course, also need to be proved.

# Links to other projects

* Similar idea, but much better implementation then mine: https://github.com/fucusy/gomoku-first-move-always-win/
* It's online version: https://www.bytedance.ai/gomoku.html
* YIXIN, powerful Gomoku engine: https://www.aiexp.info/pages/yixin.html
* Gomoku AI Cup site: https://gomocup.org/links/
