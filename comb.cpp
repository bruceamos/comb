//comb.cpp
//program to combine multiple sorted files into one file.
//Syntax:
//comb file0 file1 ...: combine all the files, writing to the standard output.
//comb -n file0 file1 ...: combine files excluding lines that appear in one of the first n.
//Program written by Bruce Amos, November 2021.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int maxFiles();
struct File {
  FILE *fp;
  char fnm[132];
  unsigned char *buffer;
  unsigned char *rec; //current record
  bool eof; //indicator that end of file has been reached and no current record is available
  int space; //space allocated for the buffer in characters
  long nelm; //actual number of characters in the buffer
  int lastTerminator;
  int cursor; //index of the current record
  File(int s, char *fileName) {
    strncpy(fnm, fileName, 132);
    eof = false;
    space = s;
    cursor = 0;
    rec = buffer = (unsigned char*)calloc(space, sizeof(unsigned char));
    fp = fopen(fileName, "rb");
    if (!fp) { fprintf(stderr, "File %s not found\n", fileName); exit(1); }
    nelm = fread(buffer, 1, space, fp);
    for (int i = 0; i < nelm && i < space; i++) if (buffer[i] == '\n') {
      buffer[i] = '\0'; //replace line feeds with string terminator.
      lastTerminator = i;
    }
    if (nelm == 0) { eof = true; rec = NULL; }
  }
  ~File() {
    fclose(fp);
    free(buffer);
  }
  void advance() { //advance either leaves a record to reference or sets eof=true.
    long residue;
    if (eof) return;
    cursor += 1 + strlen((char*)rec);
    rec = buffer + cursor;
    if (cursor < lastTerminator) return;
    if (nelm < space) { //previous read didn't fill the buffer, so there are no more characters to read.
      eof = true;
      rec = NULL;
      fprintf(stderr, "File %s reached EOF\n", fnm);
      return;
    }
    //there may be more to read. Possibly the last read filled the buffer exactly.
    residue = nelm - cursor; //number of unterminated characters at the end of the buffer.
    if (residue > 0) memcpy(buffer, buffer+cursor, residue);
    nelm = residue + fread(buffer + residue, 1, space - residue, fp);
    if (nelm == 0) { //case where the last record was at the very end of the buffer.
      eof = true;
      rec = NULL;
      fprintf(stderr, "File %s reached EOF\n", fnm);
      return;
    }
    //if (nelm < space) fprintf(stderr, "File %s nearing EOF, %ld characters remaining.\n", fnm, space - nelm);
    for (int i = 0; i < nelm && i < space; i++) if (buffer[i] == '\n') {
      buffer[i] = '\0'; //replace line feeds with string terminator.
      lastTerminator = i;
    }
    cursor = 0;
    rec = buffer;
  }
};

int cmpr(File *A, File *B) {
  if (A->eof) return (B->eof) ? 0 : 1;
  if (B->eof) return -1;
  return strcmp((char*)A->rec,(char*)B->rec);
}


