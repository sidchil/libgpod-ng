add_executable(roundtrip roundtrip.c)
target_link_libraries(roundtrip PRIVATE gpod)
add_test(NAME roundtrip COMMAND roundtrip)
