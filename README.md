#  Terracraft Valrising

 Terracraft Valrising bot simulation

## How to build


1. Copy the repository
2. Go to the source tree directory (the one containing `CMakeLists.txt` file)
3. Generate a project buildsystem, then build the project through terminal. On linux just use

   `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build ./build`

   command.
   
5. Wait till the building is finished.

## How to test

1. In terminal use `ctest --test-dir build/tests` command.
2. See the results of the test.

## How to run

1. In the same directory use `build/main <directory with input data>`.
2. Enjoy the program.