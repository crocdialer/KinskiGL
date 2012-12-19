#ifndef _included_kinski_file_functions_
#define _included_kinski_file_functions_

#include "Exception.h"

#include <string>
#include <vector>

namespace kinski {

    /// read a complete file into a string
    //int getFileSize(const std::string &theFilename); // boosted
    
    bool fileExists(const std::string &theFilename);
    const std::string readFile(const std::string & theUTF8Filename);
    bool readBinaryFile(const std::string &theUTF8Filename, std::vector<char> &theContent);
    std::string getFilenamePart(const std::string &theFileName);
    std::string getDirectoryPart(const std::string &theFileName);
    void getDirectoryEntries(const std::string &thePath,  std::vector<std::string> &theDirEntries,
                             const std::string &theFilter);
    bool readFileLineByLine(const std::string &theUTF8Filename, std::vector<std::string> &theContent);
    bool searchFile(const std::string &theFileName, std::string &retPath);
    std::string lastFileNamePart(const char *file_name);
    std::string getExtension(const std::string &thePath);
    std::string removeExtension(const std::string &theFileName);
    std::string file_string(const char* file_name);
    /************************ Exceptions ************************/
    
    class FileNotFoundException: public Exception
    {
    public:
        FileNotFoundException(const std::string &theFilename) :
        Exception(std::string("File not found: ") + theFilename) {}
    };
    
    class OpenDirectoryFailed: public Exception
    {
    public:
        OpenDirectoryFailed(const std::string &theFilename) :
        Exception(std::string("File not found: ") + theFilename) {}
    };
    
    class OpenFileFailed: public Exception
    {
    public:
        OpenFileFailed(const std::string &theFilename) :
        Exception(std::string("File not found: ") + theFilename) {}
    };
}

#endif

