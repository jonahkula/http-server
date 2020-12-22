# SINGLE THREADED HTTP SERVER WITH BACKUP AND RECOVERY

Jonah Kulakofsky\
December 10, 2020

## NOTES:

Runs on linux ubuntu version 18.04\
This server handles PUT and GET commands\
You can run these curl commands only on files in the directory\
The timestamps for backup and recovery here are in unix format

## STEPS:

1. In terminal, type "make" to create binary file in the directory
2. Type "./httpserver (address) (port number)" to run
3. Address can be localhost or an IP address
4. Port is optional and included after address, must be above 1024 and defaults to 1025 otherwise
5. Run curl commands in a new terminal
6. Run 'clear.sh' shell script to clean up any files created when ran or just "make clean"

## BACKUP AND RECOVERY:

- To backup the directory, call GET for a file 'b'
- To recover the most recent backup, call GET for a file 'r'
- To recover a backup with a specific timestamp, call GET for a file 'r/(timestamp)'
- To view a list of timestamps of all available backups, call GET for a file 'l'
