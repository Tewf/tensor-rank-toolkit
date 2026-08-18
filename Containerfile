# A reproduction image, for Podman.
#
#     podman build -t tensor-rank-toolkit .
#     podman run --rm tensor-rank-toolkit
#
# It exists so that a number published here can be reproduced on a machine that
# is not this one. Every count this repository reports is a fact about the
# problem rather than about the hardware, so it must come out the same
# everywhere; the image pins the things that could make it come out differently,
# which are the distribution, Givaro, and the two solvers built from source.
#
# It does NOT reproduce a timing, and no timing measured inside it is comparable
# to a published one. Those were measured on one core of a bare-metal 12th Gen
# Intel Core i5-12450H at 2.2 GHz, fastest of three runs on a quiet machine; a
# container shares the host's cores, caches, memory bandwidth and thermal budget
# with whatever else that host is doing, and the CI runner it is likely to run on
# is a shared virtual machine. Use it to check the counts. The protocol for the
# timings, and why they are not asserted anywhere, is MEASURING.md.

FROM ubuntu:24.04

# Kissat and drat-trim are not in the Ubuntu archive, so CI clones the default
# branch at --depth 1 and a failure there is upstream moving. That is the right
# trade for CI and the wrong one here: an image whose point is reproducing a
# published number cannot have its solver change under it, so the commit is named
# rather than the branch.
#
# The kissat commit is tag rel-4.0.4, which is the version every solver timing in
# this repository was taken with; satisfiability/results.json names it. Change it
# and the numbers in that file stop being the ones this image reproduces.
ARG KISSAT_COMMIT=8af8e56f174b778aef3aa45af9f739b2a5f492c2
ARG DRAT_TRIM_COMMIT=2e3b2dc0ecf938addbd779d42877b6ed69d9a985

# Givaro is the only build dependency. The three solvers are installed for the
# same reason CI installs them: without one on PATH the steps that use it take
# their own "skipping" branch, which is a green tick over an untested strand.
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build pkg-config libgivaro-dev \
      cryptominisat coinor-cbc glpk-utils git python3 \
 && rm -rf /var/lib/apt/lists/*

# Kissat writes the DRAT refutation and drat-trim checks it. A refutation nobody
# checks leaves a lower bound resting on the solver's word.
RUN git clone https://github.com/arminbiere/kissat /tmp/kissat \
 && git -C /tmp/kissat checkout "${KISSAT_COMMIT}" \
 && cd /tmp/kissat && ./configure && make -j"$(nproc)" \
 && install build/kissat /usr/local/bin/ \
 && rm -rf /tmp/kissat

RUN git clone https://github.com/marijnheule/drat-trim /tmp/drat-trim \
 && git -C /tmp/drat-trim checkout "${DRAT_TRIM_COMMIT}" \
 && make -C /tmp/drat-trim drat-trim \
 && install /tmp/drat-trim/drat-trim /usr/local/bin/ \
 && rm -rf /tmp/drat-trim

RUN kissat --version && command -v drat-trim

WORKDIR /src
COPY . /src

# The host's build/ comes in with COPY and is discarded: its CMakeCache.txt names
# paths that do not exist in here, and configuring on top of it fails in a way
# that reads as a broken image rather than as a stale cache.
RUN rm -rf /src/build \
 && cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build

CMD ["ctest", "--test-dir", "build", "--output-on-failure"]
