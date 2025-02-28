include_guard(GLOBAL)

add_library(common_cxx_flags INTERFACE)
target_compile_features(common_cxx_flags INTERFACE cxx_std_20)

set(using_gcc_like "$<CXX_COMPILER_ID:ARMClang,AppleClang,Clang,GNU,LCC>")
set(using_gcc "$<CXX_COMPILER_ID:GNU>")
set(using_msvc "$<CXX_COMPILER_ID:MSVC>")
set(using_gcc_like_release "$<AND:${using_gcc_like},$<CONFIG:RELEASE>>")
set(using_gcc_like_debug "$<AND:${using_gcc_like},$<CONFIG:DEBUG>>")
set(should_use_glibcxx_debug "$<AND:${using_gcc_like_debug},$<BOOL:${USE_GLIBCXX_DEBUG}>>")

target_compile_options(common_cxx_flags INTERFACE
    "$<${using_gcc_like}:-O3;-g>")
target_link_options(common_cxx_flags INTERFACE
    "$<${using_gcc_like}:-g>")
target_compile_options(common_cxx_flags INTERFACE
    "$<${using_gcc_like_release}:-DNDEBUG;-fomit-frame-pointer>")
target_compile_definitions(common_cxx_flags INTERFACE
    "$<${should_use_glibcxx_debug}:_GLIBCXX_DEBUG>")
# Enable exceptions for MSVC.
target_compile_options(common_cxx_flags INTERFACE
    "$<${using_msvc}:/EHsc>")

add_library(common_cxx_warnings INTERFACE)
target_compile_options(common_cxx_warnings INTERFACE
    "$<${using_gcc_like}:-Wall;-Wextra;-Wpedantic;-Wnon-virtual-dtor;-Wfloat-conversion;-Wmissing-declarations;-Wzero-as-null-pointer-constant>")

# We ignore the warning "restrict" because of a bug in GCC 12:
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105651
set(v12_or_later "$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,12>")
set(before_v13 "$<VERSION_LESS:$<CXX_COMPILER_VERSION>,13>")
set(bugged_gcc "$<AND:${using_gcc},${v12_or_later},${before_v13}>")
target_compile_options(common_cxx_warnings INTERFACE
    "$<${bugged_gcc}:-Wno-restrict>")

# For MSVC, use warning level 4 (/W4) because /Wall currently detects too
# many warnings outside of our code to be useful.
target_compile_options(common_cxx_warnings INTERFACE
    "$<${using_msvc}:/W4;/wd4456;/wd4458;/wd4459;/wd4244;/wd4267>")
    # Disable warnings that currently trigger in the code until we fix them.
    #   /wd4456: declaration hides previous local declaration
    #   /wd4458: declaration hides class member
    #   /wd4459: declaration hides global declaration
    #   /wd4244: conversion with possible loss of data
    #   /wd4267: conversion from size_t to int with possible loss of data
target_link_libraries(common_cxx_flags INTERFACE common_cxx_warnings)

set(v15_or_later "$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,15>")
set(using_clang "$<CXX_COMPILER_ID:Clang>")
set(using_apple_clang "$<CXX_COMPILER_ID:AppleClang>")
set(using_clang_like "$<OR:${using_clang},${using_apple_clang}>")
set(should_ignore_std_warning "$<AND:${using_clang_like},${v15_or_later}>")
target_compile_options(common_cxx_warnings INTERFACE
    "$<${should_ignore_std_warning}:-Wno-unqualified-std-cast-call>")
