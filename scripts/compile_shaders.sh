#!/bin/bash

function exit_with_err() {
    echo "ERROR: $1"
    exit 1
}

function check_exit_code() {
    if [ $? -ne 0 ]; then
        exit_with_err "$1"
    fi
}

function get_mod_time() {
    local file=$1

    if [[ "$(uname)" == "Darwin" ]]; then
        stat -f %m "$file" 2>/dev/null
    else
        stat -c %Y "$file" 2>/dev/null
    fi
}

function read_file_names() {
    local directory=$1

    if [[ ! -d "$directory" ]]; then
        echo "Error: Directory does not exist." >&2
        return 1
    fi

    local file_array=()
    for file in "$directory"/*; do
        if [[ -e "$file" && -f "$file" ]]; then
            file_array+=("$file")
        fi
    done
    echo "${file_array[@]}"
}

which glslc 1>/dev/null
check_exit_code "glslc not in PATH"

out_dir=build/assets/shaders
mkdir -p "$out_dir"
check_exit_code "failed to create ouput directory: $out_dir"

echo "Compiling shaders to $out_dir ..."

shader_array=($(read_file_names "assets/shaders"))
check_exit_code "failed to read file names"

if [[ ${#shader_array[@]} -gt 0 ]]; then
    for shaderFile in "${shader_array[@]}"; do
        out_filename=$(basename "$shaderFile")
        out_filepath="$out_dir/$out_filename"

        # Get the current modification time of the shader file
        mod_time=$(get_mod_time "$shaderFile")
        last_mod_time=$(cat "$out_filepath.modtime" 2>/dev/null || echo "")

        # echo "DEBUG: $shaderFile: mod_time=$mod_time last_mod_time=$last_mod_time"

        # Check modification times and compile only if there was a change
        if [[ $mod_time -eq $last_mod_time ]]; then
            echo "$shaderFile (NO CHANGE)"
            continue
        fi

        # Compile the shader
        glslc "$shaderFile" -o "$out_filepath.spv"
        check_exit_code "failed to compile shader $shaderFile"

        # Save the modification time
        echo "$mod_time" > "$out_filepath.modtime"
        check_exit_code "failed to save modification time for $shaderFile"

        echo "$shaderFile (COMPILED)"
    done
else
    exit_with_err "Shader folder empty"
fi

echo "Shaders Completed Successfully."
