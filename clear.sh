# clears up any files created after program runs
# type 'chmod +x clear.sh' and then './clear.sh' to run

sudo rm -rf backup-* > /dev/null 2>&1
sudo rm testfile* > /dev/null 2>&1
make clean > /dev/null 2>&1
clear