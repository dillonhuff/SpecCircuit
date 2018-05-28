cmake_minimum_required(VERSION 2.6)

project(FlatCircuit)

SET(EXTRA_CXX_COMPILE_FLAGS "-std=c++11 -I./src -I./test -I/opt/local/include -O2 -Werror -Wall -fPIC")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_COMPILE_FLAGS}")

SET(CXX_OCL_LINK_FLAGS "-L/Users/dillon/CppWorkspace/coreir/lib/ -Lrelease/lib")
SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${CXX_OCL_LINK_FLAGS}")


INCLUDE_DIRECTORIES(./src/)

SET(CPP_FILES ./src/simulator.cpp
              ./src/convert_coreir.cpp
              ./src/circuit.cpp
              ./src/analysis.cpp
              ./src/output_verilog.cpp
              ./src/transformations.cpp
              ./src/cpp_utils.cpp
              ./src/ir.cpp
              ./src/value_store.cpp)

SET(TEST_FILES ./test/test_circuit.cpp
               ./test/utils.cpp)

add_executable(all-tests ${TEST_FILES} ${CPP_FILES})
target_link_libraries(all-tests dl)
target_link_libraries(all-tests coreir)
target_link_libraries(all-tests coreir-rtlil)
target_link_libraries(all-tests coreir-commonlib)
