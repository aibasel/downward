release = ["-DCMAKE_BUILD_TYPE=Release"]
debug = ["-DCMAKE_BUILD_TYPE=Debug"]
release_no_lp = ["-DCMAKE_BUILD_TYPE=Release", "-DUSE_LP=NO"]
# USE_GLIBCXX_DEBUG is not compatible with USE_LP (see issue983).
glibcxx_debug = ["-DCMAKE_BUILD_TYPE=Debug", "-DUSE_LP=NO", "-DUSE_GLIBCXX_DEBUG=YES"]
minimal = ["-DCMAKE_BUILD_TYPE=Release", "-DDISABLE_LIBRARIES_BY_DEFAULT=YES"]
prototype = ["-DCMAKE_BUILD_TYPE=Release",
             "-DDISABLE_LIBRARIES_BY_DEFAULT=YES",
             "-DPLUGIN_CORE_TASKS_ENABLED=YES",
             "-DPLUGIN_PLUGIN_EAGER_ENABLED=YES",
             "-DPLUGIN_PLUGIN_ASTAR_ENABLED=YES",
             "-DPLUGIN_LANDMARK_CUT_HEURISTIC_ENABLED=YES"]

DEFAULT = "prototype"
DEBUG = "debug"
