// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef _included_kinski_file_functions_
#define _included_kinski_file_functions_

#include "Definitions.h"
#include "Exception.h"

namespace kinski {

    /// read a complete file into a string
    //int getFileSize(const std::string &theFilename); // boosted
    
    // manage known file locations
    const std::list<std::string>& getSearchPaths();
    void addSearchPath(const std::string &thePath);
    
    bool fileExists(const std::string &theFilename);
    const std::string readFile(const std::string & theUTF8Filename);
    std::vector<uint8_t> readBinaryFile(const std::string &theUTF8Filename);
    std::string getFilenamePart(const std::string &theFileName);
    std::string getDirectoryPart(const std::string &theFileName);
    void getDirectoryEntries(const std::string &thePath,  std::vector<std::string> &theDirEntries,
                             const std::string &theFilter);
    bool readFileLineByLine(const std::string &theUTF8Filename, std::vector<std::string> &theContent);
    std::string searchFile(const std::string &theFileName);
    std::string lastFileNamePart(const std::string &theFileName);
    std::string getExtension(const std::string &thePath);
    std::string removeExtension(const std::string &theFileName);
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
        OpenDirectoryFailed(const std::string &theDir) :
        Exception(std::string("Could not open directory: ") + theDir) {}
    };
    
    class OpenFileFailed: public Exception
    {
    public:
        OpenFileFailed(const std::string &theFilename) :
        Exception(std::string("Could not open file: ") + theFilename) {}
    };
}

#endif

