function(GetCompileFlags _compile_flags _target_name _use_cpp)
    if(_target_name STREQUAL nvim)
        set(_target_dir ${CMAKE_SOURCE_DIR}/source/${_target_name})
    else()
        # plugin target
        set(_target_dir ${CMAKE_SOURCE_DIR}/source/plugins/bin/${_target_name})
    endif()

    string(TOUPPER ${_target_name} tgt_name_upper)

    # Create template akin to
    # 'CMAKE_C_COMPILE_OBJECT' <= 'CMAKE_<LANG>_COMPILE_OBJECT'
    set(compile_flags "<DEFINITIONS> <INCLUDES> <CFLAGS>")

    # Get Macros set by add_definitions()
    set(out_msg false)
    set(code_msg false)
    get_directory_property(tgt_definitions
                           DIRECTORY
                           "${_target_dir}"
                           COMPILE_DEFINITIONS)

    foreach(defs_item ${tgt_definitions})
        if(code_msg)
            set(code_msg "${code_msg} -D${defs_item}")
        else()
            set(code_msg "-D${defs_item}")
        endif()

        if(out_msg)
            set(out_msg "${out_msg}\n                                        = ${defs_item}")
        else()
            set(out_msg "add_definitions(...)                 = ${defs_item}")
        endif()
    endforeach()
    message(STATUS "${out_msg}")
    string(REPLACE "<DEFINITIONS>" "${code_msg}" compile_flags "${compile_flags}")

    # Get include directories
    set(index  0)
    set(out_msg false)
    set(code_msg false)
    get_target_property(include_dirs ${_target_name} INCLUDE_DIRECTORIES)
    foreach(inc_dir ${include_dirs})
        string(FIND ${code_msg} ${inc_dir} inc_dir_${index})
        if(NOT inc_dir_${index} EQUAL -1)
            # just skip duplicated include directory,
            # do not show them, make message simple
            continue()
        endif()
        if(code_msg)
            set(code_msg "${code_msg} -I${inc_dir}")
        else()
            set(code_msg "-I${inc_dir}")
        endif()

        if(out_msg)
            set(out_msg "${out_msg}\n                                        = ${inc_dir}")
        else()
            set(out_msg "INCLUDE_DIRECTORIES                  = ${inc_dir}")
        endif()

        math(EXPR index "${index} + 1")
    endforeach()
    message(STATUS "${out_msg}")
    string(REPLACE "<INCLUDES>" "${code_msg}" compile_flags "${compile_flags}")

    # Get compile flags
    set(out_msg false)
    set(code_msg false)
    get_directory_property(tgt_flags
                           DIRECTORY
                           "${_target_dir}"
                           COMPILE_OPTIONS)

    if(_use_cpp)
        set(tgt_build_flags ${${tgt_name_upper}_CONFIG_CXX_FLAGS_${build_type}})
        set(tgt_compile_flags ${${tgt_name_upper}_CONFIG_CXX_FLAGS})
    else()
        set(tgt_build_flags ${${tgt_name_upper}_CONFIG_C_FLAGS_${build_type}})
        set(tgt_compile_flags ${${tgt_name_upper}_CONFIG_C_FLAGS})
    endif()

    separate_arguments(tgt_build_flags UNIX_COMMAND ${tgt_build_flags})
    separate_arguments(tgt_compile_flags UNIX_COMMAND ${tgt_compile_flags})

    foreach(flag_item ${tgt_flags} ${tgt_compile_flags} ${tgt_build_flags})
        if(code_msg)
            set(code_msg "${code_msg} ${flag_item}")
        else()
            set(code_msg "${flag_item}")
        endif()

        if(out_msg)
            set(out_msg "${out_msg}\n                                        = ${flag_item}")
        else()
            set(out_msg "Compile Options                      = ${flag_item}")
        endif()
    endforeach()
    message(STATUS "${out_msg}")
    string(REPLACE "<CFLAGS>" "${code_msg}" compile_flags "${compile_flags}")

    # Clean duplicate flags
    # only for hard code into the source file, make them looks better, and
    # the actually flags is not touched.
    list(APPEND compile_flags ${compile_flags})
    set(index  0)
    foreach(AA ${compile_flags})
        set(flag_index_${index}  0)

        foreach(BB ${compile_flags})
            if(AA STREQUAL BB)
                math(EXPR flag_index_${index} "${flag_index_${index}} + 1")
            endif()
        endforeach()

        math(EXPR index "${index} + 1")
    endforeach()

    set(index  0)
    foreach(AA ${compile_flags})
        if(flag_index_${index} EQUAL 1)
            # appear only once, not duplicate flag
            if(index EQUAL 0)
                set(tmp_cflags "${AA}")
            else()
                set(tmp_cflags "${tmp_cflags} ${AA}")
            endif()
        else()
            # duplicate flag
            string(FIND "${tmp_cflags}" "${AA}" rv)
            if(rv EQUAL -1)
                # not add, add it here once
                if(index EQUAL 0)
                    set(tmp_cflags "${AA}")
                else()
                    set(tmp_cflags "${tmp_cflags} ${AA}")
                endif()
            endif()
        endif()

        math(EXPR index "${index} + 1")
    endforeach()

    set(${_compile_flags} "${tmp_cflags}" PARENT_SCOPE)

    set(out_msg false)
    foreach(item ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_${build_type}})
        if(out_msg)
            set(out_msg "${out_msg}\n                                        = ${item}")
        else()
            set(out_msg "CMAKE_EXE_LINKER_FLAGS               = ${item}")
        endif()
    endforeach()
    message(STATUS "${out_msg}")

    set(out_msg false)
    foreach(item ${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_${build_type}})
        if(out_msg)
            set(out_msg "${out_msg}\n                                        = ${item}")
        else()
            set(out_msg "CMAKE_SHARED_LINKER_FLAGS            = ${item}")
        endif()
    endforeach()
    message(STATUS "${out_msg}")

    set(out_msg false)
    foreach(item ${CMAKE_MODULE_LINKER_FLAGS} ${CMAKE_MODULE_LINKER_FLAGS_${build_type}})
        if(out_msg)
            set(out_msg "${out_msg}\n                                        = ${item}")
        else()
            set(out_msg "CMAKE_MODULE_LINKER_FLAGS            = ${item}")
        endif()
    endforeach()
    message(STATUS "${out_msg}")

    message(STATUS "Duplicated flags automatically removed for hard coding only !")
endfunction()

macro(GetTargetCompileFlags _target _compile_flags _is_cpp_language)
    message(STATUS "==========================================================")
    message(STATUS "                           ${_target}")
    message(STATUS "==========================================================")
    GetCompileFlags(${_target}_compile_flags ${_target} ${_is_cpp_language})
    set(${_compile_flags} "${${_target}_compile_flags}")
endmacro()
