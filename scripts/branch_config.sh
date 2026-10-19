#!/bin/bash

midisplitter_branch_name() {
    local project_root="$1"
    local branch

    branch="$(git -C "$project_root" branch --show-current)" || {
        echo "Error: Unable to determine the current Git branch." >&2
        return 1
    }
    if [[ -z "$branch" ]]; then
        branch="detached-$(git -C "$project_root" rev-parse --short HEAD)" || {
            echo "Error: Unable to identify the detached Git revision." >&2
            return 1
        }
    fi
    printf '%s\n' "$branch"
}

midisplitter_branch_slug() {
    local branch="$1"
    printf '%s' "$branch" |
        tr '[:upper:]' '[:lower:]' |
        sed -E 's/[^a-z0-9]+/-/g; s/^-|-$//g'
}

midisplitter_product_name() {
    local branch="$1"
    if [[ "$branch" == "main" ]]; then
        printf '%s\n' "MIDI Splitter"
    else
        printf 'nla_midisplitter_%s_v0.0.1\n' "$(midisplitter_branch_slug "$branch")"
    fi
}

midisplitter_export_category() {
    local branch="$1"
    if [[ "$branch" == "main" ]]; then
        printf '%s\n' "production-builds"
    else
        printf '%s\n' "development-builds"
    fi
}

midisplitter_export_dir() {
    local project_root="$1"
    local branch="$2"
    if [[ "$branch" == "main" ]]; then
        printf '%s/exports/production-builds\n' "$project_root"
    else
        printf '%s/exports/development-builds\n' "$project_root"
    fi
}

midisplitter_build_dir() {
    local project_root="$1"
    local branch="$2"
    local platform="$3"
    printf '%s/build/%s/%s\n' "$project_root" "$(midisplitter_branch_slug "$branch")" "$platform"
}
