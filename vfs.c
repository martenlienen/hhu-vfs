#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

bool file_exists (const char* file) {
  FILE* handle = fopen(file, "r");

  if (handle == NULL) {
    return false;
  } else {
    fclose(handle);

    return true;
  }
}

/**
 * Gibt die Größe der Datei in Bytes zurück.
 *
 * Im Fehlerfall wird -1 zurückgegeben.
 */
long int file_size (FILE* file) {
  long int old_position = ftell(file);

  if (old_position == -1) {
    return -1;
  } else {
    if (fseek(file, 0, SEEK_END) == -1) {
      return -1;
    }

    long int size = ftell(file);

    if (fseek(file, old_position, SEEK_SET) == -1) {
      return -1;
    } else {
      return size;
    }
  }
}

/**
 * Schreibt in eine Datei und gibt bei Erfolg 0 zurück.
 */
int file_write (const void* data, int size, int num, FILE* file) {
  fwrite(data, size, num, file);

  return ferror(file);
}

/**
 * Liest aus einer Datei und gibt bei Erfolg 0 zurück.
 */
int file_read (void* ptr, size_t size, size_t count, FILE* file) {
  fread(ptr, size, count, file);

  return ferror(file);
}

struct FileInfo {
  /**
   * Dateiname
   */
  char* name;

  /**
   * Dateigröße in Bytes
   */
  uint64_t size;
};

struct FileInfo* fileinfo_create () {
  struct FileInfo* file_info = malloc(sizeof(struct FileInfo));
  file_info->name = NULL;
  file_info->size = 0;

  return file_info;
}

void fileinfo_initialize (struct FileInfo* file_info, const char* name, long int size) {
  file_info->name = malloc((strlen(name) + 1) * sizeof(char));
  strcpy(file_info->name, name);

  file_info->size = size;
}

int fileinfo_initialize_from_file (struct FileInfo* file_info, FILE* file) {
  int status = 0;
  int name_length;

  status = file_read(&name_length, sizeof(int), 1, file);

  file_info->name = malloc((name_length + 1) * sizeof(char));

  status == 0 && (status = file_read(file_info->name, sizeof(char), name_length, file));

  file_info->name[name_length] = 0;

  status == 0 && (status = file_read(&file_info->size, sizeof(uint64_t), 1, file));

  return status;
}

int fileinfo_write (struct FileInfo* file_info, FILE* file) {
  int status = 0;
  int name_length = strlen(file_info->name);

  status = file_write(&name_length, sizeof(int), 1, file);
  status == 0 && (status = file_write(file_info->name, sizeof(char), strlen(file_info->name), file));
  status == 0 && (status = file_write(&file_info->size, sizeof(uint64_t), 1, file));

  return status;
}

void fileinfo_free (struct FileInfo* file_info) {
  free(file_info->name);
  free(file_info);
}

struct ArchiveInfo {
  /**
   * Größe eines Blocks in Bytes
   */
  uint64_t blocksize;

  /**
   * Anzahl der Blöcke im Archiv
   */
  uint64_t blockcount;

  /**
   * Enthält für jeden Block ein int. Wenn der Werte -1 ist, ist der Block frei,
   * ansonsten gehört er zu der i-ten FileInfo.
   */
  int64_t* blocks;

  /**
   * Anzahl der Dateien im Archiv/Element in file_infos
   */
  uint64_t num_files;

  struct FileInfo** file_infos;
}; 

struct ArchiveInfo* archiveinfo_create () {
  struct ArchiveInfo* archive_info = malloc(sizeof(struct ArchiveInfo));
  archive_info->blocksize = 0;
  archive_info->blockcount = 0;
  archive_info->blocks = NULL;
  archive_info->num_files = 0;
  archive_info->file_infos = NULL;

  return archive_info;
}

/**
 * Initialisiert ein ArchiveInfo für ein leeres Archiv.
 */
