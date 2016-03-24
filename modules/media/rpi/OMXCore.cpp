/*
 *      Copyright (C) 2010-2013 Team XBMCn
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include <math.h>
#include <sys/time.h>
#include <cassert>

#include "OMXCore.h"
#include "OMXClock.h"
#include "linux/XMemUtils.h"
#include "core/Logger.hpp"

//#define OMX_DEBUG_EVENTS
//#define OMX_DEBUG_EVENTHANDLER

namespace
{
    char g_log_buf[512];
}
////////////////////////////////////////////////////////////////////////////////////////////
#define CLASSNAME "COMXCoreComponent"
////////////////////////////////////////////////////////////////////////////////////////////

static void add_timespecs(struct timespec &time, long millisecs)
{
   long long nsec = time.tv_nsec + (long long)millisecs * 1000000;
   while (nsec > 1000000000)
   {
      time.tv_sec += 1;
      nsec -= 1000000000;
   }
   time.tv_nsec = nsec;
}


COMXCoreTunel::COMXCoreTunel()
{
  m_src_component       = NULL;
  m_dst_component       = NULL;
  m_src_port            = 0;
  m_dst_port            = 0;
  m_tunnel_set          = false;
}

COMXCoreTunel::~COMXCoreTunel()
{
}

void COMXCoreTunel::Initialize(COMXCoreComponent *src_component, unsigned int src_port, COMXCoreComponent *dst_component, unsigned int dst_port)
{
  m_src_component  = src_component;
  m_src_port    = src_port;
  m_dst_component  = dst_component;
  m_dst_port    = dst_port;
}

OMX_ERRORTYPE COMXCoreTunel::Deestablish(bool noWait)
{
  if(!m_src_component || !m_dst_component || !IsInitialized())
    return OMX_ErrorUndefined;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  if(m_src_component->GetComponent())
  {
    omx_err = m_src_component->DisablePort(m_src_port, false);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Deestablish - Error disable port %d on component %s omx_err(0x%08x)",
          m_src_port, m_src_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
    }
  }

  if(m_dst_component->GetComponent())
  {
    omx_err = m_dst_component->DisablePort(m_dst_port, false);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Deestablish - Error disable port %d on component %s omx_err(0x%08x)",
          m_dst_port, m_dst_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
    }
  }

  if(m_src_component->GetComponent())
  {
    omx_err = m_src_component->WaitForCommand(OMX_CommandPortDisable, m_src_port);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Deestablish - Error WaitForCommand port %d on component %s omx_err(0x%08x)",
          m_dst_port, m_src_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
      return omx_err;
    }
  }

  if(m_dst_component->GetComponent())
  {
    omx_err = m_dst_component->WaitForCommand(OMX_CommandPortDisable, m_dst_port);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Deestablish - Error WaitForCommand port %d on component %s omx_err(0x%08x)",
          m_dst_port, m_dst_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
      return omx_err;
    }
  }

  if(m_src_component->GetComponent())
  {
    omx_err = OMX_SetupTunnel(m_src_component->GetComponent(), m_src_port, NULL, 0);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Deestablish - could not unset tunnel on comp src %s port %d omx_err(0x%08x)\n",
          m_src_component->GetName().c_str(), m_src_port, (int)omx_err);
      LOG_ERROR << g_log_buf;
    }
  }

  if(m_dst_component->GetComponent())
  {
    omx_err = OMX_SetupTunnel(m_dst_component->GetComponent(), m_dst_port, NULL, 0);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Deestablish - could not unset tunnel on comp dst %s port %d omx_err(0x%08x)\n",
          m_dst_component->GetName().c_str(), m_dst_port, (int)omx_err);
      LOG_ERROR << g_log_buf;
    }
  }

  m_tunnel_set = false;

  return OMX_ErrorNone;
}

OMX_ERRORTYPE COMXCoreTunel::Establish(bool enable_ports /* = true */, bool disable_ports /* = false */)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_PARAM_U32TYPE param;
  OMX_INIT_STRUCTURE(param);

  if(!m_src_component || !m_dst_component)
  {
    return OMX_ErrorUndefined;
  }

  if(m_src_component->GetState() == OMX_StateLoaded)
  {
    omx_err = m_src_component->SetStateForComponent(OMX_StateIdle);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Establish - Error setting state to idle %s omx_err(0x%08x)",
              m_src_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
      return omx_err;
    }
  }

  if(m_src_component->GetComponent() && disable_ports)
  {
    omx_err = m_src_component->DisablePort(m_src_port, false);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Establish - Error disable port %d on component %s omx_err(0x%08x)",
          m_src_port, m_src_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
    }
  }

  if(m_dst_component->GetComponent() && disable_ports)
  {
    omx_err = m_dst_component->DisablePort(m_dst_port, false);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Establish - Error disable port %d on component %s omx_err(0x%08x)",
          m_dst_port, m_dst_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
    }
  }

  if(m_src_component->GetComponent() && disable_ports)
  {
    omx_err = m_src_component->WaitForCommand(OMX_CommandPortDisable, m_src_port);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Establish - Error WaitForCommand port %d on component %s omx_err(0x%08x)",
          m_dst_port, m_src_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
      return omx_err;
    }
  }

  if(m_dst_component->GetComponent() && disable_ports)
  {
    omx_err = m_dst_component->WaitForCommand(OMX_CommandPortDisable, m_dst_port);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Establish - Error WaitForCommand port %d on component %s omx_err(0x%08x)",
          m_dst_port, m_dst_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
      return omx_err;
    }
  }

  if(m_src_component->GetComponent() && m_dst_component->GetComponent())
  {
    omx_err = OMX_SetupTunnel(m_src_component->GetComponent(), m_src_port, m_dst_component->GetComponent(), m_dst_port);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Establish - could not setup tunnel src %s port %d dst %s port %d omx_err(0x%08x)\n",
          m_src_component->GetName().c_str(), m_src_port, m_dst_component->GetName().c_str(), m_dst_port, (int)omx_err);
      LOG_ERROR << g_log_buf;
      return omx_err;
    }
  }
  else
  {
    LOG_ERROR << "COMXCoreTunel::Establish - could not setup tunnel";
    return OMX_ErrorUndefined;
  }

  m_tunnel_set = true;

  if(m_src_component->GetComponent() && enable_ports)
  {
    omx_err = m_src_component->EnablePort(m_src_port, false);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Establish - Error enable port %d on component %s omx_err(0x%08x)",
          m_src_port, m_src_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
      return omx_err;
    }
  }

  if(m_dst_component->GetComponent() && enable_ports)
  {
    omx_err = m_dst_component->EnablePort(m_dst_port, false);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreTunel::Establish - Error enable port %d on component %s omx_err(0x%08x)",
          m_dst_port, m_dst_component->GetName().c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
      return omx_err;
    }
  }

  if(m_dst_component->GetComponent() && enable_ports)
  {
    omx_err = m_dst_component->WaitForCommand(OMX_CommandPortEnable, m_dst_port);
    if(omx_err != OMX_ErrorNone)
    {
      return omx_err;
    }

    if(m_dst_component->GetState() == OMX_StateLoaded)
    {
      omx_err = m_dst_component->SetStateForComponent(OMX_StateIdle);
      if(omx_err != OMX_ErrorNone)
      {
        sprintf(g_log_buf, "COMXCoreComponent::Establish - Error setting state to idle %s omx_err(0x%08x)",
            m_src_component->GetName().c_str(), (int)omx_err);
        LOG_ERROR << g_log_buf;
        return omx_err;
      }
    }
  }

  if(m_src_component->GetComponent() && enable_ports)
  {
    omx_err = m_src_component->WaitForCommand(OMX_CommandPortEnable, m_src_port);
    if(omx_err != OMX_ErrorNone)
    {
      return omx_err;
    }
  }

  return OMX_ErrorNone;
}