int main(int argc, char**argv) {
  const int block = 1000000; //number of characters per file held in the buffer
  int numExclude = 0;
  if (argc > 1 && *argv[1] == '-' && strlen(argv[1]) > 1) {
    numExclude = atoi(argv[1]+1);
    argv++;
    argc--;
  }
  long numSkip = 0, numSort = 0, numWrote = 0;
  int gap;
  int i,j,k, numEOF = 0, numActive, numAdvanced;
  int numFiles = argc - 1;
  int fileLimit = maxFiles();
  fprintf(stderr, "System file limit is %d\n", fileLimit);
  if (numFiles >= fileLimit) fprintf(stderr, "Warning: Number of files %d exceeds %d\n", numFiles, fileLimit);
  File **files = (File**)calloc(numFiles, sizeof(File*));
  int *order = (int*)calloc(numFiles, sizeof(int));
  for (i = 0; i < numFiles; i++) order[i] = i; //order is a permutation vector
  for (i = numEOF = numActive = 0; i < numFiles; i++) {
    files[i] = new File(block, argv[i+1]);
    if (files[i]->eof) numEOF++;
  }
  numActive = numAdvanced = numFiles; //the number of files that need to be compared.
  //get the lowest buffer
  const int gaps[8] = { 1, 4, 10, 23, 57, 132, 301, 701 }; //optimum gaps for the shell sort
  while (numEOF < numFiles) {
    //the best sorting method depends on the data. The shell sort is the backdrop but we can accomodate special cases.
    //when the input files are mostly disjoint we often have only 1 or two advancements to deal with.
    int low, mid, high, place;
    if (numAdvanced < 3 && numActive > 2 &&
        cmpr(files[order[0]], files[order[2]]) <= 0 && cmpr(files[order[1]], files[order[2]]) <= 0) { //only [0], [1] are relevant
      if (cmpr(files[order[0]], files[order[1]]) > 0) {
        order[0] ^= order[1];
        order[1] ^= order[0];
        order[0] ^= order[1];
      }
    }
    else if (numAdvanced < 3 && numActive > 7) { //again only order[0], order[1] were advanced, but they went past order[2].
      for (k = numAdvanced - 1; k >= 0; k--) { //move order[k] into place among order[k+1] ... order[numActive-1]
        if (cmpr(files[order[k]], files[order[k+1]]) <= 0) place = k;
        else if (cmpr(files[order[k]], files[order[numActive - 1]]) >= 0) place = numActive - 1;
        else {
          low = k+1; high = numActive - 1; mid = (high + low) /2;
          while (high - low > 1) {
            mid = (high + low) / 2;
            int z = cmpr(files[order[k]], files[order[mid]]);
            if (z > 0) { low = mid; mid = (high + low) /2; }
            else if (z < 0) { high = mid; mid = (high + low) /2; }
            else break;
          }
          if (high - low > 1) place = mid - 1; //[k] is equal to [mid], so [k] can go to [mid-1] while [mid] stays fixed
          else place = low; //[k] will go to [low] and [k+1 ... low] will shift to [k ... low-1].
        }
        if (place > k) {
          int temp = order[k];
          for (int z = k; z < place; z++) order[z] = order[z+1];
          order[place] = temp;
        }
      }
    }
    else { //default is the normal shell sort
      for (k = 7; k >= 0; k--) {
        if (gaps[k] > numActive/2) continue;
        gap = gaps[k];
        for (i = gap; i < numActive; i++) {
          for (j = i - gap; j >= 0 && cmpr(files[order[j]], files[order[j+gap]]) > 0; j -= gap) {
            order[j] ^= order[j+gap];
            order[j+gap] ^= order[j];
            order[j] ^= order[j+gap];
          }
        }
      }
    }
    numSort++;
    numActive = numFiles - numEOF; //the EOF files don't need to take part in future sorts
    //now order[0] is the file with the lowest string
    //advance the other files when they duplicate files[order[0]]
    bool use = true;
    numAdvanced = 1;
    if (order[0] < numExclude) use = false;
    for (i = 1; i < numFiles && !files[order[i]]->eof && !strcmp((char*)files[order[i]]->rec, (char*)files[order[0]]->rec); i++) {
      //if it is the same ...
      if (order[i] < numExclude) use = false; //this record is present in one of the exclusion files
      files[order[i]]->advance();
      numAdvanced++;
      if (files[order[i]]->eof) numEOF++; //the file was not eof before the advance
    }
    if (use) { printf("%s\n", files[order[0]]->rec); numWrote++; }
    //advance the order[0] file
    files[order[0]]->advance();
    if (files[order[0]]->eof) numEOF++;
    //check to see if the order[0] file is still the smallest and can be advanced more.
    if (numFiles > 1 && numAdvanced < 2) { //none of files order[1] ... order[numFiles-1] was advanced, so they are still in order
      while (numEOF < numFiles && cmpr(files[order[0]], files[order[1]]) < 0) {
        numSkip++;
        if (order[0] >= numExclude) { printf("%s\n", files[order[0]]->rec); numWrote++; }
        files[order[0]]->advance();
        if (files[order[0]]->eof) numEOF++;
      }
    }
  }
  //we are done.
  for (i = 0; i < numFiles; i++) delete files[i];
  free(order);
  free(files);
  //report statistics. This can lead to improvements in the sorting procedure.
  fprintf(stderr, "numSort=%ld, numSkip=%ld, numWrote=%ld\n", numSort, numSkip, numWrote);
  return 0;
}

//the system may have limits on how many file handles can be open at one time.
//in practise it is usually possible to have more handles open than what ulimit indicates, so we only issue a warning.
int maxFiles() {
  char buffer[128];
  int rc;
  auto pipe = popen("ulimit -n", "r");
  if (!pipe) { fprintf(stderr, "Command ulimit -n failed\n"); return 0; }
  if (feof(pipe)) { fprintf(stderr, "Command ulimit -u produced no output\n"); return 0; }
  fgets(buffer, 128, pipe);
  rc = atoi(buffer);
  if (rc <= 0) { fprintf(stderr, "Command ulimit -u produced %s\n", buffer); return 0; }
  pclose(pipe);
  return rc;
}
