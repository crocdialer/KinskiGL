//
//  Serializer.h
//  kinskiGL
//
//  Created by Fabian on 11/4/12.
//
//

#ifndef __kinskiGL__Serializer__
#define __kinskiGL__Serializer__

#include "json/json.h"
#include "Component.h"

namespace kinski
{
    const std::string readFile(const std::string &path);
    
    /*!
     * Delegate object to handle all known types
     * Can be provided by user to add support for arbitrary data formats
     */
    class PropertyIO
    {
    public:
        virtual ~PropertyIO(){};
        virtual bool readPropertyValue(const Property::Ptr &theProperty,
                                       Json::Value &theJsonValue) const;
        virtual bool writePropertyValue(Property::Ptr &theProperty,
                                        const Json::Value &theJsonValue) const;
    };
    
    /*!
     * Save a component´s state to file using json file formatting
     */
    void saveComponentState(const Component::Ptr &theComponent, const std::string &theFileName,
                            const PropertyIO &theIO = PropertyIO());
    
    /*!
     * Read a component´s state from a json-file
     */
    void loadComponentState(Component::Ptr theComponent, const std::string &theFileName,
                            const PropertyIO &theIO = PropertyIO());

    /*!
     * Serialize a component to a string in json format. 
     * Supported Property types are determined by theIO object
     */
    std::string serializeComponent(const Component::Ptr &theComponent,
                                   const PropertyIO &theIO = PropertyIO());
    
    /************************ Exceptions ************************/
    
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
