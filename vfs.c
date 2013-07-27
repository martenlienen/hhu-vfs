#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned long int ulint;

struct FileInfo {
  /**
   * Dateiname
   */
  char* name;

  /**
   * Dateigröße in Bytes
   */
  int size;
};

void fileinfo_free (struct FileInfo* file_info) {
  free(file_info->name);
  free(file_info);
}

struct Archive {
  /**
   * Größe eines Blocks in Bytes
   */
  ulint blocksize;

  /**
   * Anzahl der Blöcke im Archiv
   */
  ulint blockcount;

  /**
   * Enthält für jeden Block ein int. Wenn der Werte -1 ist, ist der Block frei,
   * ansonsten gehört er zu der i-ten FileInfo.
   */
  int* blocks;

  /**
   * Anzahl der Dateien im Archiv/Element in file_infos
   */
  int num_files;

  struct FileInfo** file_infos;
}; 

struct Archive* archive_create_empty (ulint blocksize, ulint blockcount) {
  struct Archive* archive = malloc(sizeof(struct Archive));
  archive->blocksize = blocksize;
  archive->blockcount = blockcount;
  archive->blocks = malloc(blockcount * sizeof(int));
  archive->num_files = 0;
  archive->file_infos = NULL;

  memset(archive->blocks, -1, blockcount);

  return archive;
}

/**
 * Speichert das Archiv in eine Strukturdatei.
 */
int archive_save (struct Archive* archive, FILE* file) {
  return 0;
}

/**
 * Initialisiert die Datendatei mit Nullen.
 */
int archive_init_data_file (struct Archive* archive, FILE* file) {
  const char* zero_block[archive->blocksize];
  memset(&zero_block, 0, archive->blocksize);

  int i;
  for (i = 0; i < archive->blockcount; i++) {
    fwrite(zero_block, sizeof(char), archive->blocksize, file);
  }

  return 0;
}

void archive_free (struct Archive* archive) {
  free(archive->blocks);

  int i;
  for (i = 0; i < archive->num_files; i++) {
    fileinfo_free(archive->file_infos[i]);
  }

  free(archive->file_infos);

  free(archive); 
}

bool file_exists (const char* file) {
  FILE* handle;

  if (handle = fopen(file, "r")) {
    fclose(handle);

    return true;
  } else {
    return false;
  }
}

int cli_create (char* structure_path, char* store_path, ulint blocksize, ulint blockcount) {
  int status = 0;

  if (file_exists(store_path) || file_exists(structure_path)) {
    printf("Das Archiv existiert bereits");

    status = 3;
  } else {
    FILE* store_file = fopen(store_path, "a");

    if (store_file == NULL) {
      printf("Das Archiv konnte nicht erstellt werden");

      status = 1;
    } else {
      FILE* structure_file = fopen(structure_path, "a");

      if (structure_file == NULL) {
        printf("Das Archiv konnte nicht erstellt werden");

        if (remove(store_path) != 0) {
          printf("Die bereits erstellte Datendatei konnte nicht gelöscht werden");
        }
        
        status = 1;
      } else {
        struct Archive* archive = archive_create_empty(blocksize, blockcount);

        if (archive_save(archive, structure_file) != 0) {
          printf("Die Strukturdatei konnte nicht gespeichert werden");
          status = 1;
        } else if (archive_init_data_file(archive, store_file) != 0) {
          printf("Die Datendatei konnte nicht erstellt werden");

          if (remove(structure_path) != 0) {
            printf("Die bereits erstellt Datendatei konnte nicht gelöscht werden");
          }

          status = 1;
        }

        archive_free(archive);
      }

      fclose(store_file);
    }
  }

  return status;
}

int cli_add (const char* structure_path, const char* store_path, const char* sourcePath, const char* target) {
  int status = 0;

  if (!file_exists(sourcePath)) {
    printf("Quelldatei existiert nicht");

    status = 13;
  }

  return status;
}

void help_create () {
  printf("USAGE: vfs ARCHIVE create BLOCKSIZE BLOCKCOUNT");
}

void help_add () {
  printf("USAGE: vfs ARCHIVE add SOURCE TARGET");
}

void help () {
  help_create();
  help_add();
}

int main (int argc, char** argv) {
  if (argc < 3) {
    help();
    return 66;
  }

  char* path_prefix = argv[1];
  char store_path[strlen(path_prefix) + 6 + 1];
  char structure_path[strlen(path_prefix) + 10 + 1];
  char* command = argv[2];

  sprintf(store_path, "%s.store", path_prefix);
  sprintf(structure_path, "%s.structure", path_prefix);

  if (strcmp(command, "create") == 0) {
    if (argc < 5) {
      help_create();
      return 66;
    }

    long int blocksize = strtol(argv[3], NULL, 10);
    long int blockcount = strtol(argv[4], NULL, 10);

    if (blocksize <= 0 || blockcount <= 0) {
      printf("BLOCKSIZE und BLOCKCOUNT müssen echt positiv sein");
      return 66;
    }
    
    return cli_create(structure_path, store_path, blocksize, blockcount);
  }

  if (!file_exists(store_path) || !file_exists(structure_path)) {
    printf("Das Archiv existiert nicht");
    return 2;
  }

  if (strcmp(command, "add") == 0) {
    if (argc < 5) {
      help_add();
      return 66;
    }

    return cli_add(structure_path, store_path, argv[3], argv[4]);
  }
}