////////////////////////////////////////////////////////////////////////////////////////////

COMXCoreComponent::COMXCoreComponent()
{
  m_input_port  = 0;
  m_output_port = 0;
  m_handle      = NULL;

  m_input_alignment     = 0;
  m_input_buffer_size  = 0;
  m_input_buffer_count  = 0;

  m_output_alignment    = 0;
  m_output_buffer_size  = 0;
  m_output_buffer_count = 0;
  m_flush_input         = false;
  m_flush_output        = false;
  m_resource_error      = false;

  m_eos                 = false;

  m_exit = false;

  m_omx_input_use_buffers  = false;
  m_omx_output_use_buffers = false;

  m_omx_events.clear();
  m_ignore_error = OMX_ErrorNone;

  pthread_mutex_init(&m_omx_input_mutex, NULL);
  pthread_mutex_init(&m_omx_output_mutex, NULL);
  pthread_mutex_init(&m_omx_event_mutex, NULL);
  pthread_mutex_init(&m_omx_eos_mutex, NULL);
  pthread_cond_init(&m_input_buffer_cond, NULL);
  pthread_cond_init(&m_output_buffer_cond, NULL);
  pthread_cond_init(&m_omx_event_cond, NULL);
}

COMXCoreComponent::~COMXCoreComponent()
{
  Deinitialize();

  pthread_mutex_destroy(&m_omx_input_mutex);
  pthread_mutex_destroy(&m_omx_output_mutex);
  pthread_mutex_destroy(&m_omx_event_mutex);
  pthread_mutex_destroy(&m_omx_eos_mutex);
  pthread_cond_destroy(&m_input_buffer_cond);
  pthread_cond_destroy(&m_output_buffer_cond);
  pthread_cond_destroy(&m_omx_event_cond);
}

void COMXCoreComponent::TransitionToStateLoaded()
{
  if(!m_handle)
    return;

  if(GetState() != OMX_StateLoaded && GetState() != OMX_StateIdle)
    SetStateForComponent(OMX_StateIdle);

  if(GetState() != OMX_StateLoaded)
    SetStateForComponent(OMX_StateLoaded);
}

OMX_ERRORTYPE COMXCoreComponent::EmptyThisBuffer(OMX_BUFFERHEADERTYPE *omx_buffer)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  #if defined(OMX_DEBUG_EVENTHANDLER)
  sprintf(g_log_buf, "COMXCoreComponent::EmptyThisBuffer component(%s) %p", m_componentName.c_str(), omx_buffer);
  LOG_TRACE_2 << g_log_buf;
  #endif
  if(!m_handle || !omx_buffer)
    return OMX_ErrorUndefined;

  omx_err = OMX_EmptyThisBuffer(m_handle, omx_buffer);
  if (omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::EmptyThisBuffer component(%s) - failed with result(0x%x)\n",
        m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }

  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::FillThisBuffer(OMX_BUFFERHEADERTYPE *omx_buffer)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  #if defined(OMX_DEBUG_EVENTHANDLER)
  sprintf(g_log_buf, "COMXCoreComponent::FillThisBuffer component(%s) %p", m_componentName.c_str(),
         omx_buffer);
  LOG_TRACE_2 << g_log_buf;
  #endif
  if(!m_handle || !omx_buffer)
    return OMX_ErrorUndefined;

  omx_err = OMX_FillThisBuffer(m_handle, omx_buffer);
  if (omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::FillThisBuffer component(%s) - failed with result(0x%x)\n",
        m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }

  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::FreeOutputBuffer(OMX_BUFFERHEADERTYPE *omx_buffer)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  if(!m_handle || !omx_buffer)
    return OMX_ErrorUndefined;

  omx_err = OMX_FreeBuffer(m_handle, m_output_port, omx_buffer);
  if (omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::FreeOutputBuffer component(%s) - failed with result(0x%x)\n",
        m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }

  return omx_err;
}

void COMXCoreComponent::FlushAll()
{
  FlushInput();
  FlushOutput();
}

void COMXCoreComponent::FlushInput()
{
  if(!m_handle || m_resource_error)
    return;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  omx_err = OMX_SendCommand(m_handle, OMX_CommandFlush, m_input_port, NULL);

  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::FlushInput - Error on component %s omx_err(0x%08x)",
              m_componentName.c_str(), (int)omx_err);
    LOG_ERROR << g_log_buf;
  }
  omx_err = WaitForCommand(OMX_CommandFlush, m_input_port);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::FlushInput - %s WaitForCommand omx_err(0x%08x)",
              m_componentName.c_str(), (int)omx_err);
    LOG_ERROR << g_log_buf;
  }
}

void COMXCoreComponent::FlushOutput()
{
  if(!m_handle || m_resource_error)
    return;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  omx_err = OMX_SendCommand(m_handle, OMX_CommandFlush, m_output_port, NULL);

  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::FlushOutput - Error on component %s omx_err(0x%08x)",
              m_componentName.c_str(), (int)omx_err);
    LOG_ERROR << g_log_buf;
  }
  omx_err = WaitForCommand(OMX_CommandFlush, m_output_port);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::FlushOutput - %s WaitForCommand omx_err(0x%08x)",
              m_componentName.c_str(), (int)omx_err);
    LOG_ERROR << g_log_buf;
  }
}

// timeout in milliseconds
OMX_BUFFERHEADERTYPE *COMXCoreComponent::GetInputBuffer(long timeout /*=200*/)
{
  OMX_BUFFERHEADERTYPE *omx_input_buffer = NULL;

  if(!m_handle)
    return NULL;

  pthread_mutex_lock(&m_omx_input_mutex);
  struct timespec endtime;
  clock_gettime(CLOCK_REALTIME, &endtime);
  add_timespecs(endtime, timeout);
  while (!m_flush_input)
  {
    if (m_resource_error)
      break;
    if(!m_omx_input_avaliable.empty())
    {
      omx_input_buffer = m_omx_input_avaliable.front();
      m_omx_input_avaliable.pop();
      break;
    }

    int retcode = pthread_cond_timedwait(&m_input_buffer_cond, &m_omx_input_mutex, &endtime);
    if (retcode != 0) {
      if (timeout != 0)
        LOG_ERROR << "COMXCoreComponent::GetInputBuffer" << m_componentName << "wait event timeout";
      break;
    }
  }
  pthread_mutex_unlock(&m_omx_input_mutex);
  return omx_input_buffer;
}

OMX_BUFFERHEADERTYPE *COMXCoreComponent::GetOutputBuffer(long timeout /*=200*/)
{
  OMX_BUFFERHEADERTYPE *omx_output_buffer = NULL;
  if(!m_handle)
    return NULL;

  pthread_mutex_lock(&m_omx_output_mutex);
  struct timespec endtime;
  clock_gettime(CLOCK_REALTIME, &endtime);
  add_timespecs(endtime, timeout);
  while (!m_flush_output)
  {
    if (m_resource_error)
      break;
    if(!m_omx_output_available.empty())
    {
      omx_output_buffer = m_omx_output_available.front();
      m_omx_output_available.pop();
      break;
    }

    int retcode = pthread_cond_timedwait(&m_output_buffer_cond, &m_omx_output_mutex, &endtime);
    if (retcode != 0) {
      if (timeout != 0)
        LOG_ERROR << "COMXCoreComponent::GetOutputBuffer " << m_componentName << " wait event timeout";
      break;
    }
  }
  pthread_mutex_unlock(&m_omx_output_mutex);

  return omx_output_buffer;
}


