#include "Decklink.h"
#include "DeckLinkAPI.h"
#include <mutex>

namespace kinski{ namespace decklink{

    class Input::Impl : public IDeckLinkInputCallback
    {
    public:
        
        Impl()
        {
            IDeckLinkIterator*		iterator;
            
            iterator = CreateDeckLinkIteratorInstance();
            if (iterator)
            {
                if(iterator->Next(&m_dl) != S_OK){ LOG_ERROR << "could not open DeckLink device"; }
                iterator->Release();
            }
        }
        
        HRESULT	STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) override
        {
            CFUUIDBytes		iunknown;
            HRESULT			result = E_NOINTERFACE;
            
            // Initialise the return result
            *ppv = nullptr;
            
            // Obtain the IUnknown interface and compare it the provided REFIID
            iunknown = CFUUIDGetUUIDBytes(IUnknownUUID);
            if(memcmp(&iid, &iunknown, sizeof(REFIID)) == 0)
            {
                *ppv = this;
                AddRef();
                result = S_OK;
            }
            else if(memcmp(&iid, &IID_IDeckLinkInputCallback, sizeof(REFIID)) == 0)
            {
                *ppv = (IDeckLinkInputCallback*)this;
                AddRef();
                result = S_OK;
            }
            
            return result;
        }
        
        ULONG STDMETHODCALLTYPE	AddRef() override
        {
            return ++m_ref_count;
        }
        
        ULONG STDMETHODCALLTYPE	Release() override
        {
            int32_t		newRefValue;
            
            newRefValue = --m_ref_count;
            if(newRefValue == 0)
            {
                delete this;
                return 0;
            }
            return newRefValue;
        }
        
        virtual HRESULT VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents,
                                                IDeckLinkDisplayMode* newDisplayMode,
                                                BMDDetectedVideoInputFormatFlags detectedSignalFlags) override
        {
//            newDisplayMode->GetName(CFStringRef *name);
            LOG_DEBUG << "new display mode: ";
            return 1;
        }
        
        virtual HRESULT VideoInputFrameArrived(IDeckLinkVideoInputFrame* arrivedFrame,
                                               IDeckLinkAudioInputPacket*) override
        {
            m_last_frame.width = arrivedFrame->GetWidth();
            m_last_frame.height = arrivedFrame->GetHeight();
            m_last_frame.bytes_per_pixel = arrivedFrame->GetRowBytes() / m_last_frame.width;
            
            {
                // mutex lock
                std::unique_lock<std::mutex> lock(m_mutex);
                
                m_last_frame.buffer.resize(m_last_frame.width * m_last_frame.height *
                                           m_last_frame.bytes_per_pixel);
                arrivedFrame->GetBytes((void**)(&m_last_frame.buffer[0]));
                
                // mutex unlock
                
            }
            
            m_new_frame = true;
            
            return 1;
        }
        
        void start_capture()
        {
            HRESULT theResult;
            
            // Turn on video input
            theResult = m_input->SetCallback(this);
            
            if(theResult != S_OK)
            {
                LOG_ERROR << "SetCallback failed with result %08x" << (unsigned int)theResult;
            }
            //
            theResult = m_input->EnableVideoInput(m_displaymode, m_pixel_format, 0);
            
            if(theResult != S_OK)
            {
                LOG_ERROR << "EnableVideoInput failed with result " << (unsigned int)theResult;
            }
            
            // Sart the input stream running
            theResult = m_input->StartStreams();
            
            if(theResult != S_OK)
            {
                LOG_ERROR << "Input StartStreams failed with result %08x" << (unsigned int)theResult;
            }
        }
        
        void stop_capture()
        {
            HRESULT theResult;
            theResult = m_input->StopStreams();
            
            if(theResult != S_OK)
            {
                LOG_ERROR << "StopStreams failed with result %08x" << (unsigned int)theResult;
            }
            
            theResult = m_input->DisableVideoInput();
        }
        
//    private:
        bool m_capturing, m_new_frame;
        int32_t m_ref_count;
        
        IDeckLink *m_dl;
        IDeckLinkInput *m_input;
        BMDDisplayMode m_displaymode = bmdModeHD1080p2997;
        BMDPixelFormat m_pixel_format = bmdFormat8BitBGRA;
        
        std::mutex m_mutex;
        
        struct Frame
        {
            uint32_t width, height, bytes_per_pixel;
            std::vector<uint8_t> buffer;
        } m_last_frame;
    };
    
    Input::Input():m_impl(new Impl)
    {
    
    }
    
    Input::~Input()
    {
        
    }
    
    bool Input::copy_frame_to_texture(gl::Texture &the_texture)
    {
        return false;
    }
    
    bool Input::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
    {
        if(!m_impl->m_new_frame){ return false; }
        
        if(width){ *width = m_impl->m_last_frame.width; }
        if(height){ *height = m_impl->m_last_frame.height; }
        data = m_impl->m_last_frame.buffer;
        m_impl->m_new_frame = false;
        
        return true;
    }
}}