int archiveinfo_initialize_empty (struct ArchiveInfo* archive_info, uint64_t blocksize, uint64_t blockcount) {
  archive_info->blocksize = blocksize;
  archive_info->blockcount = blockcount;
  archive_info->blocks = malloc(blockcount * sizeof(int64_t));

  memset(archive_info->blocks, -1, blockcount * sizeof(int64_t));

  return 0;
}

/**
 * Lädt Archivinfos aus einer Datei.
 */
int archiveinfo_initialize_from_file (struct ArchiveInfo* archive_info, FILE* file) {
  int status = 0;

  status == 0 && (status = file_read(archive_info, sizeof(uint64_t), 2, file));

  archive_info->blocks = malloc(archive_info->blockcount * sizeof(uint64_t));

  status == 0 && (status = file_read(archive_info->blocks, sizeof(int64_t), archive_info->blockcount, file));
  status == 0 && (status = file_read(&archive_info->num_files, sizeof(uint64_t), 1, file));

  archive_info->file_infos = malloc(archive_info->num_files * sizeof(struct FileInfo*));

  uint64_t i;
  for (i = 0; i < archive_info->num_files && status == 0; i++) {
    archive_info->file_infos[i] = fileinfo_create();
    status = fileinfo_initialize_from_file(archive_info->file_infos[i], file);
  }

  return status;
}

int archiveinfo_write (struct ArchiveInfo* archive_info, FILE* file) {
  int status = 0;

  status == 0 && (status = file_write(archive_info, sizeof(uint64_t), 2, file));
  status == 0 && (status = file_write(archive_info->blocks, sizeof(int64_t), archive_info->blockcount, file));
  status == 0 && (status = file_write(&archive_info->num_files, sizeof(uint64_t), 1, file));

  uint64_t i;
  for (i = 0; i < archive_info->num_files && status == 0; i++) {
    status = fileinfo_write(archive_info->file_infos[i], file);
  }

  return status;
}

bool archiveinfo_has_file (struct ArchiveInfo* archive_info, const char* name) {
  uint64_t i;
  for (i = 0; i < archive_info->num_files; i++) {
    if (strcmp(name, archive_info->file_infos[i]->name) == 0) {
      return true;
    }
  }

  return false;
}

/**
 * Gibt die Anzahl der freien Blöcke im Archiv zurück.
 */
uint64_t archiveinfo_num_free_blocks (struct ArchiveInfo* archive_info) {
  uint64_t num_free = 0;

  uint64_t i;
  for (i = 0; i < archive_info->blockcount; i++) {
    if (archive_info->blocks[i] == -1) {
      num_free++;
    }
  }

  return num_free;
}

/**
 * Gibt die Anzahl der freien Blöcke zurück, die benötigt werden, um eine Datei
 * der Größe size zu speichern.
 */
uint64_t archiveinfo_needed_blocks (struct ArchiveInfo* archive_info, long int size) {
  uint64_t needed = size / archive_info->blocksize;

  if (size % archive_info->blocksize != 0) {
    needed++;
  }

  return needed;
}

/**
 * Schreibt die Indizes von num freien Blöcken in das Array blocks.
 */
void archiveinfo_get_free_blocks (struct ArchiveInfo* archive_info, uint64_t* blocks, uint64_t num) {
  uint64_t block_index = 0;
  uint64_t i;
  for (i = 0; i < archive_info->blockcount && block_index < num; i++) {
    if (archive_info->blocks[i] == -1) {
      blocks[block_index] = i;
      block_index++;
    }
  }
}

/**
 * Gibt den Index der Datei in der internen Liste der Dateien zurück.
 *
 * @private
 */
int64_t archiveinfo_get_file_index (struct ArchiveInfo* archive_info, const char* name) {
  uint64_t i;
  for (i = 0; i < archive_info->num_files; i++) {
    if (strcmp(name, archive_info->file_infos[i]->name) == 0) {
      return i;
    }
  }

  return -1;
}

uint64_t archiveinfo_get_file_size (struct ArchiveInfo* archive_info, const char* name) {
  int64_t index = archiveinfo_get_file_index(archive_info, name);

  if (index == -1) {
    return 0;
  } else {
    return archive_info->file_infos[index]->size;
  }
}

