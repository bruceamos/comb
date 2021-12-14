# comb
Combine many sorted text files into a single sorted file, eliminating duplicates.
This is a delivered as a stand alone program written in C++ which can be compiled and executed from the command line in UNIX.
The syntax: comb [-n] file0 file1 ...  
The resulting file is written to the standard output.
The advantage of this program is that it is very low on memory resources and each of the contributing files is read only once.
For a large number of big file, it may take some time to run, but this is much less than by using repeated executions of the comm program.

The initial version of the program makes a few assumptions which will hold in most situations.
Each input file is an ordinary text file with lines terminated by a line feed.
The program simultaneously opens each file and creates a buffer. The buffer size is hard coded as 100000, so it is assumed that 
no line of any file exceeds this length. There much be enough memory in the system to hold all the buffers and the system must also
provide the ability to open the requested number of files simultaneously.
The buffer size can be changed in the source code and the program recompiled if necessary.
The number of open files per process can be obtained by executing the command "ulimit -n".
If it needs to be increased, say from 256 to 2560 for example, issue the command "ulimit -n 2560".
If the limit is exceeded, an fopen() call will return NULL and the program will terminate.
The optional -n parameter (i.e. -2) specifies the number of files whose records will be removed from the program output, instead of being included.
For example -2 will have the result that the output will exclude any records that appear in the first two files file0, file1. 

The basic algorithm is to open every file and maintain a list of records, one from each file.
The alphabetically earliest record (taken over all files) is written to the standard output and the next record is read for all files which shared that record.
For each of the sharing files, the next record is read and the files are reordered according to the alphabetic order of their current record.
Most of the time, the reordering does not require a full sort since only a few of the files read a new record. Shortcuts can be taken.
The process iterates until all records from the files have been read.

The initial application of the program was for use in solving a large sliding block problem using a brute force search method.
When determining all positions that could be reached from the starting position in exactly 172 moves, for example, we started with two files containing the set of positions reachable in exactly 170 and 171 moves respectively. From the 171 move file we generated adjacent positions 100,000,000 at a time, sorting them, removing duplicates and writing to disk. This resulted in around 300 sorted files, each of which had about 60,000,000 positions. Each of the positions in the files,
since it could be reached in a single move from a position that was reachable in 171 moves, must be reachable in either 170, 171 or 172 moves.
Then the comb program was run with the -2 option: comb -2 wave170 wave171 wave172_0 wave172_1 .... > wave172
It combined all the positions in the 300 sorted files while removing the positions that could be reached in exactly 170 or 171 moves.    
The resulting file (wave172) contained all positions reachable in 172 moves, but not reachable in less than 172 moves. 
