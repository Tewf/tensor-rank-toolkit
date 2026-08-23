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

Le problème est ici attaqué sur dix axes, d'une descente peu coûteuse à un
solveur jusqu'à une forme canonique qui ne cherche rien du tout. **Rien ici n'est
jamais un flottant** : un rang annoncé est donc un fait sur l'application et non
un artefact d'arrondi.

Chaque compte ci-dessous est vérifié par la suite de tests et se reproduit
partout. Les temps de calcul, eux, non, et rien ne le prétend : voir
[`MEASURING.md`](MEASURING.md) pour le protocole et [`reproduce/`](reproduce/)
pour le pilote qui les régénère.

## Ce qu'il calcule

Dix axes. La forme longue de chacun, avec sa méthode et ses réserves, est dans
[`what-it-computes.md`](what-it-computes.md), en anglais comme le reste de la
documentation technique.

| Axe | Question | Résultat |
|---|---|---|
| [descente](descent_search/) | le rang par le haut, à bas prix | F2 5x5 à **14**, F3 3x6 à **10** |
| [exhaustion](exhaustive_search/) | le rang tranché, avec preuve | **rang de matmul 2x2 = 7** : 7 trouvé et vérifié, 6 réfuté |
| [titulaire](incumbent_search/) | le même arbre, élagué par ce qui est déjà construit | convolution cyclique F2 7, de 15 à **13**, en 22 nœuds |
| [sommes de rangs](linear_algebra/tensor_rank_sum.h) | un plancher sans recherche | GF(16) de 4 à **8**, en millisecondes |
| [faisceaux](pencil_rank/) | deux tranches, en temps polynomial | la forme de Kronecker, et là où Ja'Ja' cesse de valoir |
| [factorisation](canonical_factorisation/) | le rang comme `S = C A` | une réponse avec un reçu que quiconque peut multiplier |
| [satisfiabilité](satisfiability/) | la même question, à un solveur | sans réservoir, et une réfutation vérifiable en DRAT |
| [symétrie](orbit_reduction/) | un membre par orbite | **39,2x moins de nœuds** sur une réfutation, 261 121 applications en **13 orbites** |
| [sans isomorphes](oracle_guided_search/) | chaque classe une seule fois | **22 778x moins de nœuds** sur matmul 2x2 |
| [creusement](matrix_sparsification/) | moins d'additions, rang fixé | un schéma ⟨3,3,3⟩ de rang 23 de **221 non-nuls à 128**, minimum sur tout changement de base, chaque coefficient laissé à 0 ou ±1 |

**La feuille est l'endroit où vit une recherche exhaustive**, et aucune de ses
deux routes n'y forme plus d'élément : le parcours avance en code de Gray
réfléchi sur GF(2) comme sur GF(p), **2,52x par élément sur GF(3)**, le terme en
dimension ayant disparu plutôt que diminué, et le balayage du vivier transporte
un résidu. Mêmes verdicts, mêmes comptes de nœuds, et une carte grand public
tarifée contre les deux : [`gpu_leaf/`](gpu_leaf/).

## Le résultat qui vaut d'être énoncé à part

**L'étape coûteuse est mal tarifée.** L'étape 3 de la descente énumère le vivier
complet des applications de rang un. Sur les quatre jeux de test polynomiaux elle
a amélioré le résultat dans **deux cas sur quatre**, d'un produit chaque fois,
pour un coût supérieur d'**un à deux ordres de grandeur** à celui des deux
premières étapes réunies. Toute suite qui se contenterait d'accélérer l'étape 3
optimiserait la partie qui, le plus souvent, ne paie pas, et
[`fixtures/README.md`](fixtures/README.md) existe pour maintenir ce constat.

## Une seule chaîne

La recherche de rang reconstruit les opérateurs ⟨L, R, P⟩ et les écrit ; le
creusement est ce à quoi ils servent.

```sh
minimise-rank fixtures/f2_5x5.tensor --emit-operators out   # 25 -> 14 multiplications
sparsify-operator out_L.sms                                 # 31 -> 27 coefficients non nuls
```

La console dans le navigateur enchaîne ces deux lignes dans cet ordre, une fois
par opérateur, comme un seul parcours : [`web_interface/`](web_interface/).

## Lire et écrire ce que le domaine publie

