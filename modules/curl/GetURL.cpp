//
//  GOMTalking.cpp
//  MrProximity
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "GetURL.h"
#include <string.h>
#include "curl/curl.h"

using namespace std;

namespace kinski{ namespace net{

class Action
{
protected:
    std::vector<uint8_t> m_response;
 
    /*!
     * Callback to process incoming data
     */
    static size_t write_save(void *buffer, size_t size, size_t nmemb,
                                 void *userp)
    {
        size_t realsize = size * nmemb;
        if (userp)
        {
            Action *ourAction = static_cast<Action*>(userp);
            uint8_t* buf_start = (uint8_t*)(buffer);
            uint8_t* buf_end = buf_start + realsize;
            ourAction->m_response.insert(ourAction->m_response.end(), buf_start, buf_end);
        }
        return realsize;
    }
    
    /*!
     * Callback for data provided to Curl for sending
     */
    static size_t read_static(void *ptr, size_t size, size_t nmemb,
                                  void *inStream)
    {
        size_t length = strlen((char*) inStream);
        memcpy(ptr, inStream, length);
        return length;
    }

    
public:
    Action(){};
    virtual ~Action()
    {};
    
    const std::vector<uint8_t>& getResponse(){return m_response;};
    
    virtual bool performWithHandle(CURL* curlHandle) = 0;
};

    
// Timeout interval for http requests
const long GetURL::DEFAULT_TIMEOUT = 1L;

class GetURLAction: public Action
{
private:
	string m_nodeUrl;
public:
	GetURLAction(const string &the_url) :
    m_nodeUrl(the_url)
	{
	}

	virtual bool performWithHandle(CURL *curlHandle)
	{
		/* specify target URL, and note that this URL should include a file
		 name, not only a directory */
		curl_easy_setopt(curlHandle, CURLOPT_URL, m_nodeUrl.c_str());
		curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_save);
		CURLcode curlResult = curl_easy_perform(curlHandle);
		return !curlResult;
	}
};

GetURL::GetURL() :
        m_maxQueueSize(50),
        m_timeout(DEFAULT_TIMEOUT),
        m_stop(false)
{
//    m_thread = std::thread(std::bind(&GetURL::run, this));
}
    
GetURL::~GetURL()
{
	stop();
}

std::vector<uint8_t> GetURL::getURL(const std::string &the_url)
{
	ActionPtr getNodeAction = make_shared<GetURLAction>(the_url);
    
	CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, m_timeout);
	getNodeAction->performWithHandle(handle);
	curl_easy_cleanup(handle);

	return getNodeAction->getResponse();
}

void GetURL::addAction(const ActionPtr & theAction)
{
	std::unique_lock<std::mutex> lock(m_mutex);
    
	m_actionQueue.push_back(theAction);
    if(m_actionQueue.size() > m_maxQueueSize)
    {
        m_actionQueue.pop_front();
        //printf("dropping action...\n");
    }
	m_conditionVar.notify_one();
}

//void GetURL::setNumWorkerThreads(unsigned int n)
//{
//	stop();
//	m_stop = false;
//    m_thread = std::thread(std::bind(this, &GetURL::run));
//}

void GetURL::run()
{
	// init curl-handle for this thread
	CURL *handle = curl_easy_init();

	while (!m_stop)
	{
		ActionPtr action;
        
        curl_easy_reset(handle);

		// locked scope
		{
			std::unique_lock<std::mutex> lock(m_mutex);
            
            curl_easy_setopt(handle, CURLOPT_TIMEOUT, m_timeout);
            
			while (m_actionQueue.empty() && !m_stop)
				m_conditionVar.wait(lock);

			if (m_stop)
				break;

			action = m_actionQueue.front();
			m_actionQueue.pop_front();
		}

		// go for gold
		action->performWithHandle(handle);
	}

	//cleanup
	curl_easy_cleanup(handle);
}

long GetURL::getTimeOut()
{
    return m_timeout;
}
    
void GetURL::setTimeOut(long t)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_timeout=t;
}
    
}}// namespace