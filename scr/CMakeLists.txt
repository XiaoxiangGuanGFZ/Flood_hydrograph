# CMakeLists.txt
cmake_minimum_required(VERSION 3.0)

project(Flood_hydrograph)  # Set your project name here
enable_language(C)

# Add your source files here
set(Flood_hydrograph
    main.c
    Func_DataIO.c
    Func_FloodHydrograph.c
    Func_datetime.c
)

# Add the executable target
add_executable(Flood_hydrograph ${Flood_hydrograph})

## cmake -G "MinGW Makefiles" .
## mingw32-make