uint64_t archiveinfo_get_num_allocated_blocks (struct ArchiveInfo* archive_info, const char* name) {
  int index = archiveinfo_get_file_index(archive_info, name);

  if (index == -1) {
    return 0;
  } else {
    uint64_t num = 0;

    uint64_t i;
    for (i = 0; i < archive_info->blockcount; i++) {
      if (archive_info->blocks[i] == index) {
        num++;
      }
    }

    return num;
  }
}

void archiveinfo_get_allocated_blocks (struct ArchiveInfo* archive_info, const char* name, uint64_t* blocks) {
  int index = archiveinfo_get_file_index(archive_info, name);

  if (index != -1) {
    int block_index = 0;
    uint64_t i;
    for (i = 0; i < archive_info->blockcount; i++) {
      if (archive_info->blocks[i] == index) {
        blocks[block_index] = i;
        block_index++;
      }
    }
  }
}

/**
 * Fügt eine neue Datei hinzu und reserviert die übergebenen Blöcke dafür.
 */
void archiveinfo_add_file (struct ArchiveInfo* archive_info, const char* name, long int size, uint64_t* blocks, uint64_t num_blocks) {
  int file_info_index = archive_info->num_files;

  archive_info->num_files++;
  archive_info->file_infos = realloc(archive_info->file_infos, (archive_info->num_files) * sizeof(struct FileInfo*));
  archive_info->file_infos[file_info_index] = fileinfo_create();
  fileinfo_initialize(archive_info->file_infos[file_info_index], name, size);

  uint64_t i;
  for (i = 0; i < num_blocks; i++) {
    archive_info->blocks[blocks[i]] = file_info_index;
  }
}

void archiveinfo_delete_file (struct ArchiveInfo* archive_info, const char* name) {
  int index = archiveinfo_get_file_index(archive_info, name);

  if (index != -1) {
    uint64_t i;
    for (i = index; i < archive_info->num_files - 1; i++) {
      archive_info->file_infos[i] = archive_info->file_infos[i + 1];
    }

    archive_info->num_files--;
    archive_info->file_infos = realloc(archive_info->file_infos, archive_info->num_files * sizeof(struct FileInfo*));

    for (i = 0; i < archive_info->blockcount; i++) {
      if (archive_info->blocks[i] == index) {
        archive_info->blocks[i] = -1;
      } else if (archive_info->blocks[i] > index) {
        archive_info->blocks[i]--;
      }
    }
  }
}

uint64_t archiveinfo_used_bytes (struct ArchiveInfo* archive_info) {
  uint64_t used_bytes = 0;

  uint64_t i;
  for (i = 0; i < archive_info->num_files; i++) {
    used_bytes += archive_info->file_infos[i]->size;
  }

  return used_bytes;
}

uint64_t archiveinfo_free_bytes (struct ArchiveInfo* archive_info) {
  return (archive_info->blockcount * archive_info->blocksize) - archiveinfo_used_bytes(archive_info);
}

void archiveinfo_free (struct ArchiveInfo* archive_info) {
  free(archive_info->blocks);

  uint64_t i;
  for (i = 0; i < archive_info->num_files; i++) {
    fileinfo_free(archive_info->file_infos[i]);
  }

  free(archive_info->file_infos);

  free(archive_info); 
}

#define ARCHIVE_ALREADY_EXISTS 1
#define ARCHIVE_NOT_WRITEABLE 2
#define ARCHIVE_NOT_READABLE 3
#define ARCHIVE_FILE_ALREADY_EXISTS 4
#define ARCHIVE_FILE_TOO_BIG 5
#define ARCHIVE_FILE_NOT_FOUND 6
#define FILE_NOT_READABLE 7
#define FILE_NOT_WRITEABLE 8

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
  archive->archive_info = archiveinfo_create();

  return archive;
}

int archive_write_archive_info (struct Archive*);
int archive_initialize_store(struct Archive*);
void archive_initialize_paths(struct Archive* archive, const char* archive_path);

