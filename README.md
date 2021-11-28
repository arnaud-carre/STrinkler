# STrinkler - Atari exe packer suited for 4KiB demo

Use [Shrinkler](https://github.com/askeksa/Shrinkler) packing technology by Blueberry/Loonies

Atari platform support by Leonard/Oxygene

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

## History

v0.10
- First public version

