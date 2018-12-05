release32 = ["-DCMAKE_BUILD_TYPE=Release"]
debug32 = ["-DCMAKE_BUILD_TYPE=Debug"]
release32nolp = ["-DCMAKE_BUILD_TYPE=Release", "-DUSE_LP=NO"]
debug32nolp = ["-DCMAKE_BUILD_TYPE=Debug", "-DUSE_LP=NO"]
release64 = ["-DCMAKE_BUILD_TYPE=Release", "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'"]
debug64 = ["-DCMAKE_BUILD_TYPE=Debug",   "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'"]
release64nolp = ["-DCMAKE_BUILD_TYPE=Release", "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'", "-DUSE_LP=NO"]
debug64nolp = ["-DCMAKE_BUILD_TYPE=Debug",   "-DALLOW_64_BIT=True", "-DCMAKE_CXX_FLAGS='-m64'", "-DUSE_LP=NO"]
minimal = ["-DCMAKE_BUILD_TYPE=Release", "-DDISABLE_PLUGINS_BY_DEFAULT=YES"]

releasenative = ["-DCMAKE_BUILD_TYPE=Release", "-DALLOW_64_BIT=True"]
debugnative = ["-DCMAKE_BUILD_TYPE=Debug",   "-DALLOW_64_BIT=True"]

DEFAULT = "release32"
DEBUG = "debug32"

def supports_32_bit_builds():
    # We are importing this locally to avoid having "platform" imported as the name of a build configuration.
    import platform

    if platform.system() == "Darwin":
        release = platform.release().split(".")
        try:
            major_version = int(release[0])
            return major_version < 18
        except (IndexError, ValueError):
            print("Cannot determine OS version but assuming it supports 32-bit builds.")
    return True

if not supports_32_bit_builds():
    print("macOS Mojave and later do not support 32-bit builds. "
        "Using 64-bit builds as default. See issue854 for details.")
    DEFAULT = "release64"
    DEBUG = "debug64"

del supports_32_bit_builds
