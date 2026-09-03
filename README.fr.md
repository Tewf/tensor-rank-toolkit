# tensor-rank-toolkit

[![CI](https://github.com/Tewf/tensor-rank-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/Tewf/tensor-rank-toolkit/actions/workflows/ci.yml)
[![Site](https://img.shields.io/badge/pages-tewf.github.io%2Ftensor--rank--toolkit-1f6feb)](https://tewf.github.io/tensor-rank-toolkit/)
[![Licence](https://img.shields.io/badge/licence-MIT-lightgrey)](LICENSE)

> [Read in English](README.md)

Une bibliothèque de recherche en C++20 pour le rang tensoriel et bilinéaire
exact sur les corps finis et sur les rationnels. Le rang d'une application
bilinéaire est le nombre de multiplications qu'utilise un algorithme
bilinéaire optimal ; les sept multiplications de Strassen pour le produit de
matrices 2×2 en sont l'exemple classique, et ces rangs sont à l'origine de la
multiplication matricielle rapide. Décider le rang tensoriel est NP-complet
sur les corps finis [`[hastad1990]`](references.md) et ∃ℝ-complet sur les
réels [`[schaefer2018]`](references.md) ; la bibliothèque associe donc des
**procédures de décision complètes**, en temps exponentiel, à des
**heuristiques en temps polynomial** et à des bornes inférieures qui
n'exigent aucune recherche. Toute l'arithmétique est exacte, sur GF(p) et ℚ
via Givaro : une recherche sur des rangs et des comptes de non-nuls donne une
autre réponse quand elle est presque juste, donc rien ici n'est jamais un
flottant. Chaque compte ci-dessous est vérifié par la suite de tests ; les
temps de calcul, eux, non, et rien ne le prétend
([`MEASURING.md`](MEASURING.md) donne le protocole,
[`evidence/reproduce/`](evidence/reproduce/README.md) régénère chaque nombre publié avec sa
provenance).

## Organisation du dépôt

Les dix axes de méthode, un répertoire chacun. La méthode et les réserves de
chacun : [`what-it-computes.md`](what-it-computes.md), en anglais comme le
reste de la documentation technique.

| Axe | Question | Résultat |
|---|---|---|
| [descente](methods/bilinear_rank/greedy_heuristic/README.md) | le rang par le haut, à bas prix | F2 5x5 à **14**, F3 3x6 à **10** |
| [exhaustion](methods/bilinear_rank/exhaustive/README.md) | le rang tranché, avec preuve | **rang de matmul 2x2 = 7** : 7 trouvé et vérifié, 6 réfuté |
| [titulaire](methods/bilinear_rank/branch_and_bound/README.md) | le même arbre, élagué par ce qui est déjà construit | convolution cyclique F2 7, de 15 à **13**, en 22 nœuds |
| [sommes de rangs](core/linear_algebra/tensor_rank_sum.h) | un plancher sans recherche | GF(16) de 4 à **8**, en millisecondes |
| [faisceaux](methods/pencil_rank/README.md) | deux tranches, en temps polynomial | la forme de Kronecker, et là où Ja'Ja' cesse de valoir |
| [factorisation](methods/canonical_factorisation/README.md) | le rang comme `S = C A` | une réponse avec un reçu que quiconque peut multiplier |
| [satisfiabilité](methods/satisfiability/README.md) | la même question, à un solveur | sans réservoir, et une réfutation vérifiable en DRAT |
| [symétrie](methods/bilinear_rank/orbit_reduction/README.md) | un membre par orbite | **39,2x moins de nœuds** sur une réfutation, 261 121 applications en **13 orbites** |
| [sans isomorphes](methods/bilinear_rank/canonical_augmentation/README.md) | chaque classe une seule fois | **22 778x moins de nœuds** sur matmul 2x2 |
| [creusement](methods/matrix_sparsification/README.md) | moins d'additions, rang fixé | un schéma ⟨3,3,3⟩ de rang 23 de **221 non-nuls à 128**, minimum sur tout changement de base, chaque coefficient laissé à 0 ou ±1 |

L'infrastructure partagée et la documentation :

| Chemin | Contenu |
|---|---|
| [`core/linear_algebra/`](core/linear_algebra/README.md), [`core/formats/`](core/formats/README.md) | l'arithmétique exacte ; les fichiers tensor, SMS, DIMACS et SMT-LIB |
| [`infrastructure/cli/`](infrastructure/cli/README.md), [`infrastructure/run_limits/`](infrastructure/run_limits/README.md), [`methods/bilinear_rank/search_plan/`](methods/bilinear_rank/search_plan/README.md) | la grammaire de commande et les codes de sortie partagés ; ce qu'un calcul peut prendre à la machine ; les choix qu'un calcul écrit et rejoue |
| [`methods/bilinear_rank/map_construction/`](methods/bilinear_rank/map_construction/README.md), [`evidence/fixtures/`](evidence/fixtures/README.md), [`evidence/benchmark_tensors/`](evidence/benchmark_tensors/README.md) | construire les applications ; les jeux d'essai de tout le reste ; où chaque recherche s'arrête sur les tenseurs dont la littérature débat |
| [`infrastructure/gpu_leaf/`](infrastructure/gpu_leaf/README.md), [`methods/curve_bounds/`](methods/curve_bounds/README.md), [`methods/bilinear_rank/flip_graph/`](methods/bilinear_rank/flip_graph/README.md), [`methods/rank_metric_bound/`](methods/rank_metric_bound/README.md), [`methods/integer_programme/`](methods/integer_programme/README.md) | une carte grand public tarifée sur le test de feuille ; des bornes par courbes algébriques ; une marche qui déplace des schémas ; deux bornes inférieures sans recherche ; la couche LP et ILP |
| [`evidence/reproduce/`](evidence/reproduce/README.md), [`infrastructure/testing/`](infrastructure/testing/README.md), [`infrastructure/tools/`](infrastructure/tools/README.md) | chaque compte publié re-dérivé en CI ; l'assertion partagée des tests ; le script de comparaison des moteurs |
| [`web_interface/`](web_interface/README.md) | les outils pilotés depuis un navigateur, sur la seule bibliothèque standard de Python |
| [`start-here.md`](start-here.md) | une première session en mots simples, pour un lecteur sans le vocabulaire du domaine (en anglais) |
| [`what-is-where.md`](what-is-where.md), [`OPTIONS.md`](OPTIONS.md), [`references.md`](references.md) | la carte raisonnée ; chaque option avec la mesure derrière sa valeur par défaut ; la bibliographie, citée par clé depuis le code |
| [`writeup/article/`](writeup/article/README.md), [`writeup/positioning/`](writeup/positioning/README.md), [`writeup/the-research-front/`](writeup/the-research-front/README.md) | la rédaction avec définitions, théorèmes et résultats négatifs ; ce que cette bibliothèque apporte ; où en est le domaine |

Pourquoi treize outils en ligne de commande plutôt que huit, et la question
que chacun est seul à traiter :
[`OPTIONS/one-question-per-command.md`](OPTIONS/one-question-per-command.md).

## Méthodes

La descente est une **heuristique** gloutonne sur matroïde : exacte pour la
base que choisit sa première étape, en temps polynomial de bout en bout, et
sans garantie d'optimalité au-delà, ce que sa note de correction énonce
précisément. La recherche exhaustive est une **procédure de décision
complète** d'après [`[bdez2012]`](references.md), exponentielle comme la
NP-complétude du problème le laisse attendre ; l'axe sans isomorphes engendre
un candidat par classe d'équivalence au sens de
[`[mckay1998]`](references.md). L'axe satisfiabilité réduit la décision du
rang à SAT, son encodage binaire reprenant l'idée de
[`[heule2021]`](references.md) ; une réponse négative y est une réfutation
DRAT vérifiée par un programme indépendant. L'axe creusement prouve ses
minima : la réduction des opérateurs publiés `Grey-221` à 128 non-nuls est un
minimum sur tout changement de base inversible, pas un meilleur effort.

**La feuille est l'endroit où vit la recherche exhaustive**, et aucune de ses
deux routes n'y forme plus d'élément : le parcours avance en code de Gray
réfléchi sur GF(2) comme sur GF(p), **2,52x par élément sur GF(3)**, le terme
en dimension ayant disparu plutôt que diminué, et le balayage du vivier
transporte un résidu. Mêmes verdicts, mêmes comptes de nœuds, et une carte
grand public tarifée contre les deux : [`infrastructure/gpu_leaf/`](infrastructure/gpu_leaf/README.md).

**Un résultat négatif sur l'étape coûteuse.** L'étape 3 de l'heuristique de
descente énumère le vivier complet des applications de rang un. Sur les
quatre jeux d'essai polynomiaux elle a amélioré le résultat dans **deux cas
sur quatre**, d'un produit chaque fois, pour un coût supérieur d'**un à deux
ordres de grandeur** à celui des deux premières étapes réunies. Toute suite
qui se contenterait d'accélérer l'étape 3 optimiserait la partie qui, le plus
souvent, ne paie pas ; [`evidence/fixtures/README.md`](evidence/fixtures/README.md) existe pour
maintenir ce constat.

## Une seule chaîne

La recherche de rang reconstruit les opérateurs ⟨L, R, P⟩ ; le creusement est
ce à quoi ils servent.

```sh
minimise-rank evidence/fixtures/f2_5x5.tensor --emit-operators out   # 25 -> 14 multiplications
sparsify-operator out_L.sms                                 # 31 -> 27 coefficients non nuls
```

La console dans le navigateur enchaîne ces deux lignes dans cet ordre, une
fois par opérateur, comme un seul parcours :
[`web_interface/`](web_interface/README.md).

## Échanges avec la littérature publiée

Un triplet ⟨L, R, P⟩ au format SMS est ce sous quoi le domaine publie un
algorithme bilinéaire, et à peu près la seule chose qu'il publie : c'est donc
la porte d'entrée autant que la porte de sortie. Deux sources en distribuent
en quantité : le [catalogue FMM](https://fmm.univ-lille.fr/), des milliers de
décompositions classées par rang, et
[PLinOpt](https://github.com/jgdumas/plinopt), une bibliothèque C++ pour les
programmes linéaires et bilinéaires en ligne droite, dont le `data/` livre
Strassen, Winograd, Karatsuba, Toom-3 et la multiplication matricielle
jusqu'à 32x32x32.

En lire un est un test et non une affirmation : un triplet de Strassen publié
ailleurs reconstruit, entrée par entrée, le jeu d'essai que ce dépôt écrit à
partir de la définition de l'application, et un désaccord serait à nous de
l'expliquer. **Rien de tout cela n'est une dépendance** : rien ici ne se lie
à aucun de ces outils, et la suite entière passe sur une machine où aucun
n'est installé.

```sh
operators-to-tensor L.sms R.sms P.sms -q 2 > map.tensor     # un algorithme publié, lu ici
PMchecker out_L.sms out_R.sms out_P.sms -q 2                # le nôtre, vérifié ailleurs
```

Les deux directions et les différences qui mordent, sur une seule page :
[`core/formats/interchange/exchanging-files.md`](core/formats/interchange/exchanging-files.md).

## Deux branches

`main` est ce qui a gagné. **`rejected-experiments` est ce qui a perdu,
conservé entier** : la mesure qui a tranché chaque rejet et l'implémentation
qu'elle a retirée, car un rejet dont on a effacé les preuves ne se distingue
plus d'un caprice. On y trouve le parcours d'orbite que l'image canonique a
remplacé, le quotient par défaut qu'une recherche qui aboutit paie 7,4x, les
deux oracles exacts de creusement de `[beniamini2020]` avec l'heuristique de
base de lignes, et `find-at-rank` avec sa descente. Rien n'y est cassé et
rien n'y est maintenu. L'index de tout cela, avec le nombre qui a retiré
chacun :
[`retired/README.md`](https://github.com/Tewf/tensor-rank-toolkit/blob/rejected-experiments/retired/README.md).

## Compilation

Il faut un compilateur C++20, CMake ≥ 3.22 avec `pkg-config`, **Givaro** et
les en-têtes de **Boost** :

```sh
sudo apt install cmake ninja-build pkg-config libgivaro-dev libgmp-dev libboost-dev
```

Givaro et Boost sont les seules bibliothèques liées. Boost n'est nécessaire
qu'à [`vendor/permlib/`](vendor/permlib/README.md), pour `boost::next` et
`boost::shared_ptr`, et aucun en-tête hors de cette bibliothèque embarquée ne
l'inclut. `libgmp-dev` est sur la ligne parce que `libgivaro-dev` apporte
l'exécutable de GMP mais pas `gmpxx.h`, que les en-têtes de Givaro incluent ;
le [`Containerfile`](Containerfile) l'a découvert en échouant à compiler.
Tous les solveurs sont optionnels et cherchés sur le `PATH` à l'exécution.
`ccache` est utilisé quand il est installé et ignoré quand il ne l'est pas,
et [`Containerfile`](Containerfile) fixe un environnement pour reproduire un
nombre publié.

```sh
cmake -B build -G Ninja && cmake --build build
ctest --test-dir build            # tout, environ deux minutes
ctest --test-dir build -LE slow   # sans les recherches coûteuses
cmake --install build --prefix ~/.local   # les treize outils, sur le PATH
```

**Chaque ligne de commande documentée écrit son outil nu**, `minimise-rank …`,
ce qui suppose l'installation ci-dessus. Sans elle, les mêmes binaires se
trouvent sous le module qui possède chacun,
`build/methods/bilinear_rank/greedy_heuristic/minimise-rank` et ainsi de suite, et les lignes
s'exécutent avec ce préfixe. Les trois instruments et la coquille
`list-solvers` ne s'installent volontairement pas ; le `CMakeLists.txt`
racine dit pourquoi. Un lecteur nouveau dans le domaine commence par
[`start-here.md`](start-here.md).

Ajouter `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` et lier le résultat à la racine
de l'arbre (`ln -sf build/compile_commands.json .`) donne à clangd, et à tout
éditeur ou agent qui lui parle, les vraies options de compilation. Sans cela,
les en-têtes de chaque module paraissent manquants, car chacun possède son
propre répertoire d'inclusion.

## Citation

[`CITATION.cff`](CITATION.cff). Licence : MIT, voir [`LICENSE`](LICENSE) et
[`NOTICE`](NOTICE) pour la portée et pour le crédit du matériel qui n'est pas
de moi.

## Références

Chaque résultat implémenté ici est cité dans le code par une clé de
[`references.md`](references.md), qui tient la bibliographie annotée
complète. Les résultats de complexité qui encadrent l'entreprise : J. Håstad,
*Tensor rank is NP-complete*, J. Algorithms 11 (1990),
[`[hastad1990]`](references.md) ; M. Schaefer et D. Štefankovič, *The
complexity of tensor rank*, Theory Comput. Syst. 62 (2018),
[`[schaefer2018]`](references.md) ; C. J. Hillar et L.-H. Lim, *Most tensor
problems are NP-hard*, J. ACM 60 (2013), [`[hillar2013]`](references.md). La
recherche exhaustive implémente [`[bdez2012]`](references.md), la génération
sans isomorphes suit [`[mckay1998]`](references.md), et l'encodage SAT
binaire reprend l'idée de [`[heule2021]`](references.md).