OMX_ERRORTYPE COMXCoreComponent::WaitForInputDone(long timeout /*=200*/)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  pthread_mutex_lock(&m_omx_input_mutex);
  struct timespec endtime;
  clock_gettime(CLOCK_REALTIME, &endtime);
  add_timespecs(endtime, timeout);
  while (m_input_buffer_count != m_omx_input_avaliable.size())
  {
    if (m_resource_error)
      break;
    int retcode = pthread_cond_timedwait(&m_input_buffer_cond, &m_omx_input_mutex, &endtime);
    if (retcode != 0) {
      if (timeout != 0)
        LOG_ERROR << "COMXCoreComponent::WaitForInputDone  " << m_componentName << " wait event timeout";
      omx_err = OMX_ErrorTimeout;
      break;
    }
  }
  pthread_mutex_unlock(&m_omx_input_mutex);
  return omx_err;
}


OMX_ERRORTYPE COMXCoreComponent::WaitForOutputDone(long timeout /*=200*/)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  pthread_mutex_lock(&m_omx_output_mutex);
  struct timespec endtime;
  clock_gettime(CLOCK_REALTIME, &endtime);
  add_timespecs(endtime, timeout);
  while (m_output_buffer_count != m_omx_output_available.size())
  {
    if (m_resource_error)
      break;
    int retcode = pthread_cond_timedwait(&m_output_buffer_cond, &m_omx_output_mutex, &endtime);
    if (retcode != 0) {
      if (timeout != 0)
        LOG_ERROR << "COMXCoreComponent::WaitForOutputDone  " << m_componentName << " wait event timeout";
      omx_err = OMX_ErrorTimeout;
      break;
    }
  }
  pthread_mutex_unlock(&m_omx_output_mutex);
  return omx_err;
}


OMX_ERRORTYPE COMXCoreComponent::AllocInputBuffers(bool use_buffers /* = false **/)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  m_omx_input_use_buffers = use_buffers;

  if(!m_handle)
    return OMX_ErrorUndefined;

  OMX_PARAM_PORTDEFINITIONTYPE portFormat;
  OMX_INIT_STRUCTURE(portFormat);
  portFormat.nPortIndex = m_input_port;

  omx_err = OMX_GetParameter(m_handle, OMX_IndexParamPortDefinition, &portFormat);
  if(omx_err != OMX_ErrorNone)
    return omx_err;

  if(GetState() != OMX_StateIdle)
  {
    if(GetState() != OMX_StateLoaded)
      SetStateForComponent(OMX_StateLoaded);

    SetStateForComponent(OMX_StateIdle);
  }

  omx_err = EnablePort(m_input_port, false);
  if(omx_err != OMX_ErrorNone)
    return omx_err;

  m_input_alignment     = portFormat.nBufferAlignment;
  m_input_buffer_count  = portFormat.nBufferCountActual;
  m_input_buffer_size   = portFormat.nBufferSize;

  sprintf(g_log_buf, "COMXCoreComponent::AllocInputBuffers component(%s) - port(%d), nBufferCountMin(%u), nBufferCountActual(%u), nBufferSize(%u), nBufferAlignmen(%u)",
            m_componentName.c_str(), GetInputPort(), portFormat.nBufferCountMin,
            portFormat.nBufferCountActual, portFormat.nBufferSize, portFormat.nBufferAlignment);
  LOG_TRACE_2 << g_log_buf;

  for (size_t i = 0; i < portFormat.nBufferCountActual; i++)
  {
    OMX_BUFFERHEADERTYPE *buffer = NULL;
    OMX_U8* data = NULL;

    if(m_omx_input_use_buffers)
    {
      data = (OMX_U8*)_aligned_malloc(portFormat.nBufferSize, m_input_alignment);
      omx_err = OMX_UseBuffer(m_handle, &buffer, m_input_port, NULL, portFormat.nBufferSize, data);
    }
    else
    {
      omx_err = OMX_AllocateBuffer(m_handle, &buffer, m_input_port, NULL, portFormat.nBufferSize);
    }
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreComponent::AllocInputBuffers component(%s) - OMX_UseBuffer failed with omx_err(0x%x)",
              m_componentName.c_str(), omx_err);
      LOG_ERROR << g_log_buf;

      if(m_omx_input_use_buffers && data)
        _aligned_free(data);

      return omx_err;
    }
    buffer->nInputPortIndex = m_input_port;
    buffer->nFilledLen      = 0;
    buffer->nOffset         = 0;
    buffer->pAppPrivate     = (void*)i;
    m_omx_input_buffers.push_back(buffer);
    m_omx_input_avaliable.push(buffer);
  }

  omx_err = WaitForCommand(OMX_CommandPortEnable, m_input_port);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::AllocInputBuffers WaitForCommand:OMX_CommandPortEnable failed on %s omx_err(0x%08x)",
            m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
    return omx_err;
  }

  m_flush_input = false;

  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::AllocOutputBuffers(bool use_buffers /* = false */)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  if(!m_handle)
    return OMX_ErrorUndefined;

  m_omx_output_use_buffers = use_buffers;

  OMX_PARAM_PORTDEFINITIONTYPE portFormat;
  OMX_INIT_STRUCTURE(portFormat);
  portFormat.nPortIndex = m_output_port;

  omx_err = OMX_GetParameter(m_handle, OMX_IndexParamPortDefinition, &portFormat);
  if(omx_err != OMX_ErrorNone)
    return omx_err;

  if(GetState() != OMX_StateIdle)
  {
    if(GetState() != OMX_StateLoaded)
      SetStateForComponent(OMX_StateLoaded);

    SetStateForComponent(OMX_StateIdle);
  }

  omx_err = EnablePort(m_output_port, false);
  if(omx_err != OMX_ErrorNone)
    return omx_err;

  m_output_alignment     = portFormat.nBufferAlignment;
  m_output_buffer_count  = portFormat.nBufferCountActual;
  m_output_buffer_size   = portFormat.nBufferSize;

  sprintf(g_log_buf, "COMXCoreComponent::AllocOutputBuffers component(%s) - port(%d), nBufferCountMin(%u), nBufferCountActual(%u), nBufferSize(%u) nBufferAlignmen(%u)",
            m_componentName.c_str(), m_output_port, portFormat.nBufferCountMin,
            portFormat.nBufferCountActual, portFormat.nBufferSize, portFormat.nBufferAlignment);
  LOG_TRACE_2 << g_log_buf;

  for (size_t i = 0; i < portFormat.nBufferCountActual; i++)
  {
    OMX_BUFFERHEADERTYPE *buffer = NULL;
    OMX_U8* data = NULL;

    if(m_omx_output_use_buffers)
    {
      data = (OMX_U8*)_aligned_malloc(portFormat.nBufferSize, m_output_alignment);
      omx_err = OMX_UseBuffer(m_handle, &buffer, m_output_port, NULL, portFormat.nBufferSize, data);
    }
    else
    {
      omx_err = OMX_AllocateBuffer(m_handle, &buffer, m_output_port, NULL, portFormat.nBufferSize);
    }
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreComponent::AllocOutputBuffers component(%s) - OMX_UseBuffer failed with omx_err(0x%x)\n",
        m_componentName.c_str(), omx_err);
      LOG_ERROR << g_log_buf;

      if(m_omx_output_use_buffers && data)
       _aligned_free(data);

      return omx_err;
    }
    buffer->nOutputPortIndex = m_output_port;
    buffer->nFilledLen       = 0;
    buffer->nOffset          = 0;
    buffer->pAppPrivate      = (void*)i;
    m_omx_output_buffers.push_back(buffer);
    m_omx_output_available.push(buffer);
  }

  omx_err = WaitForCommand(OMX_CommandPortEnable, m_output_port);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::AllocOutputBuffers WaitForCommand:OMX_CommandPortEnable failed on %s omx_err(0x%08x)\n", m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
    return omx_err;
  }

  m_flush_output = false;

  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::FreeInputBuffers()
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  if(!m_handle)
    return OMX_ErrorUndefined;

  if(m_omx_input_buffers.empty())
    return OMX_ErrorNone;

  m_flush_input = true;

  omx_err = DisablePort(m_input_port, false);

  pthread_mutex_lock(&m_omx_input_mutex);
  pthread_cond_broadcast(&m_input_buffer_cond);

  for (size_t i = 0; i < m_omx_input_buffers.size(); i++)
  {
    uint8_t *buf = m_omx_input_buffers[i]->pBuffer;

    omx_err = OMX_FreeBuffer(m_handle, m_input_port, m_omx_input_buffers[i]);

    if(m_omx_input_use_buffers && buf)
      _aligned_free(buf);

    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreComponent::FreeInputBuffers error deallocate omx input buffer on component %s omx_err(0x%08x)\n", m_componentName.c_str(), omx_err);
      LOG_ERROR << g_log_buf;
    }
  }
  pthread_mutex_unlock(&m_omx_input_mutex);

  omx_err = WaitForCommand(OMX_CommandPortDisable, m_input_port);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::FreeInputBuffers WaitForCommand:OMX_CommandPortDisable failed on %s omx_err(0x%08x)\n", m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }

  WaitForInputDone(1000);

  pthread_mutex_lock(&m_omx_input_mutex);
  assert(m_omx_input_buffers.size() == m_omx_input_avaliable.size());

  m_omx_input_buffers.clear();

  while (!m_omx_input_avaliable.empty())
    m_omx_input_avaliable.pop();

  m_input_alignment     = 0;
  m_input_buffer_size   = 0;
  m_input_buffer_count  = 0;

  pthread_mutex_unlock(&m_omx_input_mutex);

  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::FreeOutputBuffers()
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  if(!m_handle)
    return OMX_ErrorUndefined;

  if(m_omx_output_buffers.empty())
    return OMX_ErrorNone;

  m_flush_output = true;

  omx_err = DisablePort(m_output_port, false);

  pthread_mutex_lock(&m_omx_output_mutex);
  pthread_cond_broadcast(&m_output_buffer_cond);

  for (size_t i = 0; i < m_omx_output_buffers.size(); i++)
  {
    uint8_t *buf = m_omx_output_buffers[i]->pBuffer;

    omx_err = OMX_FreeBuffer(m_handle, m_output_port, m_omx_output_buffers[i]);

    if(m_omx_output_use_buffers && buf)
      _aligned_free(buf);

    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreComponent::FreeOutputBuffers error deallocate omx output buffer on component %s omx_err(0x%08x)\n", m_componentName.c_str(), omx_err);
      LOG_ERROR << g_log_buf;
    }
  }
  pthread_mutex_unlock(&m_omx_output_mutex);

  omx_err = WaitForCommand(OMX_CommandPortDisable, m_output_port);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::FreeOutputBuffers WaitForCommand:OMX_CommandPortDisable failed on %s omx_err(0x%08x)\n", m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }

  WaitForOutputDone(1000);

  pthread_mutex_lock(&m_omx_output_mutex);
  assert(m_omx_output_buffers.size() == m_omx_output_available.size());

  m_omx_output_buffers.clear();

  while (!m_omx_output_available.empty())
    m_omx_output_available.pop();

  m_output_alignment    = 0;
  m_output_buffer_size  = 0;
  m_output_buffer_count = 0;

  pthread_mutex_unlock(&m_omx_output_mutex);

  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::DisableAllPorts()
{
  if(!m_handle)
    return OMX_ErrorUndefined;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  OMX_INDEXTYPE idxTypes[] = {
    OMX_IndexParamAudioInit,
    OMX_IndexParamImageInit,
    OMX_IndexParamVideoInit,
    OMX_IndexParamOtherInit
  };

  OMX_PORT_PARAM_TYPE ports;
  OMX_INIT_STRUCTURE(ports);

  int i;
  for(i=0; i < 4; i++)
  {
    omx_err = OMX_GetParameter(m_handle, idxTypes[i], &ports);
    if(omx_err == OMX_ErrorNone) {

      uint32_t j;
      for(j=0; j<ports.nPorts; j++)
      {
        OMX_PARAM_PORTDEFINITIONTYPE portFormat;
        OMX_INIT_STRUCTURE(portFormat);
        portFormat.nPortIndex = ports.nStartPortNumber+j;

        omx_err = OMX_GetParameter(m_handle, OMX_IndexParamPortDefinition, &portFormat);
        if(omx_err != OMX_ErrorNone)
        {
          if(portFormat.bEnabled == OMX_FALSE)
            continue;
        }

        omx_err = OMX_SendCommand(m_handle, OMX_CommandPortDisable, ports.nStartPortNumber+j, NULL);
        if(omx_err != OMX_ErrorNone)
        {
          sprintf(g_log_buf, "COMXCoreComponent::DisableAllPorts - Error disable port %d on component %s omx_err(0x%08x)",
            (int)(ports.nStartPortNumber) + j, m_componentName.c_str(), (int)omx_err);
          LOG_ERROR << g_log_buf;
        }
        omx_err = WaitForCommand(OMX_CommandPortDisable, ports.nStartPortNumber+j);
        if(omx_err != OMX_ErrorNone && omx_err != OMX_ErrorSameState)
          return omx_err;
      }
    }
  }

  return OMX_ErrorNone;
}

