/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "OMXAudioCodecOMX.h"
#include "utils/PCMRemap.h"
#include "linux/XMemUtils.h"

#include "core/Logger.hpp"

// the size of the audio_render output port buffers
#define AUDIO_DECODE_OUTPUT_BUFFER (32*1024)
static const char rounded_up_channels_shift[] = {0,0,1,2,2,3,3,3,3};

COMXAudioCodecOMX::COMXAudioCodecOMX()
{
  m_pBufferOutput = NULL;
  m_iBufferOutputAlloced = 0;
  m_iBufferOutputUsed = 0;

  m_pCodecContext = NULL;
  m_pConvert = NULL;
  m_bOpenedCodec = false;

  m_channels = 0;
  m_pFrame1 = NULL;
  m_frameSize = 0;
  m_bGotFrame = false;
  m_bNoConcatenate = false;
  m_iSampleFormat = AV_SAMPLE_FMT_NONE;
  m_desiredSampleFormat = AV_SAMPLE_FMT_NONE;
}

COMXAudioCodecOMX::~COMXAudioCodecOMX()
{
  av_free(m_pBufferOutput);
  m_pBufferOutput = NULL;
  m_iBufferOutputAlloced = 0;
  m_iBufferOutputUsed = 0;
  Dispose();
}

bool COMXAudioCodecOMX::Open(COMXStreamInfo &hints, enum PCMLayout layout)
{
  AVCodec* pCodec;
  m_bOpenedCodec = false;

  avcodec_register_all();

  pCodec = avcodec_find_decoder(hints.codec);
  if (!pCodec)
  {
    LOG_TRACE_2 << "COMXAudioCodecOMX::Open() Unable to find codec " << hints.codec;
    return false;
  }

  m_bFirstFrame = true;
  m_pCodecContext = avcodec_alloc_context3(pCodec);
  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = 1;

  if (pCodec->capabilities & CODEC_CAP_TRUNCATED)
    m_pCodecContext->flags |= CODEC_FLAG_TRUNCATED;

  m_channels = 0;
  m_pCodecContext->channels = hints.channels;
  m_pCodecContext->sample_rate = hints.samplerate;
  m_pCodecContext->block_align = hints.blockalign;
  m_pCodecContext->bit_rate = hints.bitrate;
  m_pCodecContext->bits_per_coded_sample = hints.bitspersample;
  if (hints.codec == AV_CODEC_ID_TRUEHD)
  {
    if (layout == PCM_LAYOUT_2_0)
    {
      m_pCodecContext->request_channel_layout = AV_CH_LAYOUT_STEREO;
      m_pCodecContext->channels = 2;
      m_pCodecContext->channel_layout = av_get_default_channel_layout(m_pCodecContext->channels);
    }
    else if (layout <= PCM_LAYOUT_5_1)
    {
      m_pCodecContext->request_channel_layout = AV_CH_LAYOUT_5POINT1;
      m_pCodecContext->channels = 6;
      m_pCodecContext->channel_layout = av_get_default_channel_layout(m_pCodecContext->channels);
    }
  }
  if (m_pCodecContext->request_channel_layout)
    LOG_TRACE_2 << "COMXAudioCodecOMX::Open() Requesting channel layout of "
        << (unsigned)m_pCodecContext->request_channel_layout;

  if(m_pCodecContext->bits_per_coded_sample == 0)
    m_pCodecContext->bits_per_coded_sample = 16;

  if( hints.extradata && hints.extrasize > 0 )
  {
    m_pCodecContext->extradata_size = hints.extrasize;
    m_pCodecContext->extradata = (uint8_t*)av_mallocz(hints.extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, hints.extradata, hints.extrasize);
  }

  if (avcodec_open2(m_pCodecContext, pCodec, NULL) < 0)
  {
    LOG_TRACE_2 << "COMXAudioCodecOMX::Open() Unable to open codec";
    Dispose();
    return false;
  }

  m_pFrame1 = av_frame_alloc();
  m_bOpenedCodec = true;
  m_iSampleFormat = AV_SAMPLE_FMT_NONE;
  m_desiredSampleFormat = m_pCodecContext->sample_fmt == AV_SAMPLE_FMT_S16 ? AV_SAMPLE_FMT_S16 : AV_SAMPLE_FMT_FLTP;
  return true;
}

