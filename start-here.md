# Start here

One page for the visitor who wants one thing and does not know the field.
Plain words; each deeper page is linked exactly where it becomes useful.

**What this toolkit does.** You hand it a small multiplication problem
(matrix multiplication, polynomial multiplication, or in general any
*bilinear map*: a rule that combines two lists of numbers into a third,
linearly in each), and it finds a way to compute that problem with **fewer
multiplications**, or proves that none exists. Three words carry everything
here:

- a **tensor** is your problem, written as a stack of matrices in a text file;
- its **rank** is the fewest multiplications any recipe for it can use;
- a **decomposition** is such a recipe: which sums to multiply, and how to
  add the products back into the answer.

Once, before anything: build and install, the five lines under **Building**
in the [README](README.md). After that the commands below run as typed.

## "I have a tensor and want a decomposition"

Write the file. This is Karatsuba's problem, whole; yours differs only in its
numbers. `field` is the arithmetic and must be a prime; 2, bit arithmetic,
is the common case here. `shape 3 2 2` means three matrices of 2x2, and blank
lines separate them:

```
field 2
shape 3 2 2

1 0
0 0

0 1
1 0

0 0
0 1
```

Then:

```sh
minimise-rank my.tensor --emit-operators out
```

Seconds. It prints the multiplication count it reached, and the recipe is the
three files it wrote: row `i` of `out_L.sms` and row `i` of `out_R.sms` are
the two sums that product `i` multiplies, and `out_P.sms` adds the products
back into your answer. That answer is **good but not guaranteed best**. To
know the best (hours, or forever on anything not small) ask instead:

```sh
decide-rank my.tensor              # the rank itself, small problems only
decide-rank my.tensor --target 2   # one question: do 2 multiplications suffice?
```

Yes means a recipe was found and checked against your map before the verdict
prints, no comes with a proof, and out of time says so honestly. On the file
above the second line answers no, because Karatsuba's 3 is the rank.
`--target` asks about that exact count, so ask at or below what you hope for.
In a script: yes is exit 0, a proved no is 1, giving up is 3, and none of the
three is a crash. Read [`OPTIONS.md`](OPTIONS.md) before `set -e`.

**`decide-rank` proves the count and writes no files; the recipe files come
from the finders.** So when you want the best recipe you can hold in your
hands, ask the sharper finder and watch one number:

```sh
tighten-rank-bound my.tensor --emit-operators out
```

It starts from `minimise-rank`'s answer and searches below it. The line it
prints ends in a gap: **`gap 0` means the count reached meets a proven floor,
so the files written are a best possible recipe**, no exhaustive search
needed. A larger gap is the space still open between what was found and what
is proven, and only `decide-rank` can close it.

## "I don't have a tensor file"

Say the problem instead, and a tool writes the file:

```sh
make-tensor --matmul 2 2 2 2 > my.tensor       # 2x2 by 2x2 matrix multiplication
make-tensor --polynomial 2 5 5 > my.tensor     # two 5-term polynomials
operators-to-tensor L.sms R.sms P.sms -q 2 > my.tensor   # a paper's recipe, read in
```

## "I have a recipe already and want fewer additions"

`sparsify-operator their_L.sms`. The multiplication count stays; the
additions around it reach the provable minimum.
[`OPTIONS/sparsifying-operators.md`](OPTIONS/sparsifying-operators.md).

## "I would rather click than type"

`python3 web_interface/serve.py` puts the same tools in the browser: pick a
shipped example or open your own file, run, stop, and keep every run's
evidence. [`web_interface/`](web_interface/README.md).

## When you want to know what just happened

What each method computes and its caveats:
[`what-it-computes.md`](what-it-computes.md). Which folder holds what:
[`what-is-where.md`](what-is-where.md). Every flag, with the measurement
behind each default: [`OPTIONS.md`](OPTIONS.md).