/**
 * Initialisiert ein leeres Archiv.
 */
int archive_initialize_empty (struct Archive* archive, const char* archive_path, uint64_t blocksize, uint64_t blockcount) {
  archive_initialize_paths(archive, archive_path);

  archiveinfo_initialize_empty(archive->archive_info, blocksize, blockcount);

  int status = 0;

  if (file_exists(archive->store_file) || file_exists(archive->structure_file)) {
    status = ARCHIVE_ALREADY_EXISTS;
  }

  status == 0 && (status = archive_write_archive_info(archive));
  status == 0 && (status = archive_initialize_store(archive));

  return status;
}

/**
 * Lädt ein Archiv aus den zwei Archivdateien.
 */
int archive_initialize_from_file (struct Archive* archive, const char* archive_path) {
  archive_initialize_paths(archive, archive_path);

  int status = 0;

  FILE* file = fopen(archive->structure_file, "r");

  if (file == NULL || !file_exists(archive->store_file)) {
    status = ARCHIVE_NOT_READABLE;
  } else {
    status = archiveinfo_initialize_from_file(archive->archive_info, file);

    fclose(file);
  }

  return status;
}

/**
 * Schreibt bytes Bytes der Datei file in die num_files Blöcke, die durch
 * blocks indiziert werden.
 */
int archive_write_file_to_blocks (struct Archive* archive, FILE* file, uint64_t bytes, uint64_t* blocks, uint64_t num_blocks) {
  int status = 0;
  struct ArchiveInfo* archive_info = archive->archive_info;
  FILE* store = fopen(archive->store_file, "r+");

  if (store == NULL) {
    status = ARCHIVE_NOT_WRITEABLE;
  } else {
    char buffer[archive_info->blocksize];
    uint64_t i;
    for (i = 0; i < num_blocks; i++) {
      int chunk_size;

      if (bytes > archive_info->blocksize) {
        chunk_size = archive_info->blocksize;
        bytes -= archive_info->blocksize;
      } else {
        chunk_size = bytes;
      }

      if (file_read(&buffer, 1, chunk_size, file) != 0) {
        status = FILE_NOT_READABLE;
        break;
      } else if (fseek(store, blocks[i] * archive_info->blocksize, SEEK_SET) != 0) {
        status = ARCHIVE_NOT_WRITEABLE;
        break;
      } else if (file_write(&buffer, 1, chunk_size, store) != 0) {
        status = ARCHIVE_NOT_WRITEABLE;
        break;
      }
    }

    fclose(store);
  }

  return status;
}

/**
 * Fügt dem Archiv die Datei unter dem gegebenen Namen hinzu.
 */
int archive_add_file (struct Archive* archive, const char* name, const char* path) {
  int status = 0;
  struct ArchiveInfo* archive_info = archive->archive_info;

  if (archiveinfo_has_file(archive_info, name)) {
    status = ARCHIVE_FILE_ALREADY_EXISTS;
  } else {
    FILE* file = fopen(path, "r");

    if (file == NULL) {
      status = FILE_NOT_READABLE;
    } else {
      long int size = file_size(file);

      if (size == -1) {
        status = FILE_NOT_READABLE;
      } else {
        uint64_t num_free = archiveinfo_num_free_blocks(archive_info);
        uint64_t num_needed = archiveinfo_needed_blocks(archive_info, size);

        if (num_free < num_needed) {
          status = ARCHIVE_FILE_TOO_BIG;
        } else {
          uint64_t* free_blocks = malloc(num_needed * sizeof(uint64_t));
          archiveinfo_get_free_blocks(archive_info, free_blocks, num_needed);
          archiveinfo_add_file(archive_info, name, size, free_blocks, num_needed);

          status = archive_write_file_to_blocks(archive, file, size, free_blocks, num_needed);
          status == 0 && (status = archive_write_archive_info(archive));

          free(free_blocks);
        }
      }

      fclose(file);
    }
  }

  return status;
}

