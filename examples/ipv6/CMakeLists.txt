set(name fragmentation-ipv6)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libinternet-apps})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name fragmentation-ipv6-two-MTU)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libinternet-apps})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name icmpv6-redirect)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libinternet-apps})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name loose-routing-ipv6)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libinternet-apps})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name ping6)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libinternet-apps})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name radvd)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libinternet-apps})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name radvd-two-prefix)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libinternet-apps})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name test-ipv6)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libinternet})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name wsn-ping6)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblr-wpan} ${libinternet} ${libsixlowpan}
                      ${libmobility} ${libinternet-apps}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