void COMXAudioCodecOMX::Dispose()
{
  if (m_pFrame1) av_free(m_pFrame1);
    m_pFrame1 = NULL;

  if (m_pConvert)
    swr_free(&m_pConvert);

  if (m_pCodecContext)
  {
    if (m_pCodecContext->extradata) av_free(m_pCodecContext->extradata);
    m_pCodecContext->extradata = NULL;
    if (m_bOpenedCodec) avcodec_close(m_pCodecContext);
    m_bOpenedCodec = false;
    av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }
  m_bGotFrame = false;
}

int COMXAudioCodecOMX::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  int iBytesUsed, got_frame;
  if (!m_pCodecContext) return -1;

  AVPacket avpkt;
  if (m_bGotFrame)
    return 0;

  av_init_packet(&avpkt);
  avpkt.data = pData;
  avpkt.size = iSize;
  iBytesUsed = avcodec_decode_audio4( m_pCodecContext
                                                 , m_pFrame1
                                                 , &got_frame
                                                 , &avpkt);
  if (iBytesUsed < 0 || !got_frame)
  {
    return iBytesUsed;
  }
  /* some codecs will attempt to consume more data than what we gave */
  if (iBytesUsed > iSize)
  {
    LOG_WARNING << "COMXAudioCodecOMX::Decode - decoder attempted to consume more data than given";
    iBytesUsed = iSize;
  }
  m_bGotFrame = true;

  if (m_bFirstFrame)
  {
    char log_buf[512];
    sprintf(log_buf, "COMXAudioCodecOMX::Decode(%p,%d) format=%d(%d) chan=%d samples=%d size=%d data=%p,%p,%p,%p,%p,%p,%p,%p",
             pData, iSize, m_pCodecContext->sample_fmt, m_desiredSampleFormat,
             m_pCodecContext->channels, m_pFrame1->nb_samples, m_pFrame1->linesize[0],
             m_pFrame1->data[0], m_pFrame1->data[1], m_pFrame1->data[2], m_pFrame1->data[3],
             m_pFrame1->data[4], m_pFrame1->data[5], m_pFrame1->data[6], m_pFrame1->data[7]
         );
    LOG_TRACE_2 << log_buf;
  }

  if (!m_iBufferOutputUsed)
  {
    m_dts = dts;
    m_pts = pts;
  }
  return iBytesUsed;
}

