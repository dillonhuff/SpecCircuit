#include "cpp_utils.h"

using namespace std;

#include <iostream>

namespace FlatCircuit {

  void compileCppLib(const std::string& cppName,
                     const std::string& targetBinary) {

#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    assert(false);
#ifdef _WIN64
    //define something for Windows (64-bit only)
    assert(false);
#else
    //define something for Windows (32-bit only)
    assert(false);
#endif
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_IPHONE_SIMULATOR
    // iOS Simulator
    assert(false);
#elif TARGET_OS_IPHONE
    // iOS device
    assert(false);
#elif TARGET_OS_MAC
    // Other kinds of Mac OS
    string compileCommand =
      "g++ -O3 -std=c++11 -fPIC -dynamiclib -Ijitted_utils/ " +
      cppName + " -o " + targetBinary;

#else
#   error "Unknown Apple platform"
    assert(false);
#endif
#elif __linux__
    // linux
    string compileCommand =
      "g++ -std=c++11 -fPIC -rdynamic -shared -Ijitted_utils/ " +
      cppName + " -o " + targetBinary;

#elif __unix__ // all unices not caught above
    // Unix
    string compileCommand =
      "g++ -std=c++11 -fPIC -rdynamic -shared -Ijitted_utils/ " +
      cppName + " -o " + targetBinary;

#elif defined(_POSIX_VERSION)
    // POSIX
    string compileCommand =
      "g++ -std=c++11 -fPIC -rdynamic -shared -Ijitted_utils/ " +
      cppName + " -o " + targetBinary;

#else
#   error "Unknown compiler"
    assert(false);
#endif

    cout << "Compile command = " << compileCommand << endl;

    int ret =
      //system(("clang++ -std=c++11 -O3 -fPIC -dynamiclib -I/Users/dillon/CppWorkspace/bsim/src/ " + cppName + " -o " + targetBinary).c_str());
      //system(("g++ -std=c++11 -fPIC -dynamiclib -I/Users/dillon/CppWorkspace/bsim/src/ " + cppName + " -o " + targetBinary).c_str());
      system(compileCommand.c_str());

    assert(ret == 0);
  }

  std::string cppLibName(const std::string& baseName) {

#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    assert(false);
#ifdef _WIN64
    //define something for Windows (64-bit only)
    assert(false);
#else
    //define something for Windows (32-bit only)
    assert(false);
#endif
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_IPHONE_SIMULATOR
    // iOS Simulator
    assert(false);
#elif TARGET_OS_IPHONE
    // iOS device
    assert(false);
#elif TARGET_OS_MAC
    // Other kinds of Mac OS
    
    return "./lib" + baseName + ".dylib";    

#else
#   error "Unknown Apple platform"
    assert(false);
#endif
#elif __linux__
    // linux
    return "./lib" + baseName + ".so";
#elif __unix__ // all unices not caught above
    // Unix
    return "./lib" + baseName + ".so";
#elif defined(_POSIX_VERSION)
    // POSIX
    return "./lib" + baseName + ".so";
#else
#   error "Unknown compiler"
    assert(false);
#endif

  }
}
