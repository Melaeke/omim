project(ugc)

set(
  SRC
  api.cpp
  api.hpp
  binary/header_v0.hpp
  binary/index_ugc.hpp
  binary/serdes.cpp
  binary/serdes.hpp
  binary/ugc_holder.hpp
  binary/visitors.hpp
  serdes.hpp
  serdes_json.hpp
  storage.cpp
  storage.hpp
  types.hpp
)

add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(ugc_tests)