void COMXCoreComponent::RemoveEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2)
{
  for (std::vector<omx_event>::iterator it = m_omx_events.begin(); it != m_omx_events.end(); )
  {
    omx_event event = *it;

    if(event.eEvent == eEvent && event.nData1 == nData1 && event.nData2 == nData2)
    {
      it = m_omx_events.erase(it);
      continue;
    }
    ++it;
  }
}

OMX_ERRORTYPE COMXCoreComponent::AddEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2)
{
  omx_event event;

  event.eEvent      = eEvent;
  event.nData1      = nData1;
  event.nData2      = nData2;

  pthread_mutex_lock(&m_omx_event_mutex);
  RemoveEvent(eEvent, nData1, nData2);
  m_omx_events.push_back(event);
  // this allows (all) blocked tasks to be awoken
  pthread_cond_broadcast(&m_omx_event_cond);
  pthread_mutex_unlock(&m_omx_event_mutex);

#ifdef OMX_DEBUG_EVENTS
  sprintf(g_log_buf, "COMXCoreComponent::AddEvent %s add event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d",
          m_componentName.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
  LOG_TRACE_2 << g_log_buf;
#endif

  return OMX_ErrorNone;
}

// timeout in milliseconds
OMX_ERRORTYPE COMXCoreComponent::WaitForEvent(OMX_EVENTTYPE eventType, long timeout)
{
#ifdef OMX_DEBUG_EVENTS
  sprintf(g_log_buf, "COMXCoreComponent::WaitForEvent %s wait event 0x%08x",
      m_componentName.c_str(), (int)eventType);
  LOG_TRACE_2 << g_log_buf;
#endif

  pthread_mutex_lock(&m_omx_event_mutex);
  struct timespec endtime;
  clock_gettime(CLOCK_REALTIME, &endtime);
  add_timespecs(endtime, timeout);
  while(true)
  {
    for (std::vector<omx_event>::iterator it = m_omx_events.begin(); it != m_omx_events.end(); it++)
    {
      omx_event event = *it;

#ifdef OMX_DEBUG_EVENTS
      sprintf(g_log_buf, "COMXCoreComponent::WaitForEvent %s inlist event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d",
          m_componentName.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
      LOG_TRACE_2 << g_log_buf;
#endif


      if(event.eEvent == OMX_EventError && event.nData1 == (OMX_U32)OMX_ErrorSameState && event.nData2 == 1)
      {
#ifdef OMX_DEBUG_EVENTS
        sprintf(g_log_buf, "COMXCoreComponent::WaitForEvent %s remove event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d",
          m_componentName.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
        LOG_TRACE_2 << g_log_buf;
#endif
        m_omx_events.erase(it);
        pthread_mutex_unlock(&m_omx_event_mutex);
        return OMX_ErrorNone;
      }
      else if(event.eEvent == OMX_EventError)
      {
        m_omx_events.erase(it);
        pthread_mutex_unlock(&m_omx_event_mutex);
        return (OMX_ERRORTYPE)event.nData1;
      }
      else if(event.eEvent == eventType)
      {
#ifdef OMX_DEBUG_EVENTS
        sprintf(g_log_buf, "COMXCoreComponent::WaitForEvent %s remove event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d\n",
          m_componentName.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
        LOG_TRACE_2 << g_log_buf;
#endif

        m_omx_events.erase(it);
        pthread_mutex_unlock(&m_omx_event_mutex);
        return OMX_ErrorNone;
      }
    }

    if (m_resource_error)
      break;
    int retcode = pthread_cond_timedwait(&m_omx_event_cond, &m_omx_event_mutex, &endtime);
    if (retcode != 0)
    {
      if (timeout > 0)
      {
          sprintf(g_log_buf, "COMXCoreComponent::WaitForEvent %s wait event 0x%08x timeout %ld\n",
          m_componentName.c_str(), (int)eventType, timeout);
          LOG_ERROR << g_log_buf;
      }
      pthread_mutex_unlock(&m_omx_event_mutex);
      return OMX_ErrorTimeout;
    }
  }
  pthread_mutex_unlock(&m_omx_event_mutex);
  return OMX_ErrorNone;
}

// timeout in milliseconds
OMX_ERRORTYPE COMXCoreComponent::WaitForCommand(OMX_U32 command, OMX_U32 nData2, long timeout)
{
#ifdef OMX_DEBUG_EVENTS
  LOG_TRACE_2 << "COMXCoreComponent::WaitForCommand %s wait event.eEvent 0x%08x event.command 0x%08x event.nData2 %d\n",
      m_componentName.c_str(), (int)OMX_EventCmdComplete, (int)command, (int)nData2);
#endif

  pthread_mutex_lock(&m_omx_event_mutex);
  struct timespec endtime;
  clock_gettime(CLOCK_REALTIME, &endtime);
  add_timespecs(endtime, timeout);
  while(true)
  {
    for (std::vector<omx_event>::iterator it = m_omx_events.begin(); it != m_omx_events.end(); it++)
    {
      omx_event event = *it;

#ifdef OMX_DEBUG_EVENTS
      sprintf(g_log_buf, "COMXCoreComponent::WaitForCommand %s inlist event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d",
              m_componentName.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
      LOG_TRACE_2 << g_log_buf;
#endif
      if(event.eEvent == OMX_EventError && event.nData1 == (OMX_U32)OMX_ErrorSameState && event.nData2 == 1)
      {
#ifdef OMX_DEBUG_EVENTS
        sprintf(g_log_buf, "COMXCoreComponent::WaitForCommand %s remove event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d",
          m_componentName.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
        LOG_TRACE_2 << g_log_buf;
#endif

        m_omx_events.erase(it);
        pthread_mutex_unlock(&m_omx_event_mutex);
        return OMX_ErrorNone;
      }
      else if(event.eEvent == OMX_EventError)
      {
        m_omx_events.erase(it);
        pthread_mutex_unlock(&m_omx_event_mutex);
        return (OMX_ERRORTYPE)event.nData1;
      }
      else if(event.eEvent == OMX_EventCmdComplete && event.nData1 == command && event.nData2 == nData2)
      {

#ifdef OMX_DEBUG_EVENTS
        sprintf(g_log_buf, "COMXCoreComponent::WaitForCommand %s remove event event.eEvent 0x%08x event.nData1 0x%08x event.nData2 %d",
          m_componentName.c_str(), (int)event.eEvent, (int)event.nData1, (int)event.nData2);
        LOG_TRACE_2 << g_log_buf;
#endif

        m_omx_events.erase(it);
        pthread_mutex_unlock(&m_omx_event_mutex);
        return OMX_ErrorNone;
      }
    }

    if (m_resource_error)
      break;
    int retcode = pthread_cond_timedwait(&m_omx_event_cond, &m_omx_event_mutex, &endtime);
    if (retcode != 0)
    {
      sprintf(g_log_buf, "COMXCoreComponent::WaitForCommand %s wait timeout event.eEvent 0x%08x event.command 0x%08x event.nData2 %d",
        m_componentName.c_str(), (int)OMX_EventCmdComplete, (int)command, (int)nData2);
      LOG_ERROR << g_log_buf;

      pthread_mutex_unlock(&m_omx_event_mutex);
      return OMX_ErrorTimeout;
    }
  }
  pthread_mutex_unlock(&m_omx_event_mutex);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE COMXCoreComponent::SetStateForComponent(OMX_STATETYPE state)
{
  if(!m_handle)
    return OMX_ErrorUndefined;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_STATETYPE state_actual = OMX_StateMax;

  if(state == state_actual)
    return OMX_ErrorNone;

  omx_err = OMX_SendCommand(m_handle, OMX_CommandStateSet, state, 0);
  if (omx_err != OMX_ErrorNone)
  {
    if(omx_err == OMX_ErrorSameState)
    {
      LOG_ERROR << "COMXCoreComponent::SetStateForComponent - " << m_componentName <<" same state";
      omx_err = OMX_ErrorNone;
    }
    else
    {
      sprintf(g_log_buf, "COMXCoreComponent::SetStateForComponent - %s failed with omx_err(0x%x)",
        m_componentName.c_str(), omx_err);
      LOG_ERROR << g_log_buf;
    }
  }
  else
  {
    omx_err = WaitForCommand(OMX_CommandStateSet, state);
    if (omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreComponent::WaitForCommand - %s failed with omx_err(0x%x)",
        m_componentName.c_str(), omx_err);
      LOG_ERROR << g_log_buf;
    }
  }
  return omx_err;
}

OMX_STATETYPE COMXCoreComponent::GetState() const
{
  if(!m_handle)
    return (OMX_STATETYPE)0;

  OMX_STATETYPE state;

  OMX_ERRORTYPE omx_err = OMX_GetState(m_handle, &state);
  if (omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::GetState - %s failed with omx_err(0x%x)\n",
      m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }
  return state;
}

OMX_ERRORTYPE COMXCoreComponent::SetParameter(OMX_INDEXTYPE paramIndex, OMX_PTR paramStruct)
{
  if(!m_handle)
    return OMX_ErrorUndefined;

  OMX_ERRORTYPE omx_err;

  omx_err = OMX_SetParameter(m_handle, paramIndex, paramStruct);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::SetParameter - %s failed with omx_err(0x%x)",
              m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }
  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::GetParameter(OMX_INDEXTYPE paramIndex, OMX_PTR paramStruct) const
{
  if(!m_handle)
    return OMX_ErrorUndefined;

  OMX_ERRORTYPE omx_err;

  omx_err = OMX_GetParameter(m_handle, paramIndex, paramStruct);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::GetParameter - %s failed with omx_err(0x%x)\n",
              m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }
  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::SetConfig(OMX_INDEXTYPE configIndex, OMX_PTR configStruct)
{
  if(!m_handle)
    return OMX_ErrorUndefined;

  OMX_ERRORTYPE omx_err;

  omx_err = OMX_SetConfig(m_handle, configIndex, configStruct);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::SetConfig - %s failed with omx_err(0x%x)\n",
              m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }
  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::GetConfig(OMX_INDEXTYPE configIndex, OMX_PTR configStruct) const
{
  if(!m_handle)
    return OMX_ErrorUndefined;

  OMX_ERRORTYPE omx_err;

  omx_err = OMX_GetConfig(m_handle, configIndex, configStruct);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::GetConfig - %s failed with omx_err(0x%x)",
            m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }
  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::SendCommand(OMX_COMMANDTYPE cmd, OMX_U32 cmdParam, OMX_PTR cmdParamData)
{
  if(!m_handle)
    return OMX_ErrorUndefined;

  OMX_ERRORTYPE omx_err;

  omx_err = OMX_SendCommand(m_handle, cmd, cmdParam, cmdParamData);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::SendCommand - %s failed with omx_err(0x%x)",
              m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
  }
  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::EnablePort(unsigned int port,  bool wait)
{
  if(!m_handle)
    return OMX_ErrorUndefined;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  OMX_PARAM_PORTDEFINITIONTYPE portFormat;
  OMX_INIT_STRUCTURE(portFormat);
  portFormat.nPortIndex = port;

  omx_err = OMX_GetParameter(m_handle, OMX_IndexParamPortDefinition, &portFormat);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::EnablePort - Error get port %d status on component %s omx_err(0x%08x)",
        port, m_componentName.c_str(), (int)omx_err);
    LOG_ERROR << g_log_buf;
  }

  if(portFormat.bEnabled == OMX_FALSE)
  {
    omx_err = OMX_SendCommand(m_handle, OMX_CommandPortEnable, port, NULL);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreComponent::EnablePort - Error enable port %d on component %s omx_err(0x%08x)",
          port, m_componentName.c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
        return omx_err;
    }
    else
    {
      if(wait)
        omx_err = WaitForCommand(OMX_CommandPortEnable, port);
    }
  }
  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::DisablePort(unsigned int port, bool wait)
{
  if(!m_handle)
    return OMX_ErrorUndefined;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  OMX_PARAM_PORTDEFINITIONTYPE portFormat;
  OMX_INIT_STRUCTURE(portFormat);
  portFormat.nPortIndex = port;

  omx_err = OMX_GetParameter(m_handle, OMX_IndexParamPortDefinition, &portFormat);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::DisablePort - Error get port %d status on component %s omx_err(0x%08x)",
        port, m_componentName.c_str(), (int)omx_err);
    LOG_ERROR << g_log_buf;
  }

  if(portFormat.bEnabled == OMX_TRUE)
  {
    omx_err = OMX_SendCommand(m_handle, OMX_CommandPortDisable, port, NULL);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreComponent::DIsablePort - Error disable port %d on component %s omx_err(0x%08x)",
              port, m_componentName.c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
      return omx_err;
    }
    else
    {
      if(wait)
        omx_err = WaitForCommand(OMX_CommandPortDisable, port);
    }
  }
  return omx_err;
}

