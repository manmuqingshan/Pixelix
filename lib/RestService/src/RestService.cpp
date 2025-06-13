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
    bool                        isSuccessful = false;
    AsyncHttpClient::OnResponse rspCallback  = [this](void* restId, const HttpResponse& rsp) {
        handleAsyncWebResponse(restId, rsp);
    };
    AsyncHttpClient::OnError errCallback = [this](void* restId) {
        handleFailedWebRequest(restId);
    };
    AsyncHttpClient::OnClosed closedCallback = [this]() {
        m_isWaitingForResponse = false;
    };

    isSuccessful = m_cmdQueue.create(CMD_QUEUE_SIZE);

    if (true == isSuccessful)
    {
        m_client.regOnResponse(rspCallback);
        m_client.regOnError(errCallback);
        m_client.regOnClosed(closedCallback);
    }

    return isSuccessful;
}

void RestService::stop()
{
    m_client.regOnResponse(nullptr);
    m_client.regOnError(nullptr);
    m_client.regOnClosed(nullptr);

    m_cmdQueue.destroy();
}

void RestService::process()
{
    if (false == m_isWaitingForResponse)
    {
        Cmd* cmd;

        if (true == m_cmdQueue.receive(&cmd, 0U))
        {
            m_isWaitingForResponse = true;

            if (true == m_client.begin(String(cmd->url)))
            {
                switch (cmd->id)
                {
                case CMD_ID_GET:
                    if (false == m_client.GET(cmd->restId))
                    {
                        Msg msg;

                        msg.restId = cmd->restId;
                        msg.isMsg  = false;
                        msg.rsp    = nullptr;

                        if (false == this->m_taskProxy.send(msg))
                        {
                            LOG_ERROR("Msg could not be sent to Msg-Queue");
                        }

                        m_isWaitingForResponse = false;
                    }
                    break;

                case CMD_ID_POST:
                    if (false == m_client.POST(cmd->u.data.data, cmd->u.data.size, cmd->restId))
                    {
                        Msg msg;

                        msg.restId = cmd->restId;
                        msg.isMsg  = false;
                        msg.rsp    = nullptr;

                        if (false == this->m_taskProxy.send(msg))
                        {
                            LOG_ERROR("Msg could not be sent to Msg-Queue");
                        }

                        m_isWaitingForResponse = false;
                    }
                    break;

                default:
                    break;
                };
            }
            else
            {
                Msg msg;

                msg.restId = cmd->restId;
                msg.isMsg  = false;
                msg.rsp    = nullptr;

                if (false == this->m_taskProxy.send(msg))
                {
                    LOG_ERROR("URL could not be parsed");
                }

                m_isWaitingForResponse = false;
            }
        }
    }
}

void RestService::setCallback(void* restId, PreProcessCallback preProcessCallback)
{
    if (nullptr == restId)
    {
        LOG_ERROR("Callback cannot be set with nullptr as restId!");
    }
    else
    {
        if (m_Callbacks.find(restId) == m_Callbacks.end())
        {
            m_Callbacks[restId] = preProcessCallback;
        }
    }
}

void RestService::deleteCallback(void* restId)
{
    if (nullptr == restId)
    {
        LOG_ERROR("Cannot delete callback for restId: nullptr!");
    }
    else
    {
        m_Callbacks.erase(restId);
    }
}

bool RestService::get(void* restId, const String& url)
{
    bool isSuccessful = true;

    if (restId == nullptr)
    {
        isSuccessful = false;
    }
    else if (url.length() > (CMD_URL_SIZE - 1U))
    {
        isSuccessful = false;
    }
    else
    {
        Cmd* cmd    = new Cmd();

        cmd->id     = CMD_ID_GET;
        cmd->restId = restId;
        strncpy(cmd->url, url.c_str(), sizeof(cmd->url) - 1U);
        cmd->url[sizeof(cmd->url) - 1U] = '\0';
        isSuccessful                    = m_cmdQueue.sendToBack(cmd, portMAX_DELAY);
    }

    return isSuccessful;
}

