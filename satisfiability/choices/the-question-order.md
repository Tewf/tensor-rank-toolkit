# A fourth choice, and the only one measurement should not have settled

**Which order the questions are asked in**, which sits outside the three above
for a reason worth naming. **One schedule is implemented and five were priced**,
which [`search/`](../search/README.md) shows is the same thing here: a question's
does not depend on the order it is reached in, so pricing every question prices
every schedule over them, the four nobody wrote included. It barely matters. The
two mandatory questions are 108.461 s of a 110 to 114 s search on GF(16), so the
whole choice is worth about 3%, and the fastest schedule beats the shipped
default by 2.2%.

**The three above were settled by running both because no paper answered them.**
This one had an answer already. `[morgado2013]` named all five of these schedules
and priced their oracle calls a decade earlier, so it was settled by pricing five
because nobody looked first.

What it did **not** say is that bisection loses in practice: its own assessment
puts BIN ahead of linear UNSAT-SAT, 261 solved against 185. That verdict is
`[heras2011]`'s, about core-guided binary search. So our result is a contrast with
the survey rather than a repetition of it, and the reason is ours to give: our cost
range is a dozen values wide, too narrow for the asymptotics to act on. The full
argument is in
[`search-in-the-literature/`](../search-in-the-literature/README.md).
