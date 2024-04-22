function(ExtractKeyValuePair kvp)
  set(arg_outkey ${ARGV1})
  set(arg_outval ${ARGV2})
  
  string(FIND ${kvp} "=" idx)
  if (idx STREQUAL -1)
	message(FATAL_ERROR "Not a key=value element ! ${kvp}")
  endif()

  # the key
  string(SUBSTRING ${kvp} 0 ${idx} key)
  # skip the =
  math(EXPR idx "${idx} + 1")
  # the value
  string(SUBSTRING ${c} ${idx} -1 value)

  set(${arg_outkey} "${key}" PARENT_SCOPE)
  set(${arg_outval} "${value}" PARENT_SCOPE)
endfunction()

function(SetTargetCompileFeatures target features)
  foreach (kvp IN LISTS ${features})
	ExtractKeyValuePair(${kvp} feat ifc)
	message("Target ${target}: setting feature ${feat}, interface ${ifc}")
	target_compile_features(${target} ${ifc} ${feat})
  endforeach()
endfunction()  

function(dict command dict )
    if(command STREQUAL SET)
        set(arg_key ${ARGV2})
        set(arg_value ${ARGV3})

        dict(_IDX ${dict} "${arg_key}" idx)
        if(NOT idx STREQUAL -1)
            list(REMOVE_AT ${dict} ${idx})
        endif()

        list(APPEND ${dict} "${arg_key}=${arg_value}")
        set(${dict} "${${dict}}" PARENT_SCOPE)

    elseif(command STREQUAL GET)
        set(arg_key ${ARGV2})
        set(arg_outvar ${ARGV3})

        dict(_IDX ${dict} "${arg_key}" idx)
        if(idx STREQUAL -1)
            message(FATAL_ERROR "No key \"${arg_key}\" in dictionary")
        endif()

        list(GET ${dict} ${idx} kv)
        string(REGEX REPLACE "^[^=]+=(.*)" "\\1" value "${kv}")
        set(${arg_outvar} "${value}" PARENT_SCOPE)

    elseif(command STREQUAL _IDX)
        set(arg_key ${ARGV2})
        set(arg_outvar ${ARGV3})
        set(idx 0)
        foreach(kv IN LISTS ${dict})
            string(REGEX REPLACE "^([^=]+)=.*" "\\1" key "${kv}")
            if(arg_key STREQUAL key)
                set(${arg_outvar} "${idx}" PARENT_SCOPE)
                return()
            endif()
            math(EXPR idx ${idx}+1)
        endforeach()
        set(${arg_outvar} "-1" PARENT_SCOPE)

    else()
        message(FATAL_ERROR "dict does not recognize sub-command ${command}")
    endif()
endfunction()