int archive_get_file (struct Archive* archive, const char* name, const char* output_path) {
  int status = 0;
  struct ArchiveInfo* archive_info = archive->archive_info;

  if (!archiveinfo_has_file(archive_info, name)) {
    status = ARCHIVE_FILE_NOT_FOUND;
  } else {
    int num_blocks = archiveinfo_get_num_allocated_blocks(archive_info, name);
    uint64_t* blocks = malloc(num_blocks * sizeof(uint64_t));
    archiveinfo_get_allocated_blocks(archive_info, name, blocks);

    FILE* store = fopen(archive->store_file, "r");

    if (store == NULL) {
      status = ARCHIVE_NOT_READABLE;
    } else {
      FILE* output = fopen(output_path, "w");

      if (output == NULL) {
        status = FILE_NOT_WRITEABLE;
      } else {
        uint64_t bytes_left = archiveinfo_get_file_size(archive_info, name);
        char buffer[archive_info->blocksize];
        int i;
        for (i = 0; i < num_blocks; i++) {
          int position = blocks[i] * archive_info->blocksize;
          int chunk_size = archive_info->blocksize;

          if (bytes_left > archive_info->blocksize) {
            chunk_size = archive_info->blocksize;
            bytes_left -= archive_info->blocksize;
          } else {
            chunk_size = bytes_left;
          }

          fseek(store, position, SEEK_SET);
          if (file_read(&buffer, 1, chunk_size, store) != 0) {
            status = ARCHIVE_NOT_READABLE;
            break;
          } else if (file_write(&buffer, 1, chunk_size, output) != 0) {
            status = FILE_NOT_WRITEABLE;
            break;
          }
        }

        fclose (output);
      }

      fclose(store);
    }

    free(blocks);
  }

  return status;
}

int archive_delete_file (struct Archive* archive, const char* name) {
  int status = 0;
  struct ArchiveInfo* archive_info = archive->archive_info;

  if (!archiveinfo_has_file(archive_info, name)) {
    status = ARCHIVE_FILE_NOT_FOUND;
  } else {
    archiveinfo_delete_file(archive_info, name);
    status = archive_write_archive_info(archive);
  }

  return status;
}

int archive_free_bytes (struct Archive* archive) {
  return archiveinfo_free_bytes(archive->archive_info);
}

int archive_used_bytes (struct Archive* archive) {
  return archiveinfo_used_bytes(archive->archive_info);
}

void archive_print_list (struct Archive* archive) {
  struct ArchiveInfo* archive_info = archive->archive_info;

  uint64_t i;
  for (i = 0; i < archive_info->num_files; i++) {
    struct FileInfo* file_info = archive_info->file_infos[i];
    uint64_t num_blocks = archiveinfo_get_num_allocated_blocks(archive_info, file_info->name);
    uint64_t* blocks = malloc(num_blocks * sizeof(uint64_t));
    archiveinfo_get_allocated_blocks(archive_info, file_info->name, blocks);

    printf("%s,%ld,%lu", file_info->name, file_info->size, num_blocks);

    uint64_t j;
    for (j = 0; j < num_blocks; j++) {
      printf(",%lu", blocks[j]);
    }

    printf("\n");

    free(blocks);
  }
}

/**
 * Vertauscht den Block i mit i + 1.
 * 
 * @private
 */
int archive_swap_blocks (struct Archive* archive, FILE* store, uint64_t i) {
  int status = 0;

  int tmp = archive->archive_info->blocks[i];
  archive->archive_info->blocks[i] = archive->archive_info->blocks[i + 1];
  archive->archive_info->blocks[i + 1] = tmp;

  uint64_t blocksize = archive->archive_info->blocksize;

  char buffer[blocksize];
  char buffer2[blocksize];

  if (fseek(store, i * blocksize, SEEK_SET) != 0) {
    status = ARCHIVE_NOT_READABLE;
  } else if (file_read(&buffer, 1, blocksize, store) != 0) {
    status = ARCHIVE_NOT_READABLE;
  } else if (file_read(&buffer2, 1, blocksize, store) != 0) {
    status = ARCHIVE_NOT_READABLE;
  } else if (fseek(store, i * blocksize, SEEK_SET) != 0) {
    status = ARCHIVE_NOT_READABLE;
  } else if (file_write(&buffer2, 1, blocksize, store) != 0) {
    status = ARCHIVE_NOT_WRITEABLE;
  } else if (file_write(&buffer, 1, blocksize, store) != 0) {
    status = ARCHIVE_NOT_WRITEABLE;
  }

  return status;
}

