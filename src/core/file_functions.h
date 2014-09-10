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

    int getFileSize(const std::string &theFilename);
    
    // manage known file locations
    const std::set<std::string>& getSearchPaths();
    void addSearchPath(const std::string &thePath, bool recursive = false);
    void clearSearchPaths();
    
    std::list<std::string> getDirectoryEntries(const std::string &thePath,
                                               bool recursive = false,
                                               const std::string &theExtension = "");
    bool fileExists(const std::string &theFilename);
    bool isDirectory(const std::string &theFilename);
    const std::string readFile(const std::string &theUTF8Filename);
    std::vector<uint8_t> readBinaryFile(const std::string &theUTF8Filename);
    std::string getFilenamePart(const std::string &theFileName);
    std::string getDirectoryPart(const std::string &theFileName);
    std::vector<std::string> readFileLineByLine(const std::string &theUTF8Filename);
    std::string searchFile(const std::string &theFileName);
    std::string get_working_directory();
    std::string getExtension(const std::string &thePath);
    std::string removeExtension(const std::string &theFileName);
    
    enum class FileType{FILE_IMAGE, FILE_MODEL, FILE_AUDIO, FILE_OTHER};
    
    FileType get_filetype(const std::string &file_name);
    
    /************************ Exceptions ************************/
    
    class FileNotFoundException: public Exception
    {
    private:
        std::string m_file_name;
    public:
        FileNotFoundException(const std::string &theFilename) :
        Exception(std::string("File not found: ") + theFilename),
        m_file_name(theFilename){}
        std::string file_name() const { return m_file_name;}
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

