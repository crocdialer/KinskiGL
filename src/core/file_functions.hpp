// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "core/core.hpp"

namespace kinski { namespace fs {

    using path = std::string;
    
    enum class FileType{IMAGE, MODEL, AUDIO, MOVIE, DIRECTORY, FONT, OTHER, NOT_A_FILE};
    
    int get_file_size(const std::string &the_file_name);
    
    // manage known file locations
    const std::set<std::string>& get_search_paths();
    void add_search_path(const std::string &thePath, int the_recursion_depth = 0);
    void clear_search_paths();
    
    std::list<std::string> get_directory_entries(const std::string &thePath,
                                                 const std::string &theExtension = "",
                                                 int the_recursion_depth = 0);
    std::list<string> get_directory_entries(const std::string &thePath, FileType the_type,
                                            int the_recursion_depth = 0);
    
    bool exists(const std::string &the_file_name);
    bool is_uri(const std::string &the_file_name);
    bool is_directory(const std::string &the_file_name);
    bool create_directory(const std::string &the_file_name);
    std::string join_paths(const std::string &p1, const std::string &p2);
    std::string path_as_uri(const std::string &p);
    const std::string read_file(const std::string &theUTF8Filename);
    std::vector<uint8_t> read_binary_file(const std::string &theUTF8Filename);
    bool write_file(const std::string &the_file_name, const std::string &the_data);
    bool write_file(const std::string &the_file_name, const std::vector<uint8_t> &the_data);
    bool append_to_file(const std::string &the_file_name, const std::string &the_data);
    std::string get_filename_part(const std::string &the_file_name);
    std::string get_directory_part(const std::string &the_file_name);
    std::string search_file(const std::string &the_file_name, bool use_entire_path = true);
    std::string get_working_directory();
    std::string get_extension(const std::string &thePath);
    std::string remove_extension(const std::string &the_file_name);
    
    FileType get_file_type(const std::string &file_name);
    
    /************************ Exceptions ************************/
    
    class FileNotFoundException: public Exception
    {
    private:
        std::string m_file_name;
    public:
        FileNotFoundException(const std::string &the_file_name) :
        Exception(std::string("File not found: ") + the_file_name),
        m_file_name(the_file_name){}
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
        OpenFileFailed(const std::string &the_file_name) :
        Exception(std::string("Could not open file: ") + the_file_name) {}
        virtual ~OpenFileFailed() noexcept {};
    };
}}
