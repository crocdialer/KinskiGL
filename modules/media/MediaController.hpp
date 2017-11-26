#pragma once

#include "gl/gl.hpp"

/*
* This class controls playback of media-files or streams and manages their assets.
*/

namespace kinski{ namespace media{

DEFINE_CLASS_PTR(MediaController)
    
class MediaController : public std::enable_shared_from_this<MediaController>
{
public:

    enum class RenderTarget{SCREEN, TEXTURE};
    enum class AudioTarget{AUTO, HDMI, AUDIO_JACK, BOTH};

    typedef std::function<void(MediaControllerPtr the_movie)> callback_t;

    static MediaControllerPtr create();
    static MediaControllerPtr create(const std::string &filePath, bool autoplay = false,
                                     bool loop = false,
                                     RenderTarget the_render_target = RenderTarget::TEXTURE,
                                     AudioTarget the_audio_target = AudioTarget::HDMI);


    virtual ~MediaController();

    void load(const std::string &the_path, bool autoplay = false, bool loop = false,
              RenderTarget the_target = RenderTarget::TEXTURE,
              AudioTarget the_audio_target = AudioTarget::HDMI);
    void unload();
    bool is_loaded() const;
    bool has_video() const;
    bool has_audio() const;
    void play();
    void restart();
    void pause();
    bool is_playing() const;
    void seek_to_time(double value);
    double duration() const;
    double current_time() const;
    double fps() const;
    float volume() const;
    void set_volume(float volume);
    bool loop() const;
    void set_loop(bool b);
    float rate() const;
    void set_rate(float r);
    const std::string& path() const;

    RenderTarget render_target() const;
    AudioTarget audio_target() const;

    void set_on_load_callback(callback_t c);
    void set_media_ended_callback(callback_t c);

    /*!
     * upload the current frame to the provided gl::Texture.
     * @param[out] the_texture
     * the texture object to be populated with the new frame, if any
     * @param as_texture2D
     * force the target of the returned texture to always be GL_TEXTURE_2D, rather than GL_RECTANGLE.
     * the texture will be converted, if necessary.
     * @return true if a new frame has been uploaded successfully, false otherwise
     */
    bool copy_frame_to_texture(gl::Texture &the_texture, bool as_texture2D = false);

    /*!
     * copy the current frame to a std::vector<uint8_t>
     * @return true if a new frame has been copied successfully,
     * false otherwise
     */
    bool copy_frame(std::vector<uint8_t>& data, int *width = nullptr, int *height = nullptr);

    /*!
     * copy the current frame to a kinski::ImagePtr
     * @return true if a new frame has been copied successfully,
     * false otherwise
     */
    bool copy_frame_to_image(ImagePtr& the_image);

    /*!
     * upload all frames to a gl::Texture object with target GL_TEXTURE_2D_ARRAY
     * @return true if all frames have been uploaded successfully,
     * false otherwise
     */
    bool copy_frames_offline(gl::Texture &tex, bool compress = false);

private:

    MediaController();
    std::unique_ptr<struct MediaControllerImpl> m_impl;
};
}}// namespaces
