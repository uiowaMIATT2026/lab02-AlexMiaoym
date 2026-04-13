set(createimage_SOURCE CreateImage.cxx)

add_executable(createimage ${createimage_SOURCE})
target_link_libraries(createimage  ${ITK_LIBRARIES})