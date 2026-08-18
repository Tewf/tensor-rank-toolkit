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

Le problème est ici attaqué dans quatre directions : moins de multiplications,
moins d'additions, le rang posé comme une question à laquelle un solveur SAT
sait répondre et, là où aucune recherche ne parvient, une borne lue sur une
courbe algébrique. **Rien ici n'est jamais un flottant** : un rang annoncé est donc un
fait sur l'application et non un artefact d'arrondi.

Chaque compte ci-dessous est vérifié par la suite de tests et se reproduit
partout. Les temps de calcul, eux, non, et rien ne le prétend : voir
[`MEASURING.md`](MEASURING.md) pour le protocole et [`reproduce/`](reproduce/)
pour le pilote qui les régénère.

## Ce qu'il calcule

**[Le rang par descente](descent_search/)**, la direction bon marché. Trois
étapes : un glouton matroïdal exact pour la base de départ, puis deux
relaxations qui échangent la garantie contre la portée.

| Application | Naïf | Atteint | Rang publié |
|---|---|---|---|
| F2 5×5 | 25 | **14** | 13, `[bdez2012]` |
| F2 3×8 | 24 | **15** | aucune solution à 14 |
| F2 4×7 | 28 | **16** | aucune solution à 14 |
| F3 3×6 | 18 | **10** | 10, `[bdez2012]` |

**[Le rang par exhaustion](exhaustive_search/)**, la direction coûteuse, celle
qui prouve. Elle tranche d'emblée les petites applications : elle retrouve le 3
de Karatsuba, les 3 et 6 classiques pour GF(4) et GF(8), et **rang ⟨2,2,2⟩ = 7**
décidé à partir du tenseur en une demi-seconde. Sur F2 5×5, elle exclut de façon
exhaustive 9, 10 et 11 produits, ce qui, joint au 14 de la descente, prouve ici
**12 ≤ rang ≤ 14** ; `[bdez2012]` annoncent 13. Sur F3 3×6, les deux bornes sont
prouvées en environ 25 secondes.

**[Des bornes inférieures sans recherche](linear_algebra/tensor_rank_sum.h).**
Deux bornes par somme de rangs renvoient un plancher à partir du seul tenseur,
en quelques millisecondes, et elles sont serrées assez souvent pour supprimer
entièrement la question la plus chère d'un balayage : elles font passer GF(16)
de 4 à **8** et la convolution cyclique de 5 à **9**, chacune coûtant auparavant
une minute d'exhaustion.

**[Le creusement des opérateurs](matrix_sparsification/)**, qui est l'autre
moitié du coût. Les opérateurs d'encodage de Strassen passent de **12
coefficients non nuls à 10**, et un opérateur en base alternative de **21 à
10**, en quelques millisecondes. Moins de coefficients non nuls signifie moins
d'additions, c'est-à-dire le coût que le compte de multiplications ne capture
pas.

**[La question du rang comme satisfiabilité](satisfiability/).** Håstad a prouvé
que décider le rang d'un tenseur est NP-complet sur tout corps fini, et cela
vaut dans les deux sens : `formula_to_tensor` transforme une instance 3SAT en
tenseur, et trois encodages transforment la question du rang en une question à
laquelle un solveur répond. Une réfutation peut être écrite en DRAT et vérifiée
par `drat-trim` : une borne inférieure issue d'un solveur se vérifie donc au
lieu d'être crue sur parole.

**[Le quotient par la symétrie](orbit_reduction/).** Un changement de
coordonnées qui fixe le sous-espace cible envoie les solutions sur des
solutions, donc un représentant par orbite suffit : **28× sur une réfutation**,
et le vivier de candidats ⟨3,3,3⟩ s'effondre de 261 121 à **13 orbites**.

