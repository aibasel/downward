# This file has been automatically generated.

# The recipe below implements a Docker multi-stage build:
# <https://docs.docker.com/develop/develop-images/multistage-build/>

###############################################################################
# A first image to build the planner
###############################################################################
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install --no-install-recommends -y \
    ca-certificates \
    cmake           \
    g++             \
    git             \
    libgmp3-dev     \
    make            \
    python3         \
    wget            \
    zlib1g-dev

# Set up some environment variables.
ENV CXX g++
ENV SOPLEX_VERSION soplex-3.1.1
ENV DOWNWARD_SOPLEX_INSTALLER $SOPLEX_VERSION.tgz
ENV DOWNWARD_SOPLEX_ROOT /opt/soplex
ENV OSI_VERSION Osi-0.107.9
ENV DOWNWARD_COIN_ROOT /opt/osi

# Install SoPlex.
WORKDIR /workspace/soplex
ADD $SOPLEX_VERSION.tgz .
RUN mkdir -p $SOPLEX_VERSION/build && \
    cd $SOPLEX_VERSION/build && \
    cmake -DCMAKE_INSTALL_PREFIX="$DOWNWARD_SOPLEX_ROOT" .. && \
    make && \
    make install

# Install OSI with support for both CPLEX and SoPlex.
WORKDIR /workspace/osi/
RUN wget http://www.coin-or.org/download/source/Osi/$OSI_VERSION.tgz && \
    tar xvzf $OSI_VERSION.tgz && \
    cd $OSI_VERSION && \
    mkdir $DOWNWARD_COIN_ROOT && \
    ./configure CC="gcc"  CFLAGS="-pthread -Wno-long-long" \
                CXX="g++" CXXFLAGS="-pthread -Wno-long-long" \
                LDFLAGS="-L$DOWNWARD_SOPLEX_ROOT/lib" \
                --without-lapack --enable-static=no \
                --prefix="$DOWNWARD_COIN_ROOT" \
                --disable-bzlib \
                --with-soplex-incdir="$DOWNWARD_SOPLEX_ROOT/include" \
                --with-soplex-lib="-lsoplex"  && \
    make && \
    make install

# Install Fast Downward.
WORKDIR /workspace/downward/
RUN git clone --depth 1 --branch TAG https://github.com/aibasel/downward.git . && \
    ./build.py release debug && \
    strip --strip-all builds/release/bin/downward


###############################################################################
# The final image to run the planner
###############################################################################
FROM ubuntu:22.04 AS runner

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
COPY --from=builder /opt/osi /opt/osi

ENV DOWNWARD_SOPLEX_ROOT=/opt/soplex
ENV DOWNWARD_COIN_ROOT=/opt/osi
ENV LD_LIBRARY_PATH=$DOWNWARD_SOPLEX_ROOT/lib:$DOWNWARD_COIN_ROOT/lib

ENTRYPOINT ["/workspace/downward/fast-downward.py"]
