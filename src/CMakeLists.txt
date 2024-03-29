set(libs "${libs_to_build}")

# Process the visualizer module first if enabled
if(${ENABLE_VISUALIZER})
  message(STATUS "Processing src/visualizer")
  add_subdirectory(visualizer)
  list(REMOVE_ITEM libs visualizer)
endif()

# Process subdirectories
foreach(libname ${libs})
  if(EXISTS ${PROJECT_SOURCE_DIR}/src/${libname}/CMakeLists.txt)
    message(STATUS "Processing src/${libname}")
    add_subdirectory(${libname})
  else()
    message(
      STATUS
        "Skipping src/${libname} : it does not contain a CMakeLists.txt file"
    )
  endif()
endforeach()

# Build the lib-ns3-static (ns3.x-static-buildtype.a/.lib) with all sublibraries
if(${NS3_STATIC})
  add_library(
    ${lib-ns3-static} STATIC ${PROJECT_SOURCE_DIR}/buildsupport/empty.cc
                             "${lib-ns3-static-objs}"
  )

  # Replace shared library suffix and check if static version exists before
  # linking
  set(ns3-external-static-libs)
  foreach(sharedlib ${ns3-external-libs})
    if(NOT (${sharedlib} MATCHES ".so"))
      continue()
    endif()

    string(REPLACE ".so" ".a" output ${sharedlib})
    if(EXISTS ${output})
      list(APPEND ns3-external-static-libs ${output})
    else()
      message(
        FATAL_ERROR "Static library version of ${sharedlib} was not found"
      )
    endif()
  endforeach()

  target_link_libraries(
    ${lib-ns3-static}
    -static
    -static-libstdc++
    -static-libgcc
    ${LIB_AS_NEEDED_PRE_STATIC}
    ${ns3-external-static-libs}
    ${LIB_AS_NEEDED_POST_STATIC}
  )
  if(${NS3_CLANG_TIMETRACE})
    add_dependencies(timeTraceReport ${lib-ns3-static})
  endif()
endif()

# Build the lib-ns3-static (ns3.x-monolib-buildtype.dll/.dylib/.so) with all
# sublibraries
if(${NS3_MONOLIB})
  add_library(
    ${lib-ns3-monolib} SHARED ${PROJECT_SOURCE_DIR}/buildsupport/empty.cc
                              "${lib-ns3-static-objs}"
  )
  set_target_properties(
    ${lib-ns3-monolib} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE
  )
  target_link_libraries(
    ${lib-ns3-monolib} ${LIB_AS_NEEDED_PRE} ${ns3-external-libs}
    ${LIB_AS_NEEDED_POST}
  )
  if(${NS3_CLANG_TIMETRACE})
    add_dependencies(timeTraceReport ${lib-ns3-monolib})
  endif()
endif()
