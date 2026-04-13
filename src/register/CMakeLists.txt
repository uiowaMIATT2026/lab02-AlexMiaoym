set(register_SOURCE ImageRegistration.cxx)

add_executable(register ${register_SOURCE})
target_link_libraries(register ${ITK_LIBRARIES})