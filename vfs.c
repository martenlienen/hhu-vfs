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


/**
 * Schreibt in eine Datei und gibt bei Erfolg 0 zurück.
 */
int file_write (const void* data, int size, int num, FILE* file) {
  fwrite(data, size, num, file);

  if (ferror(file)) {
    return 1;
  } else {
    return 0;
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

struct ArchiveInfo* archive_info_create_empty () {
  struct ArchiveInfo* archive_info = malloc(sizeof(struct ArchiveInfo));
  archive_info->blocksize = 0;
  archive_info->blockcount = 0;
  archive_info->blocks = NULL;
  archive_info->num_files = 0;
  archive_info->file_infos = NULL;

  return archive_info;
}

int archive_info_initialize (struct ArchiveInfo* archive_info, ulint blocksize, ulint blockcount) {
  archive_info->blocksize = blocksize;
  archive_info->blockcount = blockcount;
  archive_info->blocks = malloc(blockcount * sizeof(int));

  memset(archive_info->blocks, -1, blockcount * sizeof(int));

  return 0;
}

int archive_info_write (struct ArchiveInfo* archive_info, FILE* file) {
  int status = 0;

  status == 0 && (status = file_write(archive_info, sizeof(ulint), 2, file));
  status == 0 && (status = file_write(archive_info->blocks, sizeof(int), archive_info->blockcount, file));
  status == 0 && (status = file_write(&archive_info->num_files, sizeof(int), 1, file));

  int i;
  for (i = 0; i < archive_info->num_files && status == 0; i++) {
    struct FileInfo* file_info = archive_info->file_infos[i]; 

    status = file_write(file_info->name, sizeof(char), strlen(file_info->name) + 1, file);
    status == 0 && (status = file_write(&file_info->size, sizeof(int), 1, file));
  }

  return status;
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
void archive_initialize_paths(struct Archive* archive, const char* archive_path);

/**
 * Initialisiert ein leeres Archiv.
 */
int archive_initialize_empty (struct Archive* archive, const char* archive_path, ulint blocksize, ulint blockcount) {
  archive_initialize_paths(archive, archive_path);

  archive->archive_info = archive_info_create_empty();
  archive_info_initialize(archive->archive_info, blocksize, blockcount);

  int status = 0;

  if (file_exists(archive->store_file) || file_exists(archive->structure_file)) {
    status = ARCHIVE_ALREADY_EXISTS;
  }

  status == 0 && (status = archive_write_archive_info(archive));
  status == 0 && (status = archive_initialize_store(archive));

  return status;
}

void archive_initialize_paths (struct Archive* archive, const char* archive_path) {
  archive->structure_file = malloc((strlen(archive_path) + 10 + 1) * sizeof(char));
  archive->store_file = malloc((strlen(archive_path) + 6 + 1) * sizeof(char));

  sprintf(archive->structure_file, "%s.structure", archive_path);
  sprintf(archive->store_file, "%s.store", archive_path);
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
    status = archive_info_write(archive->archive_info, structure_file);

    if (status != 0) {
      status = ARCHIVE_NOT_WRITEABLE;
    }

    fclose(structure_file);
  }

  return status; 
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
  archive_info_free(archive->archive_info);

  free(archive->structure_file);
  free(archive->store_file);
  free(archive);
}

int cli_create (const char* archive_path, ulint blocksize, ulint blockcount) {
  int status = 0;

  struct Archive* archive = archive_create();
  status = archive_initialize_empty(archive, archive_path, blocksize, blockcount);
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

int cli_add (const char* archive_path, const char* source_path, const char* target) {
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

  char* archive_path = argv[1];
  char* command = argv[2];

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
    
    return cli_create(archive_path, blocksize, blockcount);
  }

  if (strcmp(command, "add") == 0) {
    if (argc < 5) {
      help_add();
      return 66;
    }

    return cli_add(archive_path, argv[3], argv[4]);
  }
}