Un triplet ⟨L, R, P⟩ au format SMS est ce sous quoi le domaine publie un
algorithme bilinéaire, et à peu près la seule chose qu'il publie : c'est donc la
porte d'entrée autant que la porte de sortie. Deux sources en distribuent en
quantité : le [catalogue FMM](https://fmm.univ-lille.fr/), des milliers de
décompositions classées par rang, et
[PLinOpt](https://github.com/jgdumas/plinopt), une bibliothèque C++ pour les
programmes linéaires et bilinéaires en ligne droite, dont le `data/` livre
Strassen, Winograd, Karatsuba, Toom-3 et la multiplication matricielle jusqu'à
32x32x32.

En lire un est un test et non une affirmation : un triplet de Strassen publié
ailleurs reconstruit, entrée par entrée, le jeu de test que ce dépôt écrit à
partir de la définition de l'application, et un désaccord serait à nous de
l'expliquer. **Rien de tout cela n'est une dépendance** : rien ici ne se lie à
aucun de ces outils, et la suite entière passe sur une machine où aucun n'est
installé.

```sh
operators-to-tensor L.sms R.sms P.sms -q 2 > map.tensor     # un algorithme publié, lu ici
PMchecker out_L.sms out_R.sms out_P.sms -q 2                # le nôtre, vérifié ailleurs
```

Ce qu'il faut installer, les deux directions et les différences qui mordent, sur
une seule page :
[`formats/interchange/exchanging-files.md`](formats/interchange/exchanging-files.md).

## Ce qu'il y a, et où

Treize modules — les répertoires qui possèdent une question, c'est-à-dire chaque
`add_subdirectory` sauf les huit qui n'en portent aucune (`cli`, `testing`,
`run_limits`, `linear_algebra`, `formats`, `map_construction`, `search_plan`,
`gpu_leaf`) — et **treize outils**, détaillés dans
[`what-is-where.md`](what-is-where.md) : ce que sert chaque dossier et quel outil
répond à quelle question. La question à laquelle chacun répond et qu'aucun autre
ne traite, et pourquoi treize plutôt que huit :
[`OPTIONS/one-question-per-command.md`](OPTIONS/one-question-per-command.md).
Chaque option, sa valeur par défaut et la mesure qui l'a choisie :
[`OPTIONS.md`](OPTIONS.md). Douze des treize se pilotent aussi depuis un
navigateur, sur la seule bibliothèque standard de Python 3 :
[`web_interface/`](web_interface/). Chaque article implémenté est nommé une seule
fois, dans [`references.md`](references.md), et le code en cite la clé.

## Deux branches

`main` est ce qui a gagné. **`rejected-experiments` est ce qui a perdu, conservé
entier** : la mesure qui a tranché chaque rejet et l'implémentation qu'elle a
retirée, car un rejet dont on a effacé les preuves ne se distingue plus d'un
caprice. On y trouve le parcours d'orbite que l'image canonique a remplacé, le
quotient par défaut qu'une recherche qui aboutit paie 7,4x, les deux oracles
exacts de creusement de `[beniamini2020]` avec l'heuristique de base de lignes,
et `find-at-rank` avec sa descente. Rien n'y est cassé et rien n'y est maintenu.
L'index de tout cela, avec le nombre qui a retiré chacun :
[`retired/README.md`](https://github.com/Tewf/tensor-rank-toolkit/blob/rejected-experiments/retired/README.md).

## Compilation

Il faut un compilateur C++20, CMake ≥ 3.22, **Givaro** et les en-têtes de
**Boost** (`sudo apt install libgivaro-dev libboost-dev`). Ce sont les deux
seules dépendances de compilation : Boost n'est nécessaire qu'à
[`vendor/permlib/`](vendor/permlib/), pour `boost::next` et `boost::shared_ptr`,
et aucun en-tête hors de cette bibliothèque embarquée ne l'inclut. Tous les
solveurs sont optionnels et cherchés sur le `PATH` à l'exécution. `ccache` est
utilisé quand il est installé et ignoré quand il ne l'est pas, et
[`Containerfile`](Containerfile) fixe un environnement pour reproduire un nombre
publié.

```sh
cmake -B build -G Ninja && cmake --build build
ctest --test-dir build            # tout, environ deux minutes
ctest --test-dir build -LE slow   # sans les recherches coûteuses
```

Ajouter `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` et lier le résultat à la racine de
l'arbre (`ln -sf build/compile_commands.json .`) donne à clangd — et à tout
éditeur ou agent qui lui parle — les vraies options de compilation. Sans cela,
les en-têtes de chaque module paraissent manquants, car chacun possède son
propre répertoire d'inclusion.

## Citation

[`CITATION.cff`](CITATION.cff). Licence : MIT, voir [`LICENSE`](LICENSE) et
[`NOTICE`](NOTICE) pour la portée et pour le crédit du matériel qui n'est pas de
moi.
