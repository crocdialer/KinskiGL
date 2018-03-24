// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "file_functions.hpp"
#include <fstream>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost::filesystem;

namespace kinski { namespace fs{

    /////////// implementation internal /////////////

    namespace
    {
        std::set<path> g_searchPaths;
    }

    std::string expand_user(std::string path)
    {
        path = trim(path);
        
        if(!path.empty() && path[0] == '~')
        {
            if(path.size() != 1 && path[1] != '/') return path; // or other error handling ?
            char const* home = getenv("HOME");
            if(home || ((home = getenv("USERPROFILE"))))
            {
                path.replace(0, 1, home);
            }
            else
            {
                char const *hdrive = getenv("HOMEDRIVE"),
                *hpath = getenv("HOMEPATH");
                if(!(hdrive && hpath)) return path; // or other error handling ?
                path.replace(0, 1, std::string(hdrive) + hpath);
            }
        }
        return path;
    }

    /////////// end implemantation internal /////////////

///////////////////////////////////////////////////////////////////////////////
    
    const std::set<fs::path>& get_search_paths()
    {
        return g_searchPaths;
    }

///////////////////////////////////////////////////////////////////////////////
    
    void add_search_path(const fs::path &thePath, int the_recursion_depth)
    {
        boost::filesystem::path path_expanded (expand_user(thePath));
        
        if (!boost::filesystem::exists(path_expanded))
        {
            LOG_DEBUG << "directory " << path_expanded << " not existing";
            return;
        }

        if(the_recursion_depth)
        {
            g_searchPaths.insert(get_directory_part(path_expanded.string()));
            recursive_directory_iterator it;
            try
            {
                it = recursive_directory_iterator(path_expanded);
                recursive_directory_iterator end;

                while(it != end)
                {
                    if(is_directory(*it)) g_searchPaths.insert(canonical(it->path()).string());
                    try{ ++it; }
                    catch(std::exception& e)
                    {
                        // e.g. no permission
                        LOG_ERROR << e.what();
                        it.no_push();
                        try { ++it; } catch(...)
                        {
                            LOG_ERROR << "Got trouble in recursive directory iteration: "<<it->path();
                            return;
                        }
                    }
                }
            }
            catch(boost::filesystem::filesystem_error &e)
            {
                LOG_ERROR<<e.what();
            }
        }
        else
        {
            g_searchPaths.insert(canonical(path_expanded).string());
        }
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    void clear_search_paths()
    {
        g_searchPaths.clear();
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    vector<string> get_directory_entries(const fs::path &thePath, const std::string &theExtension,
                                         int the_recursion_depth)
    {
        vector<string> ret;
        path p(expand_user(thePath));
        
        auto check_file_status = [](const boost::filesystem::file_status &s) -> bool
        {
            return  s.type() == boost::filesystem::regular_file ||
                    s.type() == boost::filesystem::symlink_file ||
                    s.type() == boost::filesystem::character_file ||
                    s.type() == boost::filesystem::block_file;
        };
        
        try
        {
            if(exists(p))    // does p actually exist?
            {
                if(the_recursion_depth)
                {
                    recursive_directory_iterator it(p), end;
                    while(it != end)
                    {
                        if(check_file_status(it->status()))
                        {
                            if(theExtension.empty())
                            {
                                ret.push_back(it->path().string());
                            }
                            else
                            {
                                string ext = it->path().extension().string();
                                if(!ext.empty()) ext = ext.substr(1);
                                if(theExtension == ext){ ret.push_back(it->path().string()); }
                            }
                        }

                        try{ ++it; }
                        catch(std::exception& e)
                        {
                            // e.g. no permission
                            LOG_ERROR << e.what();
                            it.no_push();
                            ++it;
                        }
                    }
                }
                else
                {
                    directory_iterator it(p), end;
                    while(it != end)
                    {
                        if(check_file_status(it->status()))
                        {
                            if(theExtension.empty())
                            {
                                ret.push_back(it->path().string());
                            }
                            else
                            {
                                string ext = it->path().extension().string();
                                if(!ext.empty()) ext = ext.substr(1);
                                if(theExtension == ext){ ret.push_back(it->path().string()); }
                            }
                        }

                        try{ ++it; }
                        catch(std::exception& e)
                        {
                            LOG_ERROR<<e.what();
                        }
                    }
                }
            }
            else{ LOG_TRACE << p << " does not exist"; }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR<<e.what();
        }
        return ret;
    }

///////////////////////////////////////////////////////////////////////////////
    
    std::vector<string> get_directory_entries(const fs::path &thePath, FileType the_type,
                                              int the_recursion_depth)
    {
        auto ret = get_directory_entries(thePath, "", the_recursion_depth);
        ret.erase(std::remove_if(ret.begin(), ret.end(), [the_type](const std::string &f)
        {
            return get_file_type(f) != the_type;
        }), ret.end());
        return ret;
    }

///////////////////////////////////////////////////////////////////////////////
    
    size_t get_file_size(const fs::path &the_file_name)
    {
        return boost::filesystem::file_size(expand_user(the_file_name));
    }

///////////////////////////////////////////////////////////////////////////////
    
    /// read a complete file into a string
    const std::string read_file(const fs::path & theUTF8Filename)
    {
        string path = search_file(theUTF8Filename);
        std::ifstream inStream(path);

        if(!inStream.is_open()){ throw OpenFileFailed(path); }
        return string((istreambuf_iterator<char>(inStream)), istreambuf_iterator<char>());
    }

///////////////////////////////////////////////////////////////////////////////
    
    std::vector<uint8_t> read_binary_file(const fs::path &theUTF8Filename)
    {
        fs::path path = search_file(theUTF8Filename);
        std::ifstream inStream(path, ios::in | ios::binary | ios::ate);

        if(!inStream.good())
        {
            throw OpenFileFailed(theUTF8Filename);
        }
        std::vector<uint8_t> content;
        content.reserve(inStream.tellg());
        inStream.seekg(0);
        
        content.insert(content.end(), (istreambuf_iterator<char>(inStream)),
                       istreambuf_iterator<char>());
        return content;
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    bool write_file(const fs::path &the_file_name, const std::string &the_data)
    {
        std::ofstream file_out(expand_user(the_file_name));
        if(!file_out){ return false; }
        file_out << the_data;
        file_out.close();
        return true;
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    bool write_file(const fs::path &the_file_name, const std::vector<uint8_t> &the_data)
    {
        std::ofstream file_out(expand_user(the_file_name), ios::out | ios::binary);
        if(!file_out){ return false; }
        file_out.write(reinterpret_cast<const char*>(&the_data[0]), the_data.size());
        file_out.close();
        return true;
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    bool append_to_file(const fs::path &the_file_name, const std::string &the_data)
    {
        std::ofstream file_out(expand_user(the_file_name), ios::out | ios::app);
        if(!file_out){ return false; }
        file_out << the_data;
        file_out.close();
        return true;
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    std::string get_filename_part(const fs::path &the_file_name)
    {
        return boost::filesystem::path(expand_user(the_file_name)).filename().string();
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    bool is_uri(const fs::path &the_str)
    {
        auto result = the_str.find("://");
        
        if(result == std::string::npos || result == 0){ return false; }
        
        for(size_t i = 0; i < result; ++i)
        {
            if(!isalpha(the_str[i])){ return false; }
        }
        return true;
    }
    
///////////////////////////////////////////////////////////////////////////////
    
    bool is_directory(const fs::path &the_file_name)
    {
        return boost::filesystem::is_directory(expand_user(the_file_name));
    }

///////////////////////////////////////////////////////////////////////////////
    
    bool exists(const fs::path& the_file_name)
    {
        return boost::filesystem::exists(expand_user(the_file_name));
    }

///////////////////////////////////////////////////////////////////////////////
    
    bool create_directory(const fs::path &the_file_name)
    {
        if(!exists(the_file_name))
        {
            try
            {
                return boost::filesystem::create_directory(the_file_name);
            } catch (boost::filesystem::filesystem_error &e){ LOG_ERROR << e.what(); }
        }
        return false;
    }

///////////////////////////////////////////////////////////////////////////////
    
    std::string join_paths(const fs::path &p1, const fs::path &p2)
    {
        return (path(p1) / path(p2)).string();
    }

///////////////////////////////////////////////////////////////////////////////

    std::string path_as_uri(const fs::path &p)
    {
        if(is_uri(p)) return p;
        return "file://" + canonical(expand_user(p)).string();
    }

///////////////////////////////////////////////////////////////////////////////

    fs::path search_file(const fs::path &the_file_name, bool use_entire_path)
    {
        auto trim_file_name = trim(the_file_name);
        
        std::string expanded_name = use_entire_path ? expand_user(trim_file_name) : get_filename_part(trim_file_name);
        boost::filesystem::path ret_path(expanded_name);
        
        try
        {
            if(ret_path.is_absolute() && is_regular_file(ret_path))
            {
                return ret_path.string();
            }
            std::set<std::string>::const_iterator it = get_search_paths().begin();
            
            for(; it != get_search_paths().end(); ++it)
            {
                ret_path = path(*it) / path(expanded_name);
                if(boost::filesystem::exists(ret_path) && is_regular_file(ret_path))
                {
                    LOG_TRACE_2 << "found '" << trim_file_name << "' as: " << ret_path.string();
                    return ret_path.string();
                }
            }
        }
        catch(boost::filesystem::filesystem_error& e){ LOG_DEBUG << e.what(); }
        
        
        if(use_entire_path){ return search_file(the_file_name, false); }
        throw FileNotFoundException(the_file_name);
    }

///////////////////////////////////////////////////////////////////////////////
    
    fs::path get_working_directory()
    {
        return boost::filesystem::current_path().string();
    }

///////////////////////////////////////////////////////////////////////////////
    
    fs::path get_directory_part(const std::string &the_file_name)
    {
        auto expanded_path = expand_user(the_file_name);
        
        if(is_directory(expanded_path))
            return boost::filesystem::path(expanded_path).string();
        else
            return boost::filesystem::path(expanded_path).parent_path().string();
    }

///////////////////////////////////////////////////////////////////////////////
    
    std::string get_extension(const fs::path &the_file_name)
    {
        return boost::filesystem::extension(expand_user(the_file_name));
    }

///////////////////////////////////////////////////////////////////////////////
    
    std::string remove_extension(const fs::path &the_file_name)
    {
        return boost::filesystem::path(expand_user(the_file_name)).replace_extension().string();
    }

///////////////////////////////////////////////////////////////////////////////
    
    FileType get_file_type(const fs::path &file_name)
    {
        if(!exists(file_name)){ return FileType::NOT_A_FILE; }
        if(is_directory(file_name)){ return FileType::DIRECTORY; }
        string ext = get_extension(file_name);
        ext = ext.empty() ? ext : kinski::to_lower(ext.substr(1));

        const std::list<string>
        image_exts{"png", "jpg", "jpeg", "bmp", "tga"},
        audio_exts{"wav", "m4a", "mp3"},
        model_exts{"obj", "dae", "3ds", "ply", "md5mesh", "fbx"},
        movie_exts{"mpg", "mov", "avi", "mp4", "m4v", "mkv"},
        font_exts{"ttf", "otf", "ttc"};

        if(kinski::contains(image_exts, ext)){ return FileType::IMAGE; }
        else if(kinski::contains(model_exts, ext)){ return FileType::MODEL; }
        else if(kinski::contains(audio_exts, ext)){ return FileType::AUDIO; }
        else if(kinski::contains(movie_exts, ext)){ return FileType::MOVIE; }
        else if(kinski::contains(font_exts, ext)){ return FileType::FONT; }

        return FileType::OTHER;
    }
    
///////////////////////////////////////////////////////////////////////////////
    
}}// namespaces kinski / fs