bool RestService::post(void* restId, const String& url, const uint8_t* payload, size_t size)
{
    bool isSuccessful = true;

    if (restId == nullptr)
    {
        isSuccessful = false;
    }
    else if (url.length() > (CMD_URL_SIZE - 1U))
    {
        isSuccessful = false;
    }
    else
    {
        Cmd* cmd    = new Cmd();

        cmd->id     = CMD_ID_POST;
        cmd->restId = restId;
        strncpy(cmd->url, url.c_str(), sizeof(cmd->url) - 1U);
        cmd->url[sizeof(cmd->url) - 1U] = '\0';
        cmd->u.data.data                = payload;
        cmd->u.data.size                = size;
        isSuccessful                    = m_cmdQueue.sendToBack(cmd, portMAX_DELAY);
    }

    return isSuccessful;
}

bool RestService::post(void* restId, const String& url, const String& payload)
{
    bool isSuccessful = true;

    if (restId == nullptr)
    {
        isSuccessful = false;
    }
    else if (url.length() > (CMD_URL_SIZE - 1U))
    {
        isSuccessful = false;
    }
    else
    {
        Cmd* cmd    = new Cmd();

        cmd->id     = CMD_ID_POST;
        cmd->restId = restId;
        strncpy(cmd->url, url.c_str(), sizeof(cmd->url) - 1U);
        cmd->url[sizeof(cmd->url) - 1U] = '\0';
        cmd->u.data.data                = reinterpret_cast<const uint8_t*>(payload.c_str());
        cmd->u.data.size                = payload.length();
        isSuccessful                    = m_cmdQueue.sendToBack(cmd, portMAX_DELAY);
    }

    return isSuccessful;
}

bool RestService::getResponse(void* restId, bool& isValidRsp, DynamicJsonDocument*& payload)
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
                LOG_FATAL("Two clients with the same restId exist!");
            }
        }
    }

    return isSuccessful;
}

void RestService::handleAsyncWebResponse(void* restId, const HttpResponse& rsp)
{
    const size_t         JSON_DOC_SIZE = 4096U;
    DynamicJsonDocument* jsonDoc       = new (std::nothrow) DynamicJsonDocument(JSON_DOC_SIZE);
    bool                 isError       = false;
    Msg                  msg;

    msg.restId = restId;

    if (HttpStatus::STATUS_CODE_OK == rsp.getStatusCode())
    {
        if (nullptr != jsonDoc)
        {
            bool        isSuccessful = false;
            size_t      payloadSize  = 0U;
            const void* vPayload     = rsp.getPayload(payloadSize);
            const char* payload      = static_cast<const char*>(vPayload);

            if ((nullptr == payload) ||
                (0U == payloadSize))
            {
                LOG_ERROR("No payload.");
                isError = true;
            }

            /* If a filter is found, it shall be applied.*/
            else if (m_Callbacks.find(restId) != m_Callbacks.end())
            {
                if (true == m_Callbacks[restId](payload, payloadSize, *jsonDoc))
                {
                    msg.isMsg = true;
                    msg.rsp   = jsonDoc;

                    if (false == m_taskProxy.send(msg))
                    {
                        isError = true;
                        LOG_ERROR("Could not send msg with rsp");
                    }
                }
                else
                {
                    isError = true;
                    LOG_ERROR("Error while preprocessing!");
                }
            }
            else
            {
                DeserializationError error = deserializeJson(*jsonDoc, payload, payloadSize);

                if (DeserializationError::Ok != error.code())
                {
                    LOG_WARNING("JSON parse error: %s", error.c_str());
                    isError = true;
                }
                else
                {
                    msg.isMsg = true;
                    msg.rsp   = jsonDoc;

                    if (false == m_taskProxy.send(msg))
                    {
                        isError = true;
                    }
                }
            }
        }
        else
        {
            LOG_ERROR("Not enough memory available to store DynamicJsonDocument");
            isError = true;
        }
    }
    else
    {
        isError = true;
        LOG_ERROR("Http-Status not ok");
    }

    if (true == isError)
    {
        delete jsonDoc;
        jsonDoc   = nullptr;
        msg.isMsg = false;
        msg.rsp   = nullptr;

        if (false == m_taskProxy.send(msg))
        {
            LOG_ERROR("Msg could not be sent to Msg-Queue");
        }
    }
}

void RestService::handleFailedWebRequest(void* restId)
{
    Msg msg;

    msg.restId = restId;
    msg.isMsg  = false;
    msg.rsp    = nullptr;

    if (false == m_taskProxy.send(msg))
    {
        LOG_ERROR("Msg could not be sent to Msg-Queue");
    }
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