/**
 * Drückt den Block right an die Position left, wobei alle Blöcke dazwischen um
 * 1 nach rechts verschoben werden.
 *
 * @private
 */
int archive_push_block (struct Archive* archive, FILE* store, uint64_t left, uint64_t right) {
  int status = 0;

  if (right > left) {
    uint64_t i;
    for (i = right - 1; i >= left && status == 0; i--) {
      status = archive_swap_blocks(archive, store, i);
    }
  }

  return status;
}

int archive_defrag (struct Archive* archive) {
  int status = 0;
  struct ArchiveInfo* archive_info = archive->archive_info;
  FILE* store = fopen(archive->store_file, "r+");

  if (store == NULL) {
    status = ARCHIVE_NOT_READABLE;
  } else {
    uint64_t block_index = 0;
    uint64_t i;
    for (i = 0; i < archive_info->num_files; i++) {
      uint64_t j;
      for (j = block_index; j < archive_info->blockcount && status == 0; j++) {
        if (archive_info->blocks[j] == (int64_t)i) {
          status = archive_push_block(archive, store, block_index, j);
          block_index++;
        }
      }
    }

    status == 0 && (status = archive_write_archive_info(archive));
    
    fclose(store);
  }

  return status;
}

/**
 * Erzeugt die beiden speziellen Pfade aus dem allgemeinen Archivpfad.
 *
 * @private
 */
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
    status = archiveinfo_write(archive->archive_info, structure_file);

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

    uint64_t i;
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
  archiveinfo_free(archive->archive_info);

  free(archive->structure_file);
  free(archive->store_file);
  free(archive);
}

int cli_create (const char* archive_path, uint64_t blocksize, uint64_t blockcount) {
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

  struct Archive* archive = archive_create();
  status = archive_initialize_from_file(archive, archive_path);
  status == 0 && (status = archive_add_file(archive, target, source_path));
  archive_free(archive);

  switch (status) {
    case ARCHIVE_NOT_READABLE:
      printf("Das Archiv ist nicht lesbar");
      return 2;
    case ARCHIVE_FILE_ALREADY_EXISTS:
      printf("Eine Datei mit dem Namen %s existiert bereits", target);
      return 11;
    case ARCHIVE_FILE_TOO_BIG:
      printf("Die Datei %s passt nicht mehr in das Archiv", target);
      return 12;
    case FILE_NOT_READABLE:
      printf("Die Datei %s ist nicht lesbar", source_path);
      return 13;
    default:
      return 0;
  }
}

int cli_get (const char* archive_path, const char* name, const char* output_path) {
  int status = 0;

  struct Archive* archive = archive_create();
  status = archive_initialize_from_file(archive, archive_path);
  status == 0 && (status = archive_get_file(archive, name, output_path));
  archive_free(archive);

  switch (status) {
    case ARCHIVE_NOT_READABLE:
      printf("Das Archiv ist nicht lesbar");
      return 2;
    case ARCHIVE_FILE_NOT_FOUND:
      printf("Die gesuchte Datei ist nicht im Archiv");
      return 21;
    case FILE_NOT_WRITEABLE:
      printf("Die Ausgabedatei konnte nicht erstellt werden");
      return 30;
    default:
      return 0;
  }
}

