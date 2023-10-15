#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct {
    HANDLE file;
    HANDLE mapping;
    size_t size;
    void* data;
} FileMap;

static FileMap open_file_map(const char* filepath) {
    HANDLE file = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE) {
        return (FileMap){ INVALID_HANDLE_VALUE };
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size)) {
        fprintf(stderr, "Could not read file size! %s", filepath);
        return (FileMap){ INVALID_HANDLE_VALUE };
    }

    HANDLE mapping = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, 0);
    if (!mapping) {
        fprintf(stderr, "Could not map file! %s", filepath);
        return (FileMap){ INVALID_HANDLE_VALUE };
    }

    void* memory = MapViewOfFileEx(mapping, FILE_MAP_READ, 0, 0, 0, 0);
    if (!memory) {
        fprintf(stderr, "Could not view mapped file! %s", filepath);
        return (FileMap){ INVALID_HANDLE_VALUE };
    }

    return (FileMap){
        .file = file,
        .mapping = mapping,
        .size = size.QuadPart,
        .data = memory
    };
}

static void close_file_map(FileMap* file_map) {
    UnmapViewOfFile(file_map->data);
    CloseHandle(file_map->mapping);
    CloseHandle(file_map->file);
}
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
    int fd;
    size_t size;
    void* data;
} FileMap;

static FileMap open_file_map(const char* filepath) {
    int fd = open(filepath, O_RDONLY);

    struct stat file_stats;
    if (fstat(fd, &file_stats) == -1) {
        return (FileMap){ 0 };
    }

    void* buffer = mmap(NULL, file_stats.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED) {
        return (FileMap){ 0 };
    }

    return (FileMap){ fd, file_stats.st_size, buffer };
}

static void close_file_map(FileMap* file_map) {
    munmap(file_map->data, file_map->size);
    close(file_map->fd);
}
#endif