int COMXAudioCodecOMX::GetData(BYTE** dst, double &dts, double &pts)
{
  if (!m_bGotFrame)
    return 0;
  int inLineSize, outLineSize;
  /* input audio is aligned */
  int inputSize = av_samples_get_buffer_size(&inLineSize, m_pCodecContext->channels, m_pFrame1->nb_samples, m_pCodecContext->sample_fmt, 0);
  /* output audio will be packed */
  int outputSize = av_samples_get_buffer_size(&outLineSize, m_pCodecContext->channels, m_pFrame1->nb_samples, m_desiredSampleFormat, 1);

  if (!m_bNoConcatenate && m_iBufferOutputUsed && (int)m_frameSize != outputSize)
  {
    LOG_TRACE_2 << "COMXAudioCodecOMX::GetData Unexpected change of size (" << m_frameSize <<" ->"
        << outputSize << ")";
    m_bNoConcatenate = true;
  }

  // if this buffer won't fit then flush out what we have
  int desired_size = AUDIO_DECODE_OUTPUT_BUFFER * (m_pCodecContext->channels * GetBitsPerSample()) >> (rounded_up_channels_shift[m_pCodecContext->channels] + 4);
  if (m_iBufferOutputUsed && (m_iBufferOutputUsed + outputSize > desired_size || m_bNoConcatenate))
  {
     int ret = m_iBufferOutputUsed;
     m_iBufferOutputUsed = 0;
     m_bNoConcatenate = false;
     dts = m_dts;
     pts = m_pts;
     *dst = m_pBufferOutput;
     return ret;
  }
  m_frameSize = outputSize;

  if (m_iBufferOutputAlloced < m_iBufferOutputUsed + outputSize)
  {
     m_pBufferOutput = (BYTE*)av_realloc(m_pBufferOutput, m_iBufferOutputUsed + outputSize + FF_INPUT_BUFFER_PADDING_SIZE);
     m_iBufferOutputAlloced = m_iBufferOutputUsed + outputSize;
  }

  /* need to convert format */
  if(m_pCodecContext->sample_fmt != m_desiredSampleFormat)
  {
    if(m_pConvert && (m_pCodecContext->sample_fmt != m_iSampleFormat || m_channels != m_pCodecContext->channels))
    {
      swr_free(&m_pConvert);
      m_channels = m_pCodecContext->channels;
    }

    if(!m_pConvert)
    {
      m_iSampleFormat = m_pCodecContext->sample_fmt;
      m_pConvert = swr_alloc_set_opts(NULL,
                      av_get_default_channel_layout(m_pCodecContext->channels),
                      m_desiredSampleFormat, m_pCodecContext->sample_rate,
                      av_get_default_channel_layout(m_pCodecContext->channels),
                      m_pCodecContext->sample_fmt, m_pCodecContext->sample_rate,
                      0, NULL);

      if(!m_pConvert || swr_init(m_pConvert) < 0)
      {
        LOG_TRACE_2 << "COMXAudioCodecOMX::Decode - Unable to initialise convert format "
            << m_pCodecContext->sample_fmt << " to " << m_desiredSampleFormat;
        return 0;
      }
    }

    /* use unaligned flag to keep output packed */
    uint8_t *out_planes[m_pCodecContext->channels];
    if(av_samples_fill_arrays(out_planes, NULL, m_pBufferOutput + m_iBufferOutputUsed, m_pCodecContext->channels, m_pFrame1->nb_samples, m_desiredSampleFormat, 1) < 0 ||
       swr_convert(m_pConvert, out_planes, m_pFrame1->nb_samples, (const uint8_t **)m_pFrame1->data, m_pFrame1->nb_samples) < 0)
    {
      LOG_TRACE_2 << "COMXAudioCodecOMX::Decode - Unable to convert format " <<
        (int)m_pCodecContext->sample_fmt << " to " << m_desiredSampleFormat;
      outputSize = 0;
    }
  }
  else
  {
    /* copy to a contiguous buffer */
    uint8_t *out_planes[m_pCodecContext->channels];
    if (av_samples_fill_arrays(out_planes, NULL, m_pBufferOutput + m_iBufferOutputUsed, m_pCodecContext->channels, m_pFrame1->nb_samples, m_desiredSampleFormat, 1) < 0 ||
      av_samples_copy(out_planes, m_pFrame1->data, 0, 0, m_pFrame1->nb_samples, m_pCodecContext->channels, m_desiredSampleFormat) < 0 )
    {
      outputSize = 0;
    }
  }
  m_bGotFrame = false;

  if (m_bFirstFrame)
  {
    char log_buf[512];
    sprintf(log_buf, "COMXAudioCodecOMX::GetData size=%d/%d line=%d/%d buf=%p, desired=%d",
            inputSize, outputSize, inLineSize, outLineSize, m_pBufferOutput, desired_size);
    LOG_TRACE_2 << log_buf;
    m_bFirstFrame = false;
  }
  m_iBufferOutputUsed += outputSize;
  return 0;
}

void COMXAudioCodecOMX::Reset()
{
  if (m_pCodecContext) avcodec_flush_buffers(m_pCodecContext);
  m_bGotFrame = false;
  m_iBufferOutputUsed = 0;
}

int COMXAudioCodecOMX::GetChannels()
{
  if (!m_pCodecContext)
    return 0;
  return m_pCodecContext->channels;
}

int COMXAudioCodecOMX::GetSampleRate()
{
  if (!m_pCodecContext)
    return 0;
  return m_pCodecContext->sample_rate;
}

int COMXAudioCodecOMX::GetBitsPerSample()
{
  if (!m_pCodecContext)
    return 0;
  return m_pCodecContext->sample_fmt == AV_SAMPLE_FMT_S16 ? 16 : 32;
}

int COMXAudioCodecOMX::GetBitRate()
{
  if (!m_pCodecContext)
    return 0;
  return m_pCodecContext->bit_rate;
}

static unsigned count_bits(int64_t value)
{
  unsigned bits = 0;
  for(;value;++bits)
    value &= value - 1;
  return bits;
}

uint64_t COMXAudioCodecOMX::GetChannelMap()
{
  uint64_t layout;
  int bits = count_bits(m_pCodecContext->channel_layout);
  if (bits == m_pCodecContext->channels)
    layout = m_pCodecContext->channel_layout;
  else
  {
    LOG_TRACE_2 << "COMXAudioCodecOMX::GetChannelMap - FFmpeg reported" << m_pCodecContext->channels <<
        " channels, but the layout contains " << bits << "ignoring";
    layout = av_get_default_channel_layout(m_pCodecContext->channels);
  }


  return layout;
}
