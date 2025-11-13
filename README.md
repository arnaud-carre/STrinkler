# STrinkler - Atari exe packer suited for 4KiB demo

Use Amiga [Shrinkler](https://github.com/askeksa/Shrinkler) packing technology by Blueberry/Loonies

Atari port by Leonard/Oxygene

## Usage

```
Usage: STrinkler [options] <input file> <output file>

Options:
  -1, ..., -9   compression level (low, best) (default=2)
  -mini         minimal PRG size, less compatibility (suited for 4KiB demo)
  -d            raw data mode
  -v            verbose
  -padr <size>  Pad till <size> using random bytes
Advanced options:
  -i <n>        Number of iterations for the compression (2)
  -l <n>        Number of shorter matches considered for each match (2)
  -a <n>        Number of matches of the same length to consider (20)
  -e <n>        Perseverance in finding multiple matches (200)
  -s <n>        Minimum match length to accept greedily (2000)
  -r <n>        Number of reference edges to keep in memory (100000)
```

You can comment on [Pouet](https://www.pouet.net/prod.php?which=90348)

Follow me on Twitter [@leonard_coder](https://twitter.com/leonard_coder) )

## Note for large exes

STrinkler is your best option for 4KiB demo, no contest.
But for larger exe (like 64KiB or more), the depacking time could very big. In this case, if you can afford slightly less compression ratio, you should try [L-Packer](https://github.com/arnaud-carre/L-Packer) (It depacks fast and always beats UPX in size)

## History

v0.10
- First public version

