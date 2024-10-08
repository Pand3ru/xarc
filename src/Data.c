#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/Structs.h"

struct dirent **retrieve_directory_data(char *path, int *amount_dir_entries) {
  struct dirent **dir_entries;
  *amount_dir_entries = scandir(path, &dir_entries, NULL, alphasort);
  if (*amount_dir_entries < 0) {
    perror("scandir");
    return NULL;
  }
  return dir_entries;
}

struct stat create_file_stat(char *full_path) {
  struct stat fileattr;
  if (stat(full_path, &fileattr) < 0) {
    perror("stat");
    struct stat error_stat;
    error_stat.st_mode = 0;
    return error_stat;
  }
  return fileattr;
}

int is_dot_or_dotdot(char *input) {
  return strcmp(input, ".") == 0 || strcmp(input, "..") == 0;
}

// Use NULL to check for errors in when using this function
char *GenerateDataStream(char *origin_path, size_t *totalsize,
                         size_t *byte_stream_size_offset) {
  int amount_dir_entries;
  char *byte_stream = NULL;
  char *file_name = NULL;
  size_t byte_stream_size = *totalsize;

  struct dirent **dir_entries =
      retrieve_directory_data(origin_path, &amount_dir_entries);
  if (dir_entries == NULL) {
    return NULL;
  }

  for (int i = 0; i < amount_dir_entries; i++) {

    file_name = dir_entries[i]->d_name;
    if (is_dot_or_dotdot(file_name)) {
      free(dir_entries[i]);
      continue;
    }

    size_t full_path_size = strlen(origin_path) + strlen(file_name) + 2;

    char *full_path = malloc(full_path_size);
    if (full_path == NULL) {
      perror("Unable to allocate memory for full_path");
      free(dir_entries);
      return NULL;
    }
    snprintf(full_path, full_path_size, "%s/%s", origin_path, file_name);

    struct stat fileattr = create_file_stat(full_path);
    if (fileattr.st_mode == 0) {
      perror("stat");
      return NULL;
    }

    if (S_ISDIR(fileattr.st_mode)) {
      size_t sub_byte_steam_size = 0;
      size_t offset = byte_stream_size + *byte_stream_size_offset;
      char *sub_byte_stream =
          GenerateDataStream(full_path, &sub_byte_steam_size, &offset);

      if (sub_byte_stream) {
        char *new_byte_steam =
            realloc(byte_stream, byte_stream_size + sub_byte_steam_size);
        if (new_byte_steam == NULL) {
          perror("Reallocation failed");
          free(byte_stream);
          free(dir_entries);
          free(full_path);
          return NULL;
        }
        byte_stream = new_byte_steam;

        memmove(byte_stream + byte_stream_size, sub_byte_stream,
                sub_byte_steam_size);
        byte_stream_size += sub_byte_steam_size;
        free(sub_byte_stream);
      }
    } else {
      FILE *file = fopen(full_path, "rb");
      if (!file) {
        perror("fopen");
        free(dir_entries);
        free(full_path);
        return NULL;
      }

      fseek(file, 0, SEEK_END);
      size_t file_size = ftell(file);
      fseek(file, 0, SEEK_SET);

      size_t path_length = strlen(full_path) + 1;
      FileHeader *header = malloc(sizeof(FileHeader) + path_length);
      if (header == NULL) {
        perror("Unable to allocate memory for header");
        fclose(file);
        free(full_path);
        free(dir_entries);
        return NULL;
      }

      header->mode = fileattr.st_mode & 0x0FFF;
      strcpy(header->file_name, file_name);
      strcpy(header->file_path, full_path);
      header->header_size = sizeof(FileHeader) + path_length;
      header->file_offset = byte_stream_size + header->header_size + file_size +
                            *byte_stream_size_offset;

      char *new_byte_steam =
          realloc(byte_stream, byte_stream_size + header->header_size);
      if (new_byte_steam == NULL) {
        perror("Reallocation failed");
        free(byte_stream);
        free(header);
        free(full_path);
        fclose(file);
        free(dir_entries);
        return NULL;
      }
      byte_stream = new_byte_steam;
      memmove(byte_stream + byte_stream_size, header, header->header_size);
      byte_stream_size += header->header_size;

      free(header);

      char *file_content = malloc(file_size);
      if (!file_content) {
        perror("Unable to allocate memory for file content");
        fclose(file);
        free(full_path);
        free(dir_entries);
        return NULL;
      }

      fread(file_content, 1, file_size, file);
      fclose(file);

      char *new_byte_file_stream =
          realloc(byte_stream, byte_stream_size + file_size);
      if (new_byte_file_stream == NULL) {
        perror("Reallocation failed");
        free(byte_stream);
        free(file_content);
        free(full_path);
        free(dir_entries);
        return NULL;
      }
      byte_stream = new_byte_file_stream;

      memmove(byte_stream + byte_stream_size, file_content, file_size);
      byte_stream_size += file_size;

      free(file_content);
    }
    free(full_path);
    free(dir_entries[i]);
    *totalsize = byte_stream_size;
  }

  free(dir_entries);
  return byte_stream;
}

