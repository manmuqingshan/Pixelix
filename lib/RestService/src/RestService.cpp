/* MIT License
 *
 * Copyright (c) 2019 - 2025 Andreas Merkle <web@blue-andi.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*******************************************************************************
    DESCRIPTION
*******************************************************************************/
/**
 * @brief  REST service
 * @author Niklas Kümmel (niklas-kuemmel@web.de)
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "RestService.h"

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/******************************************************************************
 * Prototypes
 *****************************************************************************/

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/******************************************************************************
 * Public Methods
 *****************************************************************************/

bool RestService::start()
{
    return true;
}

void RestService::stop()
{
}

void RestService::process()
{
    if (true == m_mutex.take(portMAX_DELAY))
    {

        Cmd cmd;

        if (true == m_cmdQueue.receive(&cmd, 0U))
        {
            if (m_Callbacks.find(cmd.restId) != m_Callbacks.end())
            {
                m_client.regOnResponse(m_Callbacks[cmd.restId].first);
                m_client.regOnError(m_Callbacks[cmd.restId].second);
            }
            else
            {
                Msg msg;

                msg.restId = cmd.restId;
                msg.isMsg  = false;
                msg.rsp    = nullptr;

                if (true == this->m_taskProxy.send(msg))
                {
                    LOG_ERROR("No Callbacks set");
                }

                m_mutex.give();
            }

            if (true == m_client.begin(cmd.url))
            {
                switch (cmd.id)
                {
                case CMD_ID_GET:
                    if (false == m_client.GET(cmd.restId))
                    {
                        Msg msg;

                        msg.restId = cmd.restId;
                        msg.isMsg  = false;
                        msg.rsp    = nullptr;

                        if (true == this->m_taskProxy.send(msg))
                        {
                            LOG_ERROR("Msg could not be sent to Msg-Queue");
                        }

                        m_mutex.give();
                    }
                    break;

                case CMD_ID_POST:
                    if (false == m_client.POST(cmd.u.data.data, cmd.u.data.size, cmd.restId))
                    {
                        Msg msg;

                        msg.restId = cmd.restId;
                        msg.isMsg  = false;
                        msg.rsp    = nullptr;

                        if (true == this->m_taskProxy.send(msg))
                        {
                            LOG_ERROR("Msg could not be sent to Msg-Queue");
                        }

                        m_mutex.give();
                    }
                    break;

                default:
                    break;
                };
            }
            else
            {
                Msg msg;

                msg.restId = cmd.restId;
                msg.isMsg  = false;
                msg.rsp    = nullptr;

                if (true == this->m_taskProxy.send(msg))
                {
                    LOG_ERROR("URL could not be parsed");
                }

                m_mutex.give();
            }
        }
        else
        {
            m_mutex.give();
        }
    }
}

int RestService::getRestId()
{
    return m_restIdCounter++;
}

void RestService::setCallbacks(int32_t* restId, AsyncHttpClient::OnResponse rsp_cb, AsyncHttpClient::OnError err_cb)
{
    if (m_Callbacks.find(restId) == m_Callbacks.end())
    {
        m_Callbacks[restId] = std::make_pair(rsp_cb, err_cb);
    }
}

void RestService::deleteCallbacks(int32_t* restId)
{
    m_Callbacks.erase(restId);
}

bool RestService::get(int32_t* restId, const String& url)
{
    Cmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.id     = CMD_ID_GET;
    cmd.restId = restId;
    cmd.url    = url;

    return m_cmdQueue.sendToBack(cmd, portMAX_DELAY);
}

bool RestService::post(int32_t* restId, const String& url, const uint8_t* payload, size_t size)
{
    Cmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.id          = CMD_ID_POST;
    cmd.restId      = restId;
    cmd.url         = url;
    cmd.u.data.data = payload;
    cmd.u.data.size = size;

    return m_cmdQueue.sendToBack(cmd, portMAX_DELAY);
}

bool RestService::post(int32_t* restId, const String& url, const String& payload)
{
    Cmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.id          = CMD_ID_POST;
    cmd.restId      = restId;
    cmd.url         = url;
    cmd.u.data.data = reinterpret_cast<const uint8_t*>(payload.c_str());
    cmd.u.data.size = payload.length();

    return m_cmdQueue.sendToBack(cmd, portMAX_DELAY);
}

bool RestService::getResponse(int32_t* restId, bool& isValidRsp, DynamicJsonDocument* payload)
{
    bool isSuccessful = false;
    Msg  msg;

    if (true == m_taskProxy.peek(msg))
    {
        if (restId == msg.restId)
        {
            isValidRsp   = msg.isMsg;
            payload      = msg.rsp;
            isSuccessful = true;

            if (false == m_taskProxy.receive(msg))
            {
                LOG_ERROR("Two Plugins with the same restId exist!");
            }
        }
    }

    return isSuccessful;
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
