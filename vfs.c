#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool file_exists (const char* file) {
  FILE* handle;

  if (handle = fopen(file, "r")) {
    fclose(handle);

    return true;
  } else {
    return false;
  }
}

int main (int argc, char** argv) {
  long int i;

  if (argc < 5) {
    printf("USAGE: vfs ARCHIVE create BLOCKSIZE BLOCKCOUNT");
    return 66;
  }

  char* pathPrefix = argv[1];
  char storePath[strlen(pathPrefix) + 6 + 1];
  char structurePath[strlen(pathPrefix) + 10 + 1];
  long int blocksize = strtol(argv[3], NULL, 10);
  long int blockcount = strtol(argv[4], NULL, 10);

  sprintf(storePath, "%s.store", pathPrefix);
  sprintf(structurePath, "%s.structure", pathPrefix);

  if (file_exists(storePath) || file_exists(structurePath)) {
    printf("Die Datei existiert bereits");
    return 3;
  }

  FILE* storeFile = fopen(storePath, "a");

  if (storeFile == NULL) {
    printf("Konnte %s nicht öffnen", storePath);
    return 1;
  }

  FILE* structureFile = fopen(structurePath, "a");

  if (structureFile == NULL) {
    printf("Konnte %s nicht öffnen", structurePath);
    return 1;
  }

  for (i = 0; i < blocksize * blockcount; i++) {
    fprintf(storeFile, " ");
  }

  fclose(storeFile);
  fclose(structureFile);

  return 0;
}
