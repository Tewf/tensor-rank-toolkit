# tensor-rank-toolkit

[![CI](https://github.com/Tewf/tensor-rank-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/Tewf/tensor-rank-toolkit/actions/workflows/ci.yml)
[![Site](https://img.shields.io/badge/pages-tewf.github.io%2Ftensor--rank--toolkit-1f6feb)](https://tewf.github.io/tensor-rank-toolkit/)
[![Licence](https://img.shields.io/badge/licence-MIT-lightgrey)](LICENSE)

> [Read in English](README.md)

**Rang tensoriel et bilinéaire exact sur les corps finis.** Le rang d'une
application bilinéaire est le nombre de multiplications nécessaires pour la
calculer. Le sept-au-lieu-de-huit de Strassen pour les matrices 2×2 est à
l'origine de la multiplication matricielle rapide, et trouver de telles
décompositions dans le cas général reste un problème ouvert.

Le problème est ici attaqué sur neuf axes, d'une descente peu coûteuse à un
solveur jusqu'à une forme canonique qui ne cherche rien du tout. **Rien ici n'est
jamais un flottant** : un rang annoncé est donc un fait sur l'application et non
un artefact d'arrondi.

Chaque compte ci-dessous est vérifié par la suite de tests et se reproduit
partout. Les temps de calcul, eux, non, et rien ne le prétend : voir
[`MEASURING.md`](MEASURING.md) pour le protocole et [`reproduce/`](reproduce/)
pour le pilote qui les régénère.

## Ce qu'il calcule

Neuf axes. Les nombres ci-dessous sont vérifiés par la suite de tests et se
reproduisent partout ; la forme longue de chacun, avec sa méthode et ses
réserves, est dans [`what-it-computes.md`](what-it-computes.md), en anglais
comme le reste de la documentation technique.

| Axe | Question | Résultat |
|---|---|---|
| [descente](descent_search/) | le rang par le haut, à bas prix | F2 5x5 à **14**, F3 3x6 à **10** |
| [exhaustion](exhaustive_search/) | le rang tranché, avec preuve | **rang de matmul 2x2 = 7** : 7 trouvé et vérifié, 6 réfuté |
| [sommes de rangs](linear_algebra/tensor_rank_sum.h) | un plancher sans recherche | GF(16) de 4 à **8**, en millisecondes |
| [faisceaux](pencil_rank/) | deux tranches, en temps polynomial | la forme de Kronecker, et là où Ja'Ja' cesse de valoir |
| [factorisation](canonical_factorisation/) | le rang comme `S = C A` | une réponse avec un reçu que quiconque peut multiplier |
| [satisfiabilité](satisfiability/) | la même question, à un solveur | sans réservoir, et une réfutation vérifiable en DRAT |
| [symétrie](orbit_reduction/) | un membre par orbite | **39,2x moins de nœuds** sur une réfutation, 261 121 applications en **13 orbites** |
| [sans isomorphes](oracle_guided_search/) | chaque classe une seule fois | **22 779x moins de nœuds** sur matmul 2x2 |
| [creusement](matrix_sparsification/) | moins d'additions, rang fixé | les opérateurs de Strassen de **12 non-nuls à 10** |

## Le résultat qui vaut d'être énoncé à part

**L'étape coûteuse est mal tarifée.** L'étape 3 de la descente énumère le vivier
complet des applications de rang un. Sur les quatre jeux de test polynomiaux elle
a amélioré le résultat dans **deux cas sur quatre**, d'un produit chaque fois,
pour un coût compris entre **58 et 184 fois** celui des deux premières étapes
réunies. Toute suite qui se contenterait d'accélérer l'étape 3 optimiserait la
partie qui, le plus souvent, ne paie pas, et
[`fixtures/README.md`](fixtures/README.md) existe pour maintenir ce constat.

## Une seule chaîne

La recherche de rang reconstruit les opérateurs ⟨L, R, P⟩ et les écrit ; le
creusement est ce à quoi ils servent.

```sh
minimise-rank fixtures/f2_5x5.tensor --emit-operators out   # 25 -> 14 multiplications
sparsify-operator out_L.sms                                 # 31 -> 27 coefficients non nuls
```

## Ce qu'il y a, et où

Treize modules et **douze outils**, détaillés dans
[`what-is-where.md`](what-is-where.md) : ce que sert chaque dossier et quel outil
répond à quelle question. La question à laquelle chacun répond et qu'aucun autre
ne traite, et pourquoi douze plutôt que huit :
[`OPTIONS/one-question-per-command.md`](OPTIONS/one-question-per-command.md).
Chaque option, sa valeur par défaut et la mesure qui l'a choisie :
[`OPTIONS.md`](OPTIONS.md). Chaque article implémenté est nommé une seule fois,
dans [`references.md`](references.md), et le code en cite la clé.
## Compilation

Il faut un compilateur C++20, CMake >= 3.22, **Givaro** et les en-tetes de
**Boost** (`sudo apt install libgivaro-dev libboost-dev`), les deux seules
dependances de compilation ; Boost ne sert qu'a `vendor/permlib/`. Tous les
solveurs sont
optionnels et cherches sur le `PATH` a l'execution.

```sh
cmake -B build -G Ninja && cmake --build build
ctest --test-dir build            # tout, environ deux minutes
ctest --test-dir build -LE slow   # sans les recherches couteuses
```

[`Containerfile`](Containerfile) fixe un environnement pour reproduire un nombre
publie.

## Citation

[`CITATION.cff`](CITATION.cff). Licence : MIT, voir [`LICENSE`](LICENSE) et
[`NOTICE`](NOTICE) pour la portée et pour le crédit du matériel qui n'est pas de
moi.
