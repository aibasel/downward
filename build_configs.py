release32 = ["-DCMAKE_BUILD_TYPE=Release"]
debug32 = ["-DCMAKE_BUILD_TYPE=Debug"]
release32nolp = ["-DCMAKE_BUILD_TYPE=Release", "-DUSE_LP=NO"]
debug32nolp = ["-DCMAKE_BUILD_TYPE=Debug", "-DUSE_LP=NO"]
release64 = ["-DCMAKE_BUILD_TYPE=Release", "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'"]
debug64 = ["-DCMAKE_BUILD_TYPE=Debug",   "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'"]
release64nolp = ["-DCMAKE_BUILD_TYPE=Release", "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'", "-DUSE_LP=NO"]
debug64nolp = ["-DCMAKE_BUILD_TYPE=Debug",   "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'", "-DUSE_LP=NO"]
minimal = ["-DCMAKE_BUILD_TYPE=Release", "-DDISABLE_PLUGINS_BY_DEFAULT=YES"]

release32dynamic = ["-DCMAKE_BUILD_TYPE=Release", "-DFORCE_DYNAMIC_BUILD=YES"]
debug32dynamic = ["-DCMAKE_BUILD_TYPE=Debug", "-DFORCE_DYNAMIC_BUILD=YES"]
release64dynamic = ["-DCMAKE_BUILD_TYPE=Release", "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'", "-DFORCE_DYNAMIC_BUILD=YES"]
debug64dynamic = ["-DCMAKE_BUILD_TYPE=Debug",   "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'", "-DFORCE_DYNAMIC_BUILD=YES"]

DEFAULT = "release32"
DEBUG = "debug32"
