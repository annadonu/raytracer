add_catch(test_raytracer_reader test.cc)
add_raytracer_lib(test_raytracer_reader raytracer-geom "../raytracer-geom")
add_raytracer_lib(test_raytracer_reader raytracer-reader ".")
