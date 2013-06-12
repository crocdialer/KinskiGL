// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "file_functions.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include "Exception.h"
#include "Logger.h"

using namespace std;
using namespace boost::filesystem;

namespace kinski {
    
    /////////// implemantation internal /////////////
    
    std::list<std::string> g_searchPaths;
    
    std::string expand_user(std::string path)
    {
        if (not path.empty() and path[0] == '~')
        {
            if(path.size() != 1 && path[1] != '/') return path; // or other error handling ?
            char const* home = getenv("HOME");
            if (home or ((home = getenv("USERPROFILE"))))
            {
                path.replace(0, 1, home);
            }else
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
    
    const std::list<std::string>& getSearchPaths()
    {
        return g_searchPaths;
    }
    
    void addSearchPath(const std::string &thePath, bool recursive)
    {
        boost::filesystem::path path_expanded (expand_user(thePath));
        if (!boost::filesystem::exists(path_expanded))
        {
            LOG_DEBUG<<"directory "<<path_expanded<<" not existing";
            return;
        }
            
        if(recursive)
        {
            g_searchPaths.push_back(getDirectoryPart(path_expanded.string()));
            recursive_directory_iterator it;
            try
            {
                it = recursive_directory_iterator(path_expanded);
                recursive_directory_iterator end;
                
                while(it != end)
                {
                    if(is_directory(*it)) g_searchPaths.push_back(canonical(it->path()).string());
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
            g_searchPaths.push_back(canonical(path_expanded).string());
        }
    }
    
    list<string> getDirectoryEntries(const std::string &thePath, bool recursive,
                                     const std::string &theExtension)
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
                            LOG_ERROR<<e.what();
                        }
                    }
                }
            }
            else
                LOG_DEBUG<< p <<" does not exist";
        }
        catch (const std::exception& e)
        {
            LOG_ERROR<<e.what();
        }
        return ret;
    }
    
    int getFileSize(const std::string &theFilename)
    {
        return boost::filesystem::file_size(theFilename);
    }

    /// read a complete file into a string
    const std::string readFile(const std::string & theUTF8Filename)
    {
        string path = searchFile(theUTF8Filename);
        ifstream inStream(path.c_str());
        
        if(!inStream.good())
        {
            throw OpenFileFailed(path);
        }
        return string ((istreambuf_iterator<char>(inStream)),
                       istreambuf_iterator<char>());
    }

    std::vector<uint8_t> readBinaryFile(const std::string &theUTF8Filename)
    {
        string path = searchFile(theUTF8Filename);
        ifstream inStream(path.c_str());
        
        if(!inStream.good())
        {
            throw OpenFileFailed(theUTF8Filename);
        }
        std::vector<uint8_t> content;
        content.insert(content.end(), (istreambuf_iterator<char>(inStream)),
                       istreambuf_iterator<char>());
        return content;
    }
    
    std::vector<std::string> readFileLineByLine(const std::string &theUTF8Filename)
    {
        std::vector<std::string> ret;
        const size_t MAX_LENGTH = 1000;
        char buffer[MAX_LENGTH];
        std::string newPart;
        std::string filepath = searchFile(theUTF8Filename);
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

    std::string getFilenamePart(const std::string &theFileName)
    {
        return path(theFileName).filename().string();
    }
    
    bool isDirectory(const std::string &theFilename)
    {
        return boost::filesystem::is_directory(theFilename);
    }
    
    bool fileExists(const std::string& theFilename)
    {
        return boost::filesystem::exists(theFilename);
    }
    
    std::string searchFile(const std::string &theFileName)
    {
        std::string expanded_name = expand_user(theFileName);
        boost::filesystem::path ret_path(expanded_name);
        
        if(ret_path.is_absolute() && is_regular_file(ret_path))
        {
            return ret_path.string();
        }
        std::list<std::string>::const_iterator it = getSearchPaths().begin();
        for (; it != getSearchPaths().end(); ++it)
        {
            ret_path = path(*it) / path(expanded_name);
            if (boost::filesystem::exists(ret_path))
            {
                LOG_TRACE<<"found '"<<theFileName<<"' as: "<<ret_path.string();
                return ret_path.string();
            }
        }
        throw FileNotFoundException(theFileName);
    }
    
    std::string get_working_directory()
    {
        return boost::filesystem::current_path().string();
    }
    
    std::string getDirectoryPart(const std::string &theFileName)
    {
        if(is_directory(theFileName))
            return path(theFileName).string();
        else
            return path(theFileName).parent_path().string();
    }

    std::string getExtension(const std::string &thePath)
    {
        return boost::filesystem::extension(thePath);
    }

    std::string removeExtension(const std::string &theFileName)
    {
        return path(theFileName).replace_extension().string();
    }

}

