//
//  Serializer.h
//  kinskiGL
//
//  Created by Fabian on 11/4/12.
//
//

#ifndef __kinskiGL__Serializer__
#define __kinskiGL__Serializer__

#include "Component.h"

namespace kinski
{
    const std::string readFile(const std::string &path);
    
    void saveComponentState(const Component::Ptr &theComponent, const std::string &theFileName);
    void loadComponentState(Component::Ptr theComponent, const std::string &theFileName);
    
    class FileNotFoundException: public Exception
    {
    public:
        FileNotFoundException(const std::string &theFilename) :
        Exception(std::string("File not found: ") + theFilename) {}
    };
    
    class FileReadingException: public Exception
    {
    public:
        FileReadingException(const std::string &theFilename) :
        Exception(std::string("Error while reading file: ") + theFilename) {}
    };
    
    class ParsingException: public Exception
    {
    public:
        ParsingException(const std::string &theContentString) :
        Exception(std::string("Error while parsing json string: ") + theContentString) {}
    };
    
    class OutputFileException: public Exception
    {
    public:
        OutputFileException(const std::string &theFilename) :
        Exception(std::string("Could not open file for writing configuration: ") + theFilename) {}
    };
}

#endif /* defined(__kinskiGL__Serializer__) */