OMX_ERRORTYPE COMXCoreComponent::UseEGLImage(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, void* eglImage)
{
if (m_callbacks.FillBufferDone == &COMXCoreComponent::DecoderFillBufferDoneCallback)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  if(!m_handle)
    return OMX_ErrorUndefined;

  m_omx_output_use_buffers = false;

  OMX_PARAM_PORTDEFINITIONTYPE portFormat;
  OMX_INIT_STRUCTURE(portFormat);
  portFormat.nPortIndex = m_output_port;

  omx_err = OMX_GetParameter(m_handle, OMX_IndexParamPortDefinition, &portFormat);
  if(omx_err != OMX_ErrorNone)
    return omx_err;

  if(GetState() != OMX_StateIdle)
  {
    if(GetState() != OMX_StateLoaded)
      SetStateForComponent(OMX_StateLoaded);

    SetStateForComponent(OMX_StateIdle);
  }

  omx_err = EnablePort(m_output_port, false);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "%s EnablePort failed with omx_err(0x%x)", m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
    return omx_err;
  }

  m_output_alignment     = portFormat.nBufferAlignment;
  m_output_buffer_count  = portFormat.nBufferCountActual;
  m_output_buffer_size   = portFormat.nBufferSize;

  if (portFormat.nBufferCountActual != 1)
  {
    sprintf(g_log_buf, "%s::%s - %s nBufferCountActual unexpected %d", CLASSNAME, __func__,
              m_componentName.c_str(), portFormat.nBufferCountActual);
    LOG_ERROR << g_log_buf;
    return omx_err;
  }

  sprintf(g_log_buf, "component(%s) - port(%d), nBufferCountMin(%u), nBufferCountActual(%u), nBufferSize(%u) nBufferAlignmen(%u)",
          m_componentName.c_str(), m_output_port, portFormat.nBufferCountMin,
          portFormat.nBufferCountActual, portFormat.nBufferSize, portFormat.nBufferAlignment);
  LOG_TRACE_2 << g_log_buf;

  for (size_t i = 0; i < portFormat.nBufferCountActual; i++)
  {
    omx_err = OMX_UseEGLImage(m_handle, ppBufferHdr, nPortIndex, pAppPrivate, eglImage);
    if(omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "%s::%s - %s failed with omx_err(0x%x)",
                CLASSNAME, __func__, m_componentName.c_str(), omx_err);
      LOG_ERROR << g_log_buf;
      return omx_err;
    }

    OMX_BUFFERHEADERTYPE *buffer = *ppBufferHdr;
    buffer->nOutputPortIndex = m_output_port;
    buffer->nFilledLen       = 0;
    buffer->nOffset          = 0;
    buffer->pAppPrivate      = (void*)i;
    m_omx_output_buffers.push_back(buffer);
    m_omx_output_available.push(buffer);
  }

  omx_err = WaitForCommand(OMX_CommandPortEnable, m_output_port);
  if(omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "%s EnablePort failed with omx_err(0x%x)", m_componentName.c_str(), omx_err);
    LOG_ERROR << g_log_buf;
      return omx_err;
  }
  m_flush_output = false;

  return omx_err;
}
else
{
  OMX_ERRORTYPE omx_err;
    omx_err = OMX_UseEGLImage(m_handle, ppBufferHdr, nPortIndex, pAppPrivate, eglImage);
    if(omx_err != OMX_ErrorNone)
    {
        sprintf(g_log_buf, "%s failed with omx_err(0x%x)", m_componentName.c_str(), omx_err);
        LOG_ERROR << g_log_buf;
      return omx_err;
    }
  return omx_err;
}
}

