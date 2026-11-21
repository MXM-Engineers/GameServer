#pragma once
#include <common/base.h>
#include <stdio.h>

FILE* SafeFileOpen(const char* path, char* tmpPath, i32 tmpSize);
bool SafeFileCommit(FILE* f, const char* tmpPath, const char* path);
i32 BackupDirectory(const char* dir, const char* backupRoot, i32 keep);
