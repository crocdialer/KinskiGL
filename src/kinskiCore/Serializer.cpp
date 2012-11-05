//
//  Serializer.cpp
//  kinskiGL
//
//  Created by Fabian on 11/4/12.
//
//

#include "Serializer.h"
#include "json/json.h"
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
    
    void setValue(Json::Value &theNode, Component::Ptr &theComponent)
    {
        try
        {
            std::string myName = theNode[PROPERTY_NAME].asString();
            Property::Ptr myProperty = theComponent->getPropertyByName(myName);
            
            if (theNode[PROPERTY_TYPE].asString() == PROPERTY_TYPE_FLOAT)
            {
                myProperty->setValue<float>(theNode[PROPERTY_VALUE].asDouble());
                
            } else if (theNode[PROPERTY_TYPE].asString() == PROPERTY_TYPE_DOUBLE) {
                myProperty->setValue<double>(theNode[PROPERTY_VALUE].asDouble());
                
            } else if (theNode[PROPERTY_TYPE].asString() == PROPERTY_TYPE_INT) {
                myProperty->setValue<int>(theNode[PROPERTY_VALUE].asInt());
                
            } else if (theNode[PROPERTY_TYPE].asString() == PROPERTY_TYPE_STRING) {
                myProperty->setValue<std::string>(theNode[PROPERTY_VALUE].asString());
                
            } else if (theNode[PROPERTY_TYPE].asString() == PROPERTY_TYPE_BOOLEAN) {
                myProperty->setValue<bool>(theNode[PROPERTY_VALUE].asBool());
                
            } else if (theNode[PROPERTY_TYPE].asString() == PROPERTY_TYPE_UNKNOWN) {
                // do nothing
            }
        } catch (PropertyNotFoundException &myException)
        {
            //LOG(WARNING) << myException.getMessage();
        }
    }
    
    std::string getState(const Component::Ptr &theComponent)
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
            
            if (myProperty->isOfType<float>()) {
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_TYPE]  = PROPERTY_TYPE_FLOAT;
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_VALUE] = myProperty->getValue<float>();
                
            } else if (myProperty->isOfType<std::string>()) {
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_TYPE] = PROPERTY_TYPE_STRING;
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_VALUE] = myProperty->getValue<std::string>();
                
            } else if (myProperty->isOfType<int>()) {
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_TYPE] = PROPERTY_TYPE_INT;
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_VALUE] = myProperty->getValue<int>();
                
            } else if (myProperty->isOfType<double>()) {
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_TYPE] = PROPERTY_TYPE_DOUBLE;
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_VALUE] = myProperty->getValue<double>();
                
            } else if (myProperty->isOfType<bool>()) {
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_TYPE] = PROPERTY_TYPE_BOOLEAN;
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_VALUE] = myProperty->getValue<bool>();
                
            } else {
                myRoot[myIndex][PROPERTIES][myVIndex][PROPERTY_TYPE] = PROPERTY_TYPE_UNKNOWN;
            }
            
            myVIndex++;
        }
        
        myIndex++;
        myVIndex = 0;
        
        Json::StyledWriter myWriter;
        return myWriter.write(myRoot); 
    }
    
    void saveComponentState(const Component::Ptr &theComponent, const std::string &theFileName)
    {
        std::string state = getState(theComponent);
        
        std::ofstream myFileOut(theFileName.c_str());
        
        if(!myFileOut)
        {
            throw OutputFileException(theFileName);
        }
        
        myFileOut << state;
        myFileOut.close();
    }
    
    void loadComponentState(Component::Ptr theComponent, const std::string &theFileName)
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
                setValue(myComponentNode[PROPERTIES][i], theComponent);
            }
        }
    }

}//namespace