# comb
Combine many sorted text files int a single sorted file, eliminating duplicates.
This is a delivered as a stand alone program written in C++ which can be compiled and executed from the command line in UNIX.
The syntax: comb [-n] file0 file1 ...  
The resulting file is written to the standard output.
The advantage of this program is that it is very low on memory resources and each of the contributing files is read only once.

The initial version of the program makes a few assumptions which will hold in all but the more extreme situations.
The lines are terminated by line feed.
The line length is limited to 100000, although this can be easily changed in the source code.
The program does not check for any limit of the number of files that can be simultaneously opened. It needs a file handle for each file.
If the limit is exceeded, an fopen() call will return NULL and the program will terminate.
The open file limit can be found by executing "ulimit -n" although this limit is not always strictly enforced in every system.
The optional -n parameter (i.e. -2) specifies the number of files whose records will be removed from the program output.
For example -2 will result in removal of records that appear in the first two files file0, file1. 

The basic algorithm is to open every file and maintain a list of records, one from each file.
The alphabetically earliest record is written to the standard output and the next record is read from that file and any other files which shared the same record.
For each of the sharing files, the next record is read and the files are reordered according to the alphabetic order of their current record.
Most of the time, the reordering does not require a full sort since only a few of the files read a new record. Shortcuts can be taken.
The process iterates until all records from the files have been read.

The initial application of the program was for use in solving a large sliding block problem using a brute force method.
When determining all positions that could be reached in exactly 172 moves from the initial position, we started with two files containing the set of positions reachable in exactly 170 and 171 moves respectively. From the 171 move file we generated adjacent positions 100,000,000 at a time, sorting them, removing duplicates and writing to disk. This resulted in around 300 sorted files, each of which had about 60,000,000 positions. Each of the positions in the files was reachable in either 170, 171 or 172 moves from the starting position.
Then the comb program was run with the -2 option: comb -2 wave170 wave171 wave172_0 wave172_1 .... > wave172
It combined all the positions in the 300 sorted files while removing the positions that could be reached in exactly 170 or 171 moves.    
The resulting file (wave172) contained all positions reachable in exactly 172 moves. 
