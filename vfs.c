#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned long int ulint;

bool file_exists (const char* file) {
  FILE* handle;

  if ((handle = fopen(file, "r"))) {
    fclose(handle);

    return true;
  } else {
    return false;
  }
}

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

struct ArchiveInfo {
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

struct ArchiveInfo* archive_info_create_empty (ulint blocksize, ulint blockcount) {
  struct ArchiveInfo* archive_info = malloc(sizeof(struct ArchiveInfo));
  archive_info->blocksize = blocksize;
  archive_info->blockcount = blockcount;
  archive_info->blocks = malloc(blockcount * sizeof(int));
  archive_info->num_files = 0;
  archive_info->file_infos = NULL;

  memset(archive_info->blocks, -1, blockcount);

  return archive_info;
}

/**
 * Speichert das Archiv in eine Strukturdatei.
 */
int archive_info_save (struct ArchiveInfo* archive_info, FILE* file) {
  return 0;
}

void archive_info_free (struct ArchiveInfo* archive_info) {
  free(archive_info->blocks);

  int i;
  for (i = 0; i < archive_info->num_files; i++) {
    fileinfo_free(archive_info->file_infos[i]);
  }

  free(archive_info->file_infos);

  free(archive_info); 
}

#define ARCHIVE_ALREADY_EXISTS 1
#define ARCHIVE_NOT_WRITEABLE 2

/**
 * Ein High-Level-Interface um mit einem Archiv zu interagieren.
 */
struct Archive {
  char* structure_file;
  char* store_file;
  struct ArchiveInfo* archive_info;
};

/**
 * Erstellt ein leeres Archiv, das dann mit einer der Initializer-Methoden
 * geladen werden muss.
 */
struct Archive* archive_create () {
  struct Archive* archive = malloc(sizeof(struct Archive));
  archive->structure_file = NULL;
  archive->store_file = NULL;
  archive->archive_info = NULL;

  return archive;
}

int archive_write_archive_info (struct Archive*);
int archive_initialize_store(struct Archive*);
int archive_fwrite(const void*, int, int, FILE*);

/**
 * Initialisiert ein leeres Archiv.
 */
int archive_initialize_empty (struct Archive* archive, const char* structure_file, const char* store_file, ulint blocksize, ulint blockcount) {
  archive->structure_file = malloc((strlen(structure_file) + 1) * sizeof(char));
  strcpy(archive->structure_file, structure_file);

  archive->store_file = malloc((strlen(store_file) + 1) * sizeof(char));
  strcpy(archive->store_file, store_file);

  archive->archive_info = archive_info_create_empty(blocksize, blockcount);

  int status = 0;

  if (file_exists(archive->store_file) || file_exists(archive->structure_file)) {
    status = ARCHIVE_ALREADY_EXISTS;
  }

  status == 0 && (status = archive_write_archive_info(archive));
  status == 0 && (status = archive_initialize_store(archive));

  return status;
}

/**
 * Schreibt die Strukturdatei neu.
 *
 * @private
 */
int archive_write_archive_info (struct Archive* archive) {
  int status = 0;

  FILE* structure_file = fopen(archive->structure_file, "w");

  if (structure_file == NULL) {
    status = ARCHIVE_NOT_WRITEABLE;
  } else {
    struct ArchiveInfo* archive_info = archive->archive_info;

    status == 0 && (status = archive_fwrite(archive_info, sizeof(ulint), 2, structure_file));
    status == 0 && (status = archive_fwrite(archive_info->blocks, sizeof(int), archive_info->blockcount, structure_file));
    status == 0 && (status = archive_fwrite(&archive_info->num_files, sizeof(int), 1, structure_file));

    int i;
    for (i = 0; i < archive_info->num_files && status == 0; i++) {
      struct FileInfo* file_info = archive_info->file_infos[i]; 

      archive_fwrite(file_info->name, sizeof(char), strlen(file_info->name) + 1, structure_file);
      status == 0 && (status = archive_fwrite(&file_info->size, sizeof(int), 1, structure_file));
    }

    if (status != 0) {
      status = ARCHIVE_NOT_WRITEABLE;
    }

    fclose(structure_file);
  }

  return status; 
}

/**
 * Schreibt in eine Datei und gibt bei Erfolg 0 zurück.
 *
 * @private
 */
int archive_fwrite (const void* data, int size, int num, FILE* file) {
  fwrite(data, size, num, file);

  if (ferror(file)) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * Initialisiert einen leeren Datenstore in eine nicht existente Datei.
 *
 * @private
 */
int archive_initialize_store (struct Archive* archive) {
  int status = 0;

  FILE* store = fopen(archive->store_file, "w");

  if (store == NULL) {
    status = ARCHIVE_NOT_WRITEABLE;
  } else {
    struct ArchiveInfo* archive_info = archive->archive_info;
    const char* zero_block[archive_info->blocksize];
    memset(&zero_block, 0, archive_info->blocksize);

    ulint i;
    for (i = 0; i < archive_info->blockcount; i++) {
      fwrite(zero_block, sizeof(char), archive_info->blocksize, store);

      if (ferror(store) != 0) {
        status = ARCHIVE_NOT_WRITEABLE;
      }
    }

    fclose(store);
  }

  return status;
}

/**
 * Gibt alle belegten Resourcen frei.
 */
void archive_free (struct Archive* archive) {
  free(archive->structure_file);
  free(archive->store_file);
  free(archive->archive_info);
  free(archive);
}

int cli_create (char* structure_path, char* store_path, ulint blocksize, ulint blockcount) {
  int status = 0;

  struct Archive* archive = archive_create();
  status = archive_initialize_empty(archive, structure_path, store_path, blocksize, blockcount);
  archive_free(archive);

  switch (status) {
    case ARCHIVE_NOT_WRITEABLE:
      printf("Das Archiv ist nicht beschreibbar");
      return 1;
    case ARCHIVE_ALREADY_EXISTS:
      printf("Es existiert bereits ein Archiv mit dem selben Namen");
      return 3;
    default:
      return 0;
  }
}

int cli_add (const char* structure_path, const char* store_path, const char* source_path, const char* target) {
  int status = 0;

  if (!file_exists(source_path)) {
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
