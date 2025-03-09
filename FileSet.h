#ifndef BUILT_IN_FILE_SET_H
#define BUILT_IN_FILE_SET_H

class FileSet final
{
public:
    static void Init(void);
    static void ShowMenu(Port& uiPort);
    static bool IsValidFileKey(char fileKey);
    static bool GetFile(char fileKey, const char *& fileName, const uint8_t *& data, size_t& len);
};

#endif // BUILT_IN_FILE_SET_H
