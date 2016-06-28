release32 = ["-DCMAKE_BUILD_TYPE=Release"]
debug32 = ["-DCMAKE_BUILD_TYPE=Debug"]
release32nolp = ["-DCMAKE_BUILD_TYPE=Release", "-DUSE_LP=NO"]
debug32nolp = ["-DCMAKE_BUILD_TYPE=Debug", "-DUSE_LP=NO"]
release64 = ["-DCMAKE_BUILD_TYPE=Release", "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'"]
debug64 = ["-DCMAKE_BUILD_TYPE=Debug",   "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'"]
minimal = ["-DCMAKE_BUILD_TYPE=Release", "-DDISABLE_PLUGINS_BY_DEFAULT=YES"]

DEFAULT = "release32"
DEBUG = "debug32"
