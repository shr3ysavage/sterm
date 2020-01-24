sudo apt-get install libreadline-dev
mkdir sterm
gcc sterm.c -L/usr/include -lreadline -o sterm/shell
