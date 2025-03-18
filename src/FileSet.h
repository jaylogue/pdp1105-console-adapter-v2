#ifndef BUILT_IN_FILE_SET_H
#define BUILT_IN_FILE_SET_H

struct FileHeader;

class FileSet final
{
public:
    static void Init(void);
    static size_t NumFiles(void);
    static bool GetFile(size_t index, const char *& fileName, const uint8_t *& data, size_t& len);
    static const char * GetFileName(size_t index);

private:
    static const FileHeader * sFileIndex[MAX_FILES];
    static size_t sNumFiles;
};

#endif // BUILT_IN_FILE_SET_H
