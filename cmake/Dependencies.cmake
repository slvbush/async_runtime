include(FetchContent)

message(STATUS "Fetching GTest...")
FetchContent_Declare(
    GTest
    URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
    SYSTEM
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(GTest)