bool COMXCoreComponent::Initialize( const std::string &component_name, OMX_INDEXTYPE index, OMX_CALLBACKTYPE *callbacks)
{
  OMX_ERRORTYPE omx_err;

  m_input_port  = 0;
  m_output_port = 0;
  m_handle      = NULL;

  m_input_alignment     = 0;
  m_input_buffer_size  = 0;
  m_input_buffer_count  = 0;

  m_output_alignment    = 0;
  m_output_buffer_size  = 0;
  m_output_buffer_count = 0;
  m_flush_input         = false;
  m_flush_output        = false;
  m_resource_error      = false;

  m_eos                 = false;

  m_exit = false;

  m_omx_input_use_buffers  = false;
  m_omx_output_use_buffers = false;

  m_omx_events.clear();
  m_ignore_error = OMX_ErrorNone;

  m_componentName = component_name;

  m_callbacks.EventHandler    = &COMXCoreComponent::DecoderEventHandlerCallback;
  m_callbacks.EmptyBufferDone = &COMXCoreComponent::DecoderEmptyBufferDoneCallback;
  m_callbacks.FillBufferDone  = &COMXCoreComponent::DecoderFillBufferDoneCallback;

  if (callbacks && callbacks->EventHandler)
    m_callbacks.EventHandler    = callbacks->EventHandler;
  if (callbacks && callbacks->EmptyBufferDone)
    m_callbacks.EmptyBufferDone = callbacks->EmptyBufferDone;
  if (callbacks && callbacks->FillBufferDone)
    m_callbacks.FillBufferDone  = callbacks->FillBufferDone;

  // Get video component handle setting up callbacks, component is in loaded state on return.
  if(!m_handle)
  {
    omx_err = OMX_GetHandle(&m_handle, (char*)component_name.c_str(), this, &m_callbacks);
    if (!m_handle || omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreComponent::Initialize - could not get component handle for %s omx_err(0x%08x)",
          component_name.c_str(), (int)omx_err);
      LOG_ERROR << g_log_buf;
      Deinitialize();
      return false;
    }
  }

  OMX_PORT_PARAM_TYPE port_param;
  OMX_INIT_STRUCTURE(port_param);

  omx_err = OMX_GetParameter(m_handle, index, &port_param);
  if (omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::Initialize - could not get port_param for component %s omx_err(0x%08x)",
        component_name.c_str(), (int)omx_err);
    LOG_ERROR << g_log_buf;
  }

  omx_err = DisableAllPorts();
  if (omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCoreComponent::Initialize - error disable ports on component %s omx_err(0x%08x)",
        component_name.c_str(), (int)omx_err);
    LOG_ERROR << g_log_buf;
  }

  m_input_port  = port_param.nStartPortNumber;
  m_output_port = m_input_port + 1;

  if(m_componentName == "OMX.broadcom.audio_mixer")
  {
    m_input_port  = port_param.nStartPortNumber + 1;
    m_output_port = port_param.nStartPortNumber;
  }

  if (m_output_port > port_param.nStartPortNumber+port_param.nPorts-1)
    m_output_port = port_param.nStartPortNumber+port_param.nPorts-1;

  sprintf(g_log_buf, "COMXCoreComponent::Initialize %s input port %d output port %d m_handle %p",
          m_componentName.c_str(), m_input_port, m_output_port, m_handle);
  LOG_TRACE_2 << g_log_buf;

  m_exit = false;
  m_flush_input   = false;
  m_flush_output  = false;

  return true;
}

