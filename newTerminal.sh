# automatically opens new terminal at start of program for curl commands

case $SHELL in
*/zsh)
    open -a Terminal .
    ;;
*/bash)
    gnome-terminal
    ;;
*)
    return
esac