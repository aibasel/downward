# Planning in the browser
- Tested with emscripten version 2.0.8

## Building the JS+WASM files:

- make sure the emscripten sdk is installed and executables (mainly em++ and emcmake) are on the PATH. 

- cmake needs to be installed 

- you can use the `./configure_cmake.sh` file to configure cmake for an emscripten or native build using `./configure_cmake.sh emscripten` or `./configure_cmake.sh native` respectively.

- use above mentioned to switch to emscripten build type, this executes `cmake` with custom parameters: `CMAKE_CXX_COMPILER=em++` and `EMSCRIPTEN=1`.

- build the WebAssembly and Javascript files using `make` in the `/src` directory, output files can be found in `src/bin`.

- as the `CMAKE_CXX_COMPILER` stays to be `em++`, you need to change it back in case you want to build the native version. The configuration script assums `g++` to be your default native compiler.

## Using the JS+WASM files:

- instead of using STDIN to pass the translated `output.sas` file to the program, we added the option to use the `--input` argument, which takes a file handle and treats it as input stream

- thus, before calling the search, but after the Module was loaded, we need to save the translated output file on the browsers virtual filesystem

- There are multiple ways to do this, an example however can be found under `usage_example.js` loaded at the end of `index.html`. The example assumes `output.sas`, `downward.js` as well as `downward.wasm` files in the working directory

- the `runFastDownward()` function executes the example. Make sure to have added the necessary files in beforehand.

## Debug

- you can use the included function `Module.getExceptionMessage(pointer)` to get a more detailed exception message in case you need it for debugging:
in the `downward.js` file, you need to replace the original exception logging by the result of `Module.getExceptionMessage(pointer)`. I.e. replace `err('exception thrown: ' + toLog);` by 
`err('exception thrown: ' + Module.getExceptionMessage(toLog));`
`