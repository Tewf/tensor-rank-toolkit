# exhaustive_search/

The tool that *knows* rather than guesses: `decide-rank` answers whether your
map can be computed in exactly `k` multiplications — yes with a verified
recipe, no with the whole tree walked — at a cost that grows exponentially,
so it settles small maps outright and refuses what cannot fit.

```sh
decide-rank my.tensor --target 7
```

New to all of this: [`../start-here.md`](../start-here.md). Every flag:
[`../OPTIONS/searching-for-rank.md`](../OPTIONS/searching-for-rank.md). The
algorithm in pseudocode with every parameter:
[`../how-the-search-works/`](../how-the-search-works/README.md). What the
code assumes, precisely: the header of
[`exhaustive_search.h`](exhaustive_search.h).
