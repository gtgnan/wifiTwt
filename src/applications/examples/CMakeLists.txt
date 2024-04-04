build_lib_example(
  NAME three-gpp-http-example
  SOURCE_FILES three-gpp-http-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libpoint-to-point}
    ${libinternet}
    ${libnetwork}
)

build_lib_example(
  NAME video-example
  SOURCE_FILES video-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libpoint-to-point}
    ${libinternet}
    ${libnetwork}
    ${libflow-monitor}
)

build_lib_example(
  NAME voip-example
  SOURCE_FILES voip-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libpoint-to-point}
    ${libinternet}
    ${libnetwork}
    ${libflow-monitor}
)