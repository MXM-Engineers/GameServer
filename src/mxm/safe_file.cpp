#include "safe_file.h"
#include <io.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

FILE* SafeFileOpen(const char* path, char* tmpPath, i32 tmpSize)
{
	snprintf(tmpPath, tmpSize, "%s.tmp", path);
	return fopen(tmpPath, "wb");
}

bool SafeFileCommit(FILE* f, const char* tmpPath, const char* path)
{
	fflush(f);
	_commit(_fileno(f));
	fclose(f);
	return MoveFileExA(tmpPath, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

i32 BackupDirectory(const char* dir, const char* backupRoot, i32 keep)
{
	CreateDirectoryA(backupRoot, nullptr);
	SYSTEMTIME st;
	GetLocalTime(&st);
	char target[512];
	snprintf(target, sizeof(target), "%s/%04u%02u%02u_%02u%02u%02u", backupRoot, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	if(!CreateDirectoryA(target, nullptr)) return -1;
	char pattern[512];
	snprintf(pattern, sizeof(pattern), "%s/*", dir);
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA(pattern, &fd);
	i32 copied = 0;
	if(h != INVALID_HANDLE_VALUE) {
		do {
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
			char from[512], to[512];
			snprintf(from, sizeof(from), "%s/%s", dir, fd.cFileName);
			snprintf(to, sizeof(to), "%s/%s", target, fd.cFileName);
			if(CopyFileA(from, to, FALSE)) copied++;
		} while(FindNextFileA(h, &fd));
		FindClose(h);
	}
	char backups[64][MAX_PATH];
	i32 count = 0;
	snprintf(pattern, sizeof(pattern), "%s/*", backupRoot);
	h = FindFirstFileA(pattern, &fd);
	if(h != INVALID_HANDLE_VALUE) {
		do {
			if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.' || count >= 64) continue;
			snprintf(backups[count++], MAX_PATH, "%s", fd.cFileName);
		} while(FindNextFileA(h, &fd));
		FindClose(h);
	}
	for(i32 i = 0; i < count; i++) for(i32 j = i + 1; j < count; j++) {
		if(strcmp(backups[i], backups[j]) > 0) {
			char t[MAX_PATH];
			strcpy(t, backups[i]); strcpy(backups[i], backups[j]); strcpy(backups[j], t);
		}
	}
	for(i32 i = 0; i < count - keep; i++) {
		char old[512];
		snprintf(old, sizeof(old), "%s/%s/*", backupRoot, backups[i]);
		h = FindFirstFileA(old, &fd);
		if(h != INVALID_HANDLE_VALUE) {
			do {
				if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
				char file[512];
				snprintf(file, sizeof(file), "%s/%s/%s", backupRoot, backups[i], fd.cFileName);
				DeleteFileA(file);
			} while(FindNextFileA(h, &fd));
			FindClose(h);
		}
		snprintf(old, sizeof(old), "%s/%s", backupRoot, backups[i]);
		RemoveDirectoryA(old);
	}
	return copied;
}