int cli_del (const char* archive_path, const char* name) {
  int status = 0;

  struct Archive* archive = archive_create();
  status = archive_initialize_from_file(archive, archive_path);
  status == 0 && (status = archive_delete_file(archive, name));
  archive_free(archive);

  switch (status) {
    case ARCHIVE_NOT_READABLE:
      printf("Das Archiv ist nicht lesbar");
      return 2;
    case ARCHIVE_FILE_NOT_FOUND:
      printf("Die Datei ist nicht im Archiv");
      return 21;
    default:
      return 0;
  }
}

int cli_free (const char* archive_path) {
  int status = 0;

  struct Archive* archive = archive_create();
  status = archive_initialize_from_file(archive, archive_path);

  int free_bytes = archive_free_bytes(archive);

  archive_free(archive);

  switch (status) {
    case ARCHIVE_NOT_READABLE:
      printf("Das Archiv ist nicht lesbar");
      return 2;
    default:
      printf("%d", free_bytes);
      return 0;
  }
}

int cli_used (const char* archive_path) {
  int status = 0;

  struct Archive* archive = archive_create();
  status = archive_initialize_from_file(archive, archive_path);

  int used_bytes = archive_used_bytes(archive);

  archive_free(archive);

  switch (status) {
    case ARCHIVE_NOT_READABLE:
      printf("Das Archiv ist nicht lesbar");
      return 2;
    default:
      printf("%d", used_bytes);
      return 0;
  }
}

int cli_list (const char* archive_path) {
  int status = 0;

  struct Archive* archive = archive_create();
  status = archive_initialize_from_file(archive, archive_path);
  archive_print_list(archive);
  archive_free(archive);

  switch (status) {
    case ARCHIVE_NOT_READABLE:
      printf("Das Archiv ist nicht lesbar");
      return 2;
    default:
      return 0;
  }
}

int cli_defrag (const char* archive_path) {
  int status = 0;

  struct Archive* archive = archive_create();
  status = archive_initialize_from_file(archive, archive_path);
  status == 0 && (status = archive_defrag(archive));
  archive_free(archive);

  switch (status) {
    case ARCHIVE_NOT_WRITEABLE:
    case ARCHIVE_NOT_READABLE:
      printf("Das Archiv ist nicht les-/schreibbar");
      return 2;
    default:
      return 0;
  }
}

void help_create () {
  printf("USAGE: vfs ARCHIVE create BLOCKSIZE BLOCKCOUNT");
}

void help_add () {
  printf("USAGE: vfs ARCHIVE add SOURCE TARGET");
}

void help_get () {
  printf("USAGE: vfs ARCHIVE get SOURCE OUTPUT");
}

void help_del () {
  printf("USAGE: vfs ARCHIVE del TARGET");
}

void help_free () {
  printf("USAGE: vfs ARCHIVE free");
}

void help_used () {
  printf("USAGE: vfs ARCHIVE used");
}

void help_list () {
  printf("USAGE: vfs ARCHIVE list");
}

void help_defrag () {
  printf("USAGE: vfs ARCHIVE defrag");
}

void help () {
  help_create();
  help_add();
  help_get();
  help_del();
  help_free();
  help_used();
  help_list();
  help_defrag();
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
  } else if (strcmp(command, "add") == 0) {
    if (argc < 5) {
      help_add();
      return 66;
    }

    return cli_add(archive_path, argv[3], argv[4]);
  } else if (strcmp(command, "get") == 0) {
    if (argc < 5) {
      help_get();
      return 66;
    }

    return cli_get(archive_path, argv[3], argv[4]);
  } else if (strcmp(command, "del") == 0) {
    if (argc < 4) {
      help_del();
      return 66;
    }

    return cli_del(archive_path, argv[3]);
  } else if (strcmp(command, "free") == 0) {
    return cli_free(archive_path);
  } else if (strcmp(command, "used") == 0) {
    return cli_used(archive_path);
  } else if (strcmp(command, "list") == 0) {
    return cli_list(archive_path);
  } else if (strcmp(command, "defrag") == 0) {
    return cli_defrag(archive_path);
  } else {
    printf("Der Befehl ist ungültig");
    help();
    return 66;
  }
}
