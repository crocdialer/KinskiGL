// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "core/core.hpp"

namespace kinski {

    enum class FileType{IMAGE, MODEL, AUDIO, MOVIE, DIRECTORY, FONT, OTHER, NOT_A_FILE};
    
    int get_file_size(const std::string &theFilename);
    
    // manage known file locations
    const std::set<std::string>& get_search_paths();
    void add_search_path(const std::string &thePath, bool recursive = false);
    void clear_search_paths();
    
    std::list<std::string> get_directory_entries(const std::string &thePath,
                                               const std::string &theExtension = "",
                                               bool recursive = false);
    std::list<string> get_directory_entries(const std::string &thePath, FileType the_type,
                                            bool recursive = false);
    
    bool file_exists(const std::string &theFilename);
    bool is_directory(const std::string &theFilename);
    bool create_directory(const std::string &theFilename);
    std::string join_paths(const std::string &p1, const std::string &p2);
    const std::string read_file(const std::string &theUTF8Filename);
    std::vector<uint8_t> read_binary_file(const std::string &theUTF8Filename);
    std::string get_filename_part(const std::string &theFileName);
    std::string get_directory_part(const std::string &theFileName);
    std::vector<std::string> read_file_line_by_line(const std::string &theUTF8Filename);
    std::string search_file(const std::string &theFileName, bool use_entire_path = true);
    std::string get_working_directory();
    std::string get_extension(const std::string &thePath);
    std::string remove_extension(const std::string &theFileName);
    
    FileType get_file_type(const std::string &file_name);
    
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
        virtual ~FileNotFoundException() noexcept {};
    };
    
    class OpenDirectoryFailed: public Exception
    {
    public:
        OpenDirectoryFailed(const std::string &theDir) :
        Exception(std::string("Could not open directory: ") + theDir) {}
        virtual ~OpenDirectoryFailed() noexcept {};
    };
    
    class OpenFileFailed: public Exception
    {
    public:
        OpenFileFailed(const std::string &theFilename) :
        Exception(std::string("Could not open file: ") + theFilename) {}
        virtual ~OpenFileFailed() noexcept {};
    };
}