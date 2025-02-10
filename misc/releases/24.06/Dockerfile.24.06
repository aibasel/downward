# This file has been automatically generated.

# The recipe below implements a Docker multi-stage build:
# <https://docs.docker.com/develop/develop-images/multistage-build/>

###############################################################################
# A first image to build the planner
###############################################################################
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install --no-install-recommends -y \
    ca-certificates \
    cmake           \
    g++             \
    git             \
    libgmp3-dev     \
    make            \
    python3         \
    zlib1g-dev

# Set up some environment variables.
ENV CXX g++
ENV SOPLEX_REVISION release-710
ENV soplex_DIR /opt/soplex

# Install SoPlex.
WORKDIR /workspace/soplex
RUN git clone --depth 1 --branch $SOPLEX_REVISION https://github.com/scipopt/soplex.git . && \
    cmake -DCMAKE_INSTALL_PREFIX="$soplex_DIR" -S . -B build && \
    cmake --build build && \
    cmake --install build

# Install Fast Downward.
WORKDIR /workspace/downward/
RUN git clone --depth 1 --branch release-24.06.0 https://github.com/aibasel/downward.git . && \
    ./build.py release debug && \
    strip --strip-all builds/release/bin/downward


###############################################################################
# The final image to run the planner
###############################################################################
FROM ubuntu:24.04 AS runner

RUN apt-get update && apt-get install --no-install-recommends -y \
    python3  \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace/downward/

# Copy the relevant files from the previous docker build into this build.
COPY --from=builder /workspace/downward/fast-downward.py .
COPY --from=builder /workspace/downward/builds/release/bin/ ./builds/release/bin/
COPY --from=builder /workspace/downward/builds/debug/bin/ ./builds/debug/bin/
COPY --from=builder /workspace/downward/driver ./driver
COPY --from=builder /opt/soplex /opt/soplex

ENV soplex_DIR=/opt/soplex
ENV LD_LIBRARY_PATH=$soplex_DIR/lib

ENTRYPOINT ["/workspace/downward/fast-downward.py"]
