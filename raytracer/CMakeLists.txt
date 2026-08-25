function(add_target NAME FILE)
  add_catch(${NAME} ${FILE})

  add_raytracer_lib(${NAME} raytracer-geom "../raytracer-geom")
  add_raytracer_lib(${NAME} raytracer-reader "../raytracer-reader")
  add_raytracer_lib(${NAME} raytracer ".")

  target_link_libraries(${NAME} PRIVATE ${PNG_LIBRARY})
  target_include_directories(${NAME} PRIVATE ${PNG_INCLUDE_DIRS})
endfunction()

add_target(test_raytracer_asan test_asan.cc)
add_target(test_raytracer_release test_release.cc)
