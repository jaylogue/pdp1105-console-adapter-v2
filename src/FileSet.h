/*
 * Copyright 2024-2025 Jay Logue
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
