CS 460 Programming Assignment 2
by Corey Amoruso, Michael Swiger, Sasha Demyanik

Description
===========
Backs up the files in the current working directory to a directory named
.backup in the current working directory. Can also restore files to the current
working directory from the backup.

Compiling and Running
=====================
To compile BackItUp, simply run `make' in the terminal from the source
directory. To create a backup of the directory in which BackItUp resides, run
`./backitup' from the terminal, and to restore the files from .backup, run
`./backitup -r'.