// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "file_functions.hpp"
#include <fstream>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost::filesystem;

namespace kinski {

    /////////// implementation internal /////////////

    static std::set<std::string> g_searchPaths;

    std::string expand_user(std::string path)
    {
        if (!path.empty() && path[0] == '~')
        {
            if(path.size() != 1 && path[1] != '/') return path; // or other error handling ?
            char const* home = getenv("HOME");
            if (home || ((home = getenv("USERPROFILE"))))
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

    const std::set<std::string>& get_search_paths()
    {
        return g_searchPaths;
    }

    void add_search_path(const std::string &thePath, bool recursive)
    {
        boost::filesystem::path path_expanded (expand_user(thePath));
        if (!boost::filesystem::exists(path_expanded))
        {
            LOG_DEBUG<<"directory "<<path_expanded<<" not existing";
            return;
        }

        if(recursive)
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
                        LOG_ERROR<<e.what();
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

    void clear_search_paths()
    {
        g_searchPaths.clear();
    }

    list<string> get_directory_entries(const std::string &thePath, const std::string &theExtension,
                                       bool recursive)
    {
        list<string> ret;
        path p (expand_user(thePath));

        try
        {
            if (exists(p))    // does p actually exist?
            {
                if(recursive)
                {
                    recursive_directory_iterator it(p), end;
                    while(it != end)
                    {
                        if(boost::filesystem::is_regular_file(*it))
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
                            LOG_ERROR<<e.what();
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
                        auto f_stat(it->status());
                        if(f_stat.type() == boost::filesystem::regular_file ||
                           f_stat.type() == boost::filesystem::symlink_file ||
                           f_stat.type() == boost::filesystem::character_file ||
                           f_stat.type() == boost::filesystem::block_file)
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
            else{ LOG_DEBUG<< p <<" does not exist"; }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR<<e.what();
        }
        return ret;
    }

    std::list<string> get_directory_entries(const std::string &thePath, FileType the_type,
                                            bool recursive)
    {
        auto ret = get_directory_entries(thePath, "", recursive);
        ret.erase(std::remove_if(ret.begin(), ret.end(), [the_type](const std::string &f)
        {
            return get_file_type(f) != the_type;
        }), ret.end());
        return ret;
    }

    int get_file_size(const std::string &theFilename)
    {
        return boost::filesystem::file_size(theFilename);
    }

    /// read a complete file into a string
    const std::string read_file(const std::string & theUTF8Filename)
    {
        string path = search_file(theUTF8Filename);
        std::ifstream inStream(path.c_str());

        if(!inStream.good())
        {
            throw OpenFileFailed(path);
        }
        return string ((istreambuf_iterator<char>(inStream)),
                       istreambuf_iterator<char>());
    }

    std::vector<uint8_t> read_binary_file(const std::string &theUTF8Filename)
    {
        string path = search_file(theUTF8Filename);
        std::ifstream inStream(path.c_str());

        if(!inStream.good())
        {
            throw OpenFileFailed(theUTF8Filename);
        }
        std::vector<uint8_t> content;
        content.insert(content.end(), (istreambuf_iterator<char>(inStream)),
                       istreambuf_iterator<char>());
        return content;
    }

    std::vector<std::string> read_file_line_by_line(const std::string &theUTF8Filename)
    {
        std::vector<std::string> ret;
        const size_t MAX_LENGTH = 1000;
        char buffer[MAX_LENGTH];
        std::string newPart;
        std::string filepath = search_file(theUTF8Filename);
        FILE *file;
        if ((file = fopen(filepath.c_str(), "rb")) == NULL) {
            throw OpenFileFailed("Error opening file " + theUTF8Filename);
        }
        size_t size = fread(buffer, 1, MAX_LENGTH,file);
        bool endedWithNewLine = false;
        while (size > 0) {
            newPart = std::string(buffer, size);
            std::stringstream stream(newPart);
            std::string item;
            bool first = true;
            while (std::getline(stream, item, '\n')) {
                if (first && !endedWithNewLine && ret.size() >0) {
                    ret.back().append(item);
                } else {
                    ret.push_back(item);
                }
                first = false;
            }
            endedWithNewLine = (item.size() == 0);
            size = fread(buffer, 1, MAX_LENGTH,file);
        }
        fclose(file);
        return ret;
    }

    std::string get_filename_part(const std::string &theFileName)
    {
        return path(theFileName).filename().string();
    }

    bool is_directory(const std::string &theFilename)
    {
        return boost::filesystem::is_directory(expand_user(theFilename));
    }

    bool file_exists(const std::string& theFilename)
    {
        return boost::filesystem::exists(theFilename);
    }

    bool create_directory(const std::string &theFilename)
    {
        if(!file_exists(theFilename))
        {
            try
            {
                return boost::filesystem::create_directory(theFilename);
            } catch (boost::filesystem::filesystem_error &e){ LOG_ERROR << e.what(); }
        }
        return false;
    }

    std::string join_paths(const std::string &p1, const std::string &p2)
    {
        return (path(p1) / path(p2)).string();
    }

    std::string search_file(const std::string &theFileName, bool use_entire_path)
    {
        std::string expanded_name = use_entire_path ? expand_user(theFileName) : get_filename_part(theFileName);
        boost::filesystem::path ret_path(expanded_name);

        if(ret_path.is_absolute() && is_regular_file(ret_path))
        {
            return ret_path.string();
        }
        std::set<std::string>::const_iterator it = get_search_paths().begin();
        for (; it != get_search_paths().end(); ++it)
        {
            ret_path = path(*it) / path(expanded_name);
            if (boost::filesystem::exists(ret_path) && is_regular_file(ret_path))
            {
                LOG_TRACE<<"found '"<<theFileName<<"' as: "<<ret_path.string();
                return ret_path.string();
            }
        }
        if(use_entire_path){ return search_file(theFileName, false); }
        throw FileNotFoundException(theFileName);
    }

    std::string get_working_directory()
    {
        return boost::filesystem::current_path().string();
    }

    std::string get_directory_part(const std::string &theFileName)
    {
        if(is_directory(theFileName))
            return path(theFileName).string();
        else
            return path(theFileName).parent_path().string();
    }

    std::string get_extension(const std::string &thePath)
    {
        return boost::filesystem::extension(thePath);
    }

    std::string remove_extension(const std::string &theFileName)
    {
        return path(theFileName).replace_extension().string();
    }

    FileType get_file_type(const std::string &file_name)
    {
        if(!file_exists(file_name)){ return FileType::NOT_A_FILE; }
        if(is_directory(file_name)){ return FileType::DIRECTORY; }
        string ext = kinski::get_extension(file_name);
        ext = ext.empty() ? ext : kinski::to_lower(ext.substr(1));

        std::list<string>
        image_exts{"png", "jpg", "jpeg", "bmp", "tga"},
        audio_exts{"wav", "m4a", "mp3"},
        model_exts{"obj", "dae", "3ds", "ply", "md5mesh", "fbx"},
        movie_exts{"mpg", "mov", "avi", "mp4", "m4v"},
        font_exts{"ttf", "otf", "ttc"};

        if(kinski::is_in(ext, image_exts)){ return FileType::IMAGE; }
        else if(kinski::is_in(ext, model_exts)){ return FileType::MODEL; }
        else if(kinski::is_in(ext, audio_exts)){ return FileType::AUDIO; }
        else if(kinski::is_in(ext, movie_exts)){ return FileType::MOVIE; }
        else if(kinski::is_in(ext, font_exts)){ return FileType::FONT; }

        return FileType::OTHER;
    }
}
