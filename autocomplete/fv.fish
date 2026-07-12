set -l scene_dir "$HOME/.FreeVisualizer/shaders"

complete -c fv -s h -l help -d 'Print help'
complete -c fv -s l -l ls -d 'List scenes'
complete -c fv -s S -l yt-search -d 'Search youtube, return 10 results'
complete -c fv -s d -l yt-dl -d 'Download audio of a YouTube video by title'
complete -c fv -s f -l fullscreen -d 'Start in fullscreen mode'
complete -c fv -s t -l test -d 'No audio mode, testing shaders'
complete -c fv -s c -l color -d 'Custom colors, e.g. "#FF0000 #0000FF"'
complete -c fv -s r -l random -d 'Random color pallet'

complete -c fv -s s -l scene -d 'Which scene to use' -r -f -a "(ls $scene_dir 2>/dev/null | string replace -r '\.[^.]*\$' '')"

complete -c fv -s p -l path -d 'Use the shader at path' -r -F

complete -c fv -s v -l sub -d 'Provide a .srt subtitle file' -r -F -a "(__fish_complete_suffix .srt)"