int CreateDirectories(const char *path) {
  char tmp[256];
  char *p = NULL;
  mode_t mode = 0755;

  snprintf(tmp, sizeof(tmp), "%s", path);

  p = strrchr(tmp, '/');
  if (p != NULL) {
    *p = 0;
  } else {
    return -1;
  }

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      mkdir(tmp, mode);
      *p = '/';
    }
  }
  mkdir(tmp, mode);
  DIR *dir = opendir(tmp);
  if (dir) {
    closedir(dir);
    return 0;
  } else {
    return -1;
  }
}

int CleanUpIfExtractionFails(char *destPath) {
  struct dirent **dir_entries;
  int amount_dir_entries = scandir(destPath, &dir_entries, NULL, alphasort);
  if (amount_dir_entries < 0) {
    perror("scandir");
    return 0;
  }

  for (int i = 0; i < amount_dir_entries; i++) {
    char *file_name = dir_entries[i]->d_name;

    if (is_dot_or_dotdot(file_name)) {
      free(dir_entries[i]);
      continue;
    }

    size_t full_path_size = strlen(destPath) + strlen(file_name) + 2;
    char *full_path = malloc(full_path_size);
    if (full_path == NULL) {
      perror("Unable to allocate memory for full_path");
      free(dir_entries);
      return 0;
    }
    snprintf(full_path, full_path_size, "%s/%s", destPath, file_name);

    struct stat fileattr;

    if (stat(full_path, &fileattr) < 0) {
      perror("stat");
      free(full_path);
      free(dir_entries);
      return 0;
    }

    if (S_ISDIR(fileattr.st_mode)) {
      CleanUpIfExtractionFails(full_path);
      rmdir(full_path);
    } else {
      remove(full_path);
    }
  }
  struct stat fileattr;
  if (stat(destPath, &fileattr) < 0) {
    return 0;
  }
  if (S_ISDIR(fileattr.st_mode)) {
    rmdir(destPath);
  } else {
    remove(destPath);
  }
  return 1;
}

int CreateFileAndWriteContent(FileHeader *header, char *byte_stream,
                              size_t offset, size_t byte_stream_size,
                              char *full_path) {
  int file = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, header->mode);
  if (file == -1) {
    perror("File creation");
    return -1;
  }
  ssize_t bytesWritten =
      write(file, byte_stream + offset + header->header_size,
            byte_stream_size - (byte_stream_size - header->file_offset) -
                offset - header->header_size); /* ;.................;
                                                    o hS    fO     bSs */
  printf("created %s\n", full_path);
  if (bytesWritten == -1) {
    perror("Error writing to file");
    return -1;
  }
  printf("Wrote content for: %s\n", full_path);
  return 1;
}

char *normalizePath(char *basePath, char *relativePath) {
  char *normalizedPath = malloc(strlen(basePath) + strlen(relativePath) + 2);
  if (normalizedPath == NULL) {
    perror("malloc");
    return NULL;
  }
  char *relativePath_mutable = strdup(relativePath);
  char *basePath_mutable = strdup(basePath);

  strcpy(normalizedPath, basePath_mutable);
  printf("BasePath: %s\nRelativePath: %s\n", basePath_mutable,
         relativePath_mutable);

  char *token = strtok(relativePath_mutable, "/");
  while (token) {
    if (strcmp(token, ".") == 0) {
      token = strtok(NULL, "/");
      continue;
    } else if (strcmp(token, "..") == 0) {
      char *lastSlash = strrchr(normalizedPath, '/');
      if (lastSlash) {
        *lastSlash = '\0';
      }
      token = strtok(NULL, "/");
      continue;
    }

    strcat(normalizedPath, "/");
    strcat(normalizedPath, token);
    token = strtok(NULL, "/");
  }

  printf("normalizedPath: %s\n", normalizedPath);
  return normalizedPath;
}

// DestPath should end on a trailing /
int RecreateFromDataStream(char *byte_stream, char *destPath,
                           size_t byte_stream_size) {
  struct stat fileattr;
  if (stat(destPath, &fileattr) >= 0) {
    return -1;
  }
  size_t current_byte = 0;

  while (current_byte < byte_stream_size) {
    printf("-------------------------\n");
    FileHeader *header = (FileHeader *)(byte_stream + current_byte);
    char *full_path = normalizePath(destPath, header->file_path);
    if (full_path == NULL) {
      return -1;
    }
    printf("Creating directory for %s\n", full_path);
    assert(CreateDirectories(full_path) != -1);
    assert(CreateFileAndWriteContent(header, byte_stream, current_byte,
                                     byte_stream_size, full_path) != -1);
    current_byte = header->file_offset;
  }
  return 1;
}