void COMXCoreComponent::ResetEos()
{
  pthread_mutex_lock(&m_omx_eos_mutex);
  m_eos = false;
  pthread_mutex_unlock(&m_omx_eos_mutex);
}

bool COMXCoreComponent::Deinitialize()
{
  OMX_ERRORTYPE omx_err;

  m_exit = true;

  m_flush_input   = true;
  m_flush_output  = true;

  if(m_handle)
  {
    FlushAll();

    FreeOutputBuffers();
    FreeInputBuffers();

    TransitionToStateLoaded();

    sprintf(g_log_buf, "COMXCoreComponent::Deinitialize : %s handle %p",
            m_componentName.c_str(), m_handle);
    LOG_TRACE_2 << g_log_buf;
    omx_err = OMX_FreeHandle(m_handle);
    if (omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCoreComponent::Deinitialize - failed to free handle for component %s omx_err(0x%08x)",
          m_componentName.c_str(), omx_err);
      LOG_ERROR << g_log_buf;
    }
    m_handle = NULL;

    m_input_port      = 0;
    m_output_port     = 0;
    m_componentName   = "";
    m_resource_error  = false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
// DecoderEventHandler -- OMX event callback
OMX_ERRORTYPE COMXCoreComponent::DecoderEventHandlerCallback(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 nData1,
  OMX_U32 nData2,
  OMX_PTR pEventData)
{
  if(!pAppData)
    return OMX_ErrorNone;

  COMXCoreComponent *ctx = static_cast<COMXCoreComponent*>(pAppData);
  return ctx->DecoderEventHandler(hComponent, eEvent, nData1, nData2, pEventData);
}

// DecoderEmptyBufferDone -- OMXCore input buffer has been emptied
OMX_ERRORTYPE COMXCoreComponent::DecoderEmptyBufferDoneCallback(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer)
{
  if(!pAppData)
    return OMX_ErrorNone;

  COMXCoreComponent *ctx = static_cast<COMXCoreComponent*>(pAppData);
  return ctx->DecoderEmptyBufferDone( hComponent, pBuffer);
}

// DecoderFillBufferDone -- OMXCore output buffer has been filled
OMX_ERRORTYPE COMXCoreComponent::DecoderFillBufferDoneCallback(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer)
{
  if(!pAppData)
    return OMX_ErrorNone;

  COMXCoreComponent *ctx = static_cast<COMXCoreComponent*>(pAppData);
  return ctx->DecoderFillBufferDone(hComponent, pBuffer);
}

OMX_ERRORTYPE COMXCoreComponent::DecoderEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE* pBuffer)
{
  if(m_exit)
    return OMX_ErrorNone;

  #if defined(OMX_DEBUG_EVENTHANDLER)
  sprintf(g_log_buf, "COMXCoreComponent::DecoderEmptyBufferDone component(%s) %p %d/%d",
          m_componentName.c_str(), pBuffer, m_omx_input_avaliable.size(), m_input_buffer_count);
  LOG_TRACE_2 << g_log_buf;
  #endif

  if(CustomEmptyBufferDoneHandler)
  {
    auto error = (*(CustomEmptyBufferDoneHandler))(hComponent, this, pBuffer);
    (void)error;
  }
  else
  {
    pthread_mutex_lock(&m_omx_input_mutex);
    m_omx_input_avaliable.push(pBuffer);

    // this allows (all) blocked tasks to be awoken
    pthread_cond_broadcast(&m_input_buffer_cond);

    pthread_mutex_unlock(&m_omx_input_mutex);
  }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE COMXCoreComponent::DecoderFillBufferDone(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE* pBuffer)
{
  if(m_exit)
    return OMX_ErrorNone;

  #if defined(OMX_DEBUG_EVENTHANDLER)
  sprintf(g_log_buf, "COMXCoreComponent::DecoderFillBufferDone component(%s) %p %d/%d",
          m_componentName.c_str(), pBuffer, m_omx_output_available.size(), m_output_buffer_count);
  LOG_TRACE_2 << g_log_buf;
  #endif

  if(CustomFillBufferDoneHandler)
  {
    auto error = (*(CustomFillBufferDoneHandler))(hComponent, this, pBuffer);
    (void)error;
  }
  else
  {
    pthread_mutex_lock(&m_omx_output_mutex);
    m_omx_output_available.push(pBuffer);

    // this allows (all) blocked tasks to be awoken
    pthread_cond_broadcast(&m_output_buffer_cond);
    pthread_mutex_unlock(&m_omx_output_mutex);
  }

  return OMX_ErrorNone;
}

// DecoderEmptyBufferDone -- OMXCore input buffer has been emptied
////////////////////////////////////////////////////////////////////////////////////////////
// Component event handler -- OMX event callback
OMX_ERRORTYPE COMXCoreComponent::DecoderEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_EVENTTYPE eEvent,
  OMX_U32 nData1,
  OMX_U32 nData2,
  OMX_PTR pEventData)
{
#ifdef OMX_DEBUG_EVENTS
    sprintf(g_log_buf, "%s eEvent(0x%x), nData1(0x%x), nData2(0x%x), pEventData(0x%p)",
            GetName().c_str(), eEvent, nData1, nData2, pEventData);
    LOG_TRACE_2 << g_log_buf;
#endif

  // if the error is expected, then we can skip it
  if (eEvent == OMX_EventError && (OMX_S32)nData1 == m_ignore_error)
  {
    sprintf(g_log_buf, "%s Ignoring expected event: eEvent(0x%x), nData1(0x%x), nData2(0x%x), pEventData(0x%p)",
            GetName().c_str(), eEvent, nData1, nData2, pEventData);
    LOG_TRACE_2 << g_log_buf;
    m_ignore_error = OMX_ErrorNone;
    return OMX_ErrorNone;
  }
  AddEvent(eEvent, nData1, nData2);

  switch (eEvent)
  {
    case OMX_EventCmdComplete:

      switch(nData1)
      {
        case OMX_CommandStateSet:
          switch ((int)nData2)
          {
            case OMX_StateInvalid:
            #if defined(OMX_DEBUG_EVENTHANDLER)
              LOG_TRACE_2 << GetName() << " - OMX_StateInvalid";
            #endif
            break;
            case OMX_StateLoaded:
            #if defined(OMX_DEBUG_EVENTHANDLER)
              LOG_TRACE_2 << GetName() << " - OMX_StateLoaded";
            #endif
            break;
            case OMX_StateIdle:
            #if defined(OMX_DEBUG_EVENTHANDLER)
              LOG_TRACE_2 << GetName() << " - OMX_StateIdle";
            #endif
            break;
            case OMX_StateExecuting:
            #if defined(OMX_DEBUG_EVENTHANDLER)
              LOG_TRACE_2 << GetName() << " - OMX_StateExecuting";
            #endif
            break;
            case OMX_StatePause:
            #if defined(OMX_DEBUG_EVENTHANDLER)
              LOG_TRACE_2 << GetName() << " - OMX_StatePause";
            #endif
            break;
            case OMX_StateWaitForResources:
            #if defined(OMX_DEBUG_EVENTHANDLER)
              LOG_TRACE_2 << GetName() << " - OMX_StateWaitForResources";
            #endif
            break;
            default:
            #if defined(OMX_DEBUG_EVENTHANDLER)
              sprintf(g_log_buf, "%s - Unknown OMX_Statexxxxx, state(%d)", GetName().c_str(), (int)nData2);
              LOG_TRACE_2 << g_log_buf;
            #endif
            break;
          }
        break;
        case OMX_CommandFlush:
          #if defined(OMX_DEBUG_EVENTHANDLER)
          sprintf(g_log_buf, "%s - OMX_CommandFlush, port %d", GetName().c_str(), (int)nData2);
          LOG_TRACE_2 << g_log_buf;
          #endif
        break;
        case OMX_CommandPortDisable:
          #if defined(OMX_DEBUG_EVENTHANDLER)
          sprintf(g_log_buf, "%s - OMX_CommandPortDisable, nData1(0x%x), port %d",
                  GetName().c_str(), nData1, (int)nData2);
          LOG_TRACE_2 << g_log_buf;
          #endif
        break;
        case OMX_CommandPortEnable:
          #if defined(OMX_DEBUG_EVENTHANDLER)
          sprintf(g_log_buf, "%s::%s %s - OMX_CommandPortEnable, nData1(0x%x), port %d",
                  GetName().c_str(), nData1, (int)nData2);
          LOG_TRACE_2 << g_log_buf;
          #endif
        break;
        #if defined(OMX_DEBUG_EVENTHANDLER)
        case OMX_CommandMarkBuffer:
          sprintf(g_log_buf, "%s - OMX_CommandMarkBuffer, nData1(0x%x), port %d",
                  GetName().c_str(), nData1, (int)nData2);
          LOG_TRACE_2 << g_log_buf;
        break;
        #endif
      }
    break;
    case OMX_EventBufferFlag:
      #if defined(OMX_DEBUG_EVENTHANDLER)
      sprintf(g_log_buf, "%s::%s %s - OMX_EventBufferFlag(input)", GetName().c_str());
      LOG_TRACE_2 << g_log_buf;
      #endif
      if(nData2 & OMX_BUFFERFLAG_EOS)
      {
        pthread_mutex_lock(&m_omx_eos_mutex);
        m_eos = true;
        pthread_mutex_unlock(&m_omx_eos_mutex);
      }
    break;
    case OMX_EventPortSettingsChanged:
      #if defined(OMX_DEBUG_EVENTHANDLER)
      sprintf(g_log_buf, "%s - OMX_EventPortSettingsChanged(output)", GetName().c_str());
      LOG_TRACE_2 << g_log_buf;
      #endif
    break;
    case OMX_EventParamOrConfigChanged:
      #if defined(OMX_DEBUG_EVENTHANDLER)
      sprintf(g_log_buf, "%s - OMX_EventParamOrConfigChanged(output)", GetName().c_str());
      LOG_TRACE_2 << g_log_buf;
      #endif
    break;
    #if defined(OMX_DEBUG_EVENTHANDLER)
    case OMX_EventMark:
      sprintf(g_log_buf, "%s - OMX_EventMark",GetName().c_str());
      LOG_TRACE_2 << g_log_buf;
    break;
    case OMX_EventResourcesAcquired:
      sprintf(g_log_buf, "%s- OMX_EventResourcesAcquired", GetName().c_str());
      LOG_TRACE_2 << g_log_buf;
    break;
    #endif
    case OMX_EventError:
      switch((OMX_S32)nData1)
      {
        case OMX_ErrorSameState:
          //#if defined(OMX_DEBUG_EVENTHANDLER)
          //LOG_ERROR << "%s::%s %s - OMX_ErrorSameState, same state\n", CLASSNAME, __func__, GetName().c_str());
          //#endif
        break;
        case OMX_ErrorInsufficientResources:
          sprintf(g_log_buf, "%s - OMX_ErrorInsufficientResources, insufficient resources",GetName().c_str());
          LOG_ERROR << g_log_buf;
          m_resource_error = true;
        break;
        case OMX_ErrorFormatNotDetected:
          sprintf(g_log_buf, "%s - OMX_ErrorFormatNotDetected, cannot parse input stream", GetName().c_str());
          LOG_ERROR << g_log_buf;
        break;
        case OMX_ErrorPortUnpopulated:
          sprintf(g_log_buf, "%s - OMX_ErrorPortUnpopulated port %d",
                  GetName().c_str(), (int)nData2);
          LOG_WARNING << g_log_buf;
        break;
        case OMX_ErrorStreamCorrupt:
          sprintf(g_log_buf, "%s - OMX_ErrorStreamCorrupt, Bitstream corrupt", GetName().c_str());
          LOG_ERROR << g_log_buf;
          m_resource_error = true;
        break;
        case OMX_ErrorUnsupportedSetting:
          sprintf(g_log_buf, "%s - OMX_ErrorUnsupportedSetting, unsupported setting", GetName().c_str());
          LOG_ERROR << g_log_buf;
        break;
        default:
          sprintf(g_log_buf, "%s - OMX_EventError detected, nData1(0x%x), port %d",
                  GetName().c_str(), nData1, (int)nData2);
          LOG_ERROR << g_log_buf;
        break;
      }
      // wake things up
      if (m_resource_error)
      {
        pthread_cond_broadcast(&m_output_buffer_cond);
        pthread_cond_broadcast(&m_input_buffer_cond);
        pthread_cond_broadcast(&m_omx_event_cond);
      }
    break;
    default:
      sprintf(g_log_buf, "%s - Unknown eEvent(0x%x), nData1(0x%x), port %d",
              GetName().c_str(), eEvent, nData1, (int)nData2);
      LOG_WARNING << g_log_buf;
    break;
  }

  return OMX_ErrorNone;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
COMXCore::COMXCore()
{
  m_is_open = false;
}

COMXCore::~COMXCore()
{

}

bool COMXCore::Initialize()
{
  OMX_ERRORTYPE omx_err = OMX_Init();
  if (omx_err != OMX_ErrorNone)
  {
    sprintf(g_log_buf, "COMXCore::Initialize - OMXCore failed to init, omx_err(0x%08x)", omx_err);
    LOG_ERROR << g_log_buf;
    return false;
  }

  m_is_open = true;
  return true;
}

void COMXCore::Deinitialize()
{
  if(m_is_open)
  {
    OMX_ERRORTYPE omx_err = OMX_Deinit();
    if (omx_err != OMX_ErrorNone)
    {
      sprintf(g_log_buf, "COMXCore::Deinitialize - OMXCore failed to deinit, omx_err(0x%08x)", omx_err);
      LOG_ERROR << g_log_buf;
    }
  }
}
