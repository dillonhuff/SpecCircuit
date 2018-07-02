cmake_minimum_required(VERSION 2.6)

project(FlatCircuit)

SET(EXTRA_CXX_COMPILE_FLAGS "-std=c++11 -I./src -I./test -I./benchmarks -I/opt/local/include -O2 -Werror -Wall -fPIC")
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
              ./src/utils.cpp
              ./src/value_store.cpp)

add_library(flatcircuit ${CPP_FILES})

# Test executables
SET(CGRA_TEST_FILES ./test/cgra_tests.cpp)
SET(SMALL_TEST_FILES ./test/test_circuit.cpp ./test/op_tests.cpp)
SET(REGRESSION_TEST_FILES ./test/op_tests.cpp)

add_executable(all-tests test/test_main.cpp ${SMALL_TEST_FILES} ${CGRA_TEST_FILES})
target_link_libraries(all-tests flatcircuit dl coreir coreir-rtlil coreir-commonlib)

add_executable(small-tests test/test_main.cpp ${SMALL_TEST_FILES})
target_link_libraries(small-tests flatcircuit dl coreir coreir-rtlil coreir-commonlib)

add_executable(regression-tests test/test_main.cpp ${REGRESSION_TEST_FILES})
target_link_libraries(regression-tests flatcircuit dl coreir coreir-rtlil coreir-commonlib)

# End test executables

SET(BENCHMARK_FILES ./benchmarks/main.cpp)
add_executable(run-benchmarks ${BENCHMARK_FILES})
target_link_libraries(run-benchmarks flatcircuit dl coreir coreir-rtlil coreir-commonlib)

add_executable(specialize-bitstream ./benchmarks/specialize_bitstream.cpp)
target_link_libraries(specialize-bitstream flatcircuit dl coreir coreir-rtlil coreir-commonlib)
