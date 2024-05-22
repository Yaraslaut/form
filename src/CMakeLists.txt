

add_executable(test test.cpp)
add_executable(example main.cpp)

target_link_libraries(test form)
target_link_libraries(example form)