**[L'énumération sans isomorphes](oracle_guided_search/).**
`enumerate-subspaces --canonical` est l'augmentation canonique de
`[mckay1998]`, qui déduplique sans aucune mémoire. Elle rend les 36 sous-espaces
solutions de ⟨2,2,2⟩ pour ce qu'ils sont, **1 orbite**, en visitant **1982×
moins de nœuds**. Le temps de paroi ne s'améliore que de 1,6×, parce que trouver
un code canonique en parcourant le groupe entier dépense sur lui-même l'essentiel
de l'économie, et cela est mesuré plutôt qu'escamoté
([`deduplication-cost.md`](oracle_guided_search/deduplication-cost.md)).

## Le résultat qui vaut d'être énoncé à part

**L'étape coûteuse n'apporte presque rien.** Sur les quatre jeux de test
polynomiaux, l'étape 3 de la descente, celle qui énumère le vivier complet des
applications de rang un, n'a amélioré le résultat que dans **un cas sur
quatre**, et concentre l'essentiel du coût. Toute suite qui se contenterait
d'accélérer l'étape 3 optimiserait la partie qui, le plus souvent, ne paie pas.
Les jeux de test existent pour maintenir ce constat :
[`fixtures/README.md`](fixtures/README.md).

## Une seule chaîne

La recherche de rang reconstruit les opérateurs d'encodage ⟨L, R, P⟩ à partir de
sa décomposition et les écrit ; le creusement est précisément ce à quoi ils
servent.

```sh
minimise-rank fixtures/f2_5x5.tensor --emit-operators out   # 25 -> 14 multiplications
sparsify-operator out_L.sms                                 # 31 -> 27 coefficients non nuls
```

## Ce qu'il y a, et où

```
linear_algebra/          arithmétique exacte sur GF(p) et sur Q, partagée par tout le reste
formats/                 fichiers tenseur, matrice dense, SMS, DIMACS et SMT-LIB
cli/                     ce que partagent toutes les commandes : horloge, codes de
                         sortie, grammaire des arguments, partage stdout/stderr,
                         paramètres réglables
tunables.conf            les bornes d'une exécution, dans un fichier et non dans le code
testing/                 l'aide aux assertions dont se servent les tests de chaque module
run_limits/              la mémoire et le nombre de cœurs qu'une exécution peut prendre
descent_search/          le rang par le haut, par descente
exhaustive_search/       le rang tranché d'emblée, et ce que cela coûte
map_construction/        la construction des applications sur lesquelles tournent les méthodes
orbit_reduction/         le quotient des trois recherches par la symétrie
flip_graph/              déplacer une décomposition de côté au lieu d'en construire une
oracle_guided_search/    recherche à k fixé, réfutation d'arbre, augmentation canonique
matrix_sparsification/   le moins de coefficients non nuls dans un opérateur
satisfiability/          la même question de rang posée à un solveur SAT ou SMT
curve_bounds/            des bornes par interpolation sur une courbe algébrique
integer_programme/       la couche programme linéaire et entier dont se sert le volet des courbes
fixtures/                les applications et opérateurs sur lesquels tout est exécuté
reproduce/               régénère chaque nombre publié, avec sa provenance
references.md            tout article cité ici, par les clés que le code emploie
state-of-the-art/        où en est le front de recherche, et quelles parties sont ici
positioning/             ce que cette bibliothèque y ajoute, et ce qu'elle n'y ajoute pas
MEASURING.md             comment une mesure de temps est prise ici, et ce qu'elle ne dit pas
```

Un dossier qui a quelque chose à dire porte son propre `README.md` ; celui qui
n'a rien à dire énonce sa raison d'être en tête de son `CMakeLists.txt`. Chaque
dossier de méthode contient le code, ses `tests/` et, lorsqu'il a un point
d'entrée, un `commands/`.

**Onze outils en ligne de commande.** Trois demandent combien de multiplications
une application exige et ne s'accordent pas sur ce qu'ils savent prouver :
`minimise-rank` (descente), `decide-rank` (complet), `walk-scheme` (une marche
qui se déplace de côté). `decide-rank-by-sat` pose cette question au solveur de
quelqu'un d'autre, et `list-solvers` dit de quels moteurs la machine dispose.
`find-at-rank`, `deflate-strictly` et `enumerate-subspaces` sont les voies à k
fixé, par réfutation d'arbre et sans isomorphes. `curve-bounds` répond à une
autre question : il borne le rang à partir des points d'une courbe au lieu de
chercher. Puis `sparsify-operator` pour l'autre volet, et `make-tensor` pour
construire une application sur laquelle lancer n'importe lequel des autres.

Tout article dont une partie de ceci est une mise en œuvre est nommé une seule
fois, dans [`references.md`](references.md), et le code en cite la clé.

## Compilation

Demande un compilateur C++20, CMake ≥ 3.22 et **Givaro** (`sudo apt install
libgivaro-dev`). Givaro est la seule dépendance de compilation ; chaque solveur
est facultatif et localisé dans le `PATH` à l'exécution.

```sh
cmake -B build -G Ninja && cmake --build build
ctest --test-dir build            # tout, environ deux minutes
ctest --test-dir build -LE slow   # sans les recherches coûteuses
```

`ccache` est utilisé automatiquement s'il est installé et ignoré sinon : il
raccourcit une recompilation sans devenir une seconde dépendance. Un
environnement figé pour reproduire un nombre publié se trouve dans
[`Containerfile`](Containerfile).

## Citation

[`CITATION.cff`](CITATION.cff). Licence : MIT, voir [`LICENSE`](LICENSE) et
[`NOTICE`](NOTICE) pour la portée et pour le crédit du matériel qui n'est pas de
moi.
