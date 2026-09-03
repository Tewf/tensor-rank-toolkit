# search_plan/

The seven choices a run makes about *how* it will be carried out — which leaf
route, which device, quotient or not — collected in one place that owns no
rule of its own. `--plan-out` writes them down, `--plan-in` replays them, so
the same run on other hardware differs in its machine and not in its
decisions.

The seven choices and the three rules that settle them:
[`search_plan.h`](search_plan.h). Why this module links four others and owns
nothing: the top of [`CMakeLists.txt`](CMakeLists.txt).
