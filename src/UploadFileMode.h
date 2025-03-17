#ifndef UPLOAD_FILE
#define UPLOAD_FILE

extern uint8_t gUploadedFile[];
extern size_t gUploadedFileLen;

extern bool UploadFileMode(Port& uiPort);
extern void TrimXMODEMPadding(void);

#endif //  UPLOAD_FILE
