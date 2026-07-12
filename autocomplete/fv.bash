# fv autocomplete script
_fv_completions() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    local scene_dir="$HOME/.FreeVisualizer/shaders"

    opts="-h --help -s --scene -l --ls -S --yt-search -d --yt-dl -f --fullscreen -p --path -t --test -c --color -r --random -v --sub"

    case "$prev" in
        -s|--scene)
            local scenes
	    scenes=$(for f in "$scene_dir"/*; do
            [ -f "$f" ] && basename "${f%.*}"
	    done 2>/dev/null)
            COMPREPLY=( $(compgen -W "$scenes" -- "$cur") )
            return 0
        ;;
        -p|--path)
            COMPREPLY=( $(compgen -f -- "$cur") )
            return 0
            ;;
        -v|--sub)
            COMPREPLY=( $(compgen -f -X '!*.srt' -- "$cur") )
            return 0
            ;;
        -c|--color)
            return 0
            ;;
    esac

    COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
}
complete -F _fv_completions fv
