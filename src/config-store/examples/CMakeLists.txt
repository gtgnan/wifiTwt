set(name config-store-save)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libconfig-store})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
