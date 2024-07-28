# CMakeLists.txt
cmake_minimum_required(VERSION 3.0)

project(CPFF)  # Set your project name here
enable_language(C)

# Specify the path to NetCDF include directory
include_directories("C:/netCDF4.9.2/include")

# Specify the path to NetCDF libraries directory
link_directories("C:/netCDF4.9.2/lib")


# Add include directories
include_directories(${NetCDF_INCLUDE_DIR})

# Add your source files here
set(Flood_hydrograph
    main.c
    Func_DataIO.c
    Func_FloodHydrograph.c
    Func_datetime.c
)

# Add the executable target
add_executable(Flood_hydrograph ${Flood_hydrograph})


target_link_libraries(Flood_hydrograph PRIVATE netcdf)

## cmake -G "MinGW Makefiles" .
## mingw32-make
