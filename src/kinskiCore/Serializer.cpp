//
//  Serializer.cpp
//  kinskiGL
//
//  Created by Fabian on 11/4/12.
//
//

#include "Serializer.h"
#include <fstream>

#define PROPERTY_TYPE         "type"
#define PROPERTY_VALUE        "value"
#define PROPERTY_VALUES       "values"
#define PROPERTY_TYPE_FLOAT   "float"
#define PROPERTY_TYPE_STRING  "string"
#define PROPERTY_TYPE_DOUBLE  "double"
#define PROPERTY_TYPE_BOOLEAN "bool"
#define PROPERTY_TYPE_INT     "int"
#define PROPERTY_TYPE_UNKNOWN "unknown"
#define PROPERTY_NAME         "name"
#define PROPERTIES            "properties"

using namespace std;

namespace kinski {
    
    
    const string readFile(const std::string &path)
    {
        
        ifstream inStream(path.c_str());
        if(!inStream.good())
        {
            throw FileNotFoundException(path);
        }
        
        return string ((istreambuf_iterator<char>(inStream)),
                       istreambuf_iterator<char>());
    }

    bool PropertyIO::readPropertyValue(const Property::Ptr &theProperty,
                                       Json::Value &theJsonValue) const
    {
        bool success = false;
        
        if (theProperty->isOfType<float>()) {
            theJsonValue[PROPERTY_TYPE]  = PROPERTY_TYPE_FLOAT;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<float>();
            success = true;
            
        } else if (theProperty->isOfType<std::string>()) {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_STRING;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<std::string>();
            success = true;
            
        } else if (theProperty->isOfType<int>()) {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_INT;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<int>();
            success = true;
            
        } else if (theProperty->isOfType<double>()) {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_DOUBLE;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<double>();
            success = true;
            
        } else if (theProperty->isOfType<bool>()) {
            theJsonValue[PROPERTY_TYPE] = PROPERTY_TYPE_BOOLEAN;
            theJsonValue[PROPERTY_VALUE] = theProperty->getValue<bool>();
            success = true;
            
        } 
        
        return success;
    }
    
    bool PropertyIO::writePropertyValue(Property::Ptr &theProperty,
                                        const Json::Value &theJsonValue) const
    {
        bool success = false;
        
        if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_FLOAT)
        {
            theProperty->setValue<float>(theJsonValue[PROPERTY_VALUE].asDouble());
            success = true;
            
        } else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_DOUBLE) {
            theProperty->setValue<double>(theJsonValue[PROPERTY_VALUE].asDouble());
            success = true;
            
        } else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_INT) {
            theProperty->setValue<int>(theJsonValue[PROPERTY_VALUE].asInt());
            success = true;
            
        } else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_STRING) {
            theProperty->setValue<std::string>(theJsonValue[PROPERTY_VALUE].asString());
            success = true;
            
        } else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_BOOLEAN) {
            theProperty->setValue<bool>(theJsonValue[PROPERTY_VALUE].asBool());
            success = true;
            
        } else if (theJsonValue[PROPERTY_TYPE].asString() == PROPERTY_TYPE_UNKNOWN) {
            // do nothing
        }
        
        return success;
    }
    
    std::string serializeComponent(const Component::Ptr &theComponent, const PropertyIO &theIO)
    {
        Json::Value myRoot;
        
        int myIndex = 0;
        int myVIndex = 0;
        
        std::string myName = "component";
        
        myRoot[myIndex][PROPERTY_NAME] = myName;
        
        std::list<Property::Ptr> myProperties = theComponent->getPropertyList();
        std::list<Property::Ptr>::const_iterator myPIt;
        
        for ( myPIt = myProperties.begin(); myPIt != myProperties.end(); myPIt++ ) {
            const Property::Ptr &myProperty = *myPIt;
            std::string myPropName = myProperty->getName();
            
            myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_NAME] = myPropName;
            
            // delegate reading to PropertyIO object
            if(! theIO.readPropertyValue(myProperty, myRoot[myIndex][PROPERTIES][myVIndex]))
            {
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_TYPE] = PROPERTY_TYPE_UNKNOWN;
            }

            myVIndex++;
        }
        
        myIndex++;
        myVIndex = 0;
        
        Json::StyledWriter myWriter;
        return myWriter.write(myRoot); 
    }
    
    void saveComponentState(const Component::Ptr &theComponent, const std::string &theFileName,
                            const PropertyIO &theIO)
    {
        std::string state = serializeComponent(theComponent);
        
        std::ofstream myFileOut(theFileName.c_str());
        
        if(!myFileOut)
        {
            throw OutputFileException(theFileName);
        }
        
        myFileOut << state;
        myFileOut.close();
    }
    
    void loadComponentState(Component::Ptr theComponent, const std::string &theFileName,
                            const PropertyIO &theIO)
    {
        std::string myConfigString = readFile(theFileName);
        
        Json::Reader myReader;
        Json::Value myRoot;
        
        bool myParsingSuccessful = myReader.parse(myConfigString, myRoot);
        if (!myParsingSuccessful)
        {
            throw ParsingException(myConfigString);
        }
        
        for (unsigned int i=0; i<myRoot.size(); i++)
        {
            Json::Value myComponentNode = myRoot[i];

            for (unsigned int i=0; i < myComponentNode[PROPERTIES].size(); i++)
            {
                try
                {
                    std::string myName = myComponentNode[PROPERTIES][i][PROPERTY_NAME].asString();
                    Property::Ptr myProperty = theComponent->getPropertyByName(myName);
                    theIO.writePropertyValue(myProperty, myComponentNode[PROPERTIES][i]);
                    
                } catch (PropertyNotFoundException &myException)
                {
                    //LOG(WARNING) << myException.getMessage();
                }
            }
        }
    }

}//namespace