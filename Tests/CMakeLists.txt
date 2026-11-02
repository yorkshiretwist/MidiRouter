include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

# The routing engine is header-only and has no JUCE dependency, so the tests
# build and run without JUCE.
add_executable(MidiRouterTests RouterTests.cpp)
target_include_directories(MidiRouterTests PRIVATE ${CMAKE_SOURCE_DIR}/Source)
target_link_libraries(MidiRouterTests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(MidiRouterTests)
