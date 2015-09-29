// Raspian includes
#include "bcm_host.h"
#include "ilclient.h"

#include "gl/Texture.h"
#include "gl/Buffer.h"
#include "MovieController.h"

namespace kinski{ namespace video{
    
    struct MovieControllerImpl
    {
        std::string m_src_path;
        bool m_playing;
        bool m_loop;
        float m_rate;
        
        MovieController::MovieCallback m_on_load_cb, m_movie_ended_cb;

        MovieControllerImpl():
        m_src_path(""),
        m_playing(false),
        m_loop(false),
        m_rate(1.f)
        {

        }
        ~MovieControllerImpl()
        {

        };
    };
    
    MovieControllerPtr MovieController::create()
    {
        return MovieControllerPtr(new MovieController());
    }
    
    MovieControllerPtr MovieController::create(const std::string &filePath, bool autoplay,
                                               bool loop)
    {
        return MovieControllerPtr(new MovieController(filePath, autoplay, loop));
    }
    
    MovieController::MovieController():
    m_impl(new MovieControllerImpl)
    {
        
    }
    
    MovieController::MovieController(const std::string &filePath, bool autoplay, bool loop):
    m_impl(new MovieControllerImpl)
    {
        load(filePath, autoplay, loop);
    }
    
    MovieController::~MovieController()
    {

    }

    void MovieController::load(const std::string &filePath, bool autoplay, bool loop)
    {
   
    }
    void MovieController::play()
    {
        LOG_TRACE << "starting movie playback";
        //[m_impl->m_player play];
        //[m_impl->m_player setRate: m_impl->m_rate];
        m_impl->m_playing = true;
    }
    
    void MovieController::unload()
    {
        m_impl.reset(new MovieControllerImpl);
        m_impl->m_playing = false;
    }
    
    void MovieController::pause()
    {
        //[m_impl->m_player pause];
        //[m_impl->m_player setRate: 0.f];
        m_impl->m_playing = false;
    }
    
    bool MovieController::isPlaying() const
    {
        return m_impl->m_playing;
    }
    
    void MovieController::restart()
    {
        //[m_impl->m_player seekToTime:kCMTimeZero];
        play();
    }
    
    float MovieController::volume() const
    {
        return 0.f;//m_impl->m_player.volume;
    }
    
    void MovieController::set_volume(float newVolume)
    {
        float val = clamp(newVolume, 0.f, 1.f);
        //m_impl->m_player.volume = val;
    }
    
    bool MovieController::copy_frame(std::vector<uint8_t>& data, int *width, int *height)
    {
        return false;
    }
    
    bool MovieController::copy_frame_to_texture(gl::Texture &tex, bool as_texture2D)
    {
      return false;
    }
    
    bool MovieController::copy_frames_offline(gl::Texture &tex, bool compress)
    {
        LOG_WARNING << "implementation not available on RPI"
        return false;
    }
    
    double MovieController::duration() const
    {
        return 0.f;
    }
    
    double MovieController::current_time() const
    {
        return CMTimeGetSeconds([m_impl->m_player currentTime]);
    }
    
    void MovieController::seek_to_time(float value)
    {
        //CMTime t = CMTimeMakeWithSeconds(value, NSEC_PER_SEC);
        //[m_impl->m_player seekToTime:t];
        //[m_impl->m_player setRate: m_impl->m_rate];
    }
    
    void MovieController::set_loop(bool b)
    {

    }
    
    bool MovieController::loop() const
    {
        return m_impl->m_loop;
    }
    
    void MovieController::set_rate(float r)
    {
        m_impl->m_rate = r;
        //[m_impl->m_player setRate: r];
    }
    
    const std::string& MovieController::get_path() const
    {
        return m_impl->m_src_path;
    }
    
    void MovieController::set_on_load_callback(MovieCallback c)
    {
        m_impl->m_on_load_cb = c;
    }
    
    void MovieController::set_movie_ended_callback(MovieCallback c)
    {
        m_impl->m_movie_ended_cb = c;
    }
}}// namespaces
