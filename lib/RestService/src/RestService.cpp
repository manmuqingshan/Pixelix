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
    AsyncHttpClient::OnResponse rspCallback  = [this](const HttpResponse& rsp) {
        handleAsyncWebResponse(rsp);
    };
    AsyncHttpClient::OnError errCallback = [this]() {
        handleFailedWebRequest();
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

    m_isRunning = true;

    return isSuccessful;
}

void RestService::stop()
{
    Cmd* cmd    = nullptr;
    Msg* msg    = nullptr;

    m_isRunning = false;
    m_client.regOnResponse(nullptr);
    m_client.regOnError(nullptr);
    m_client.regOnClosed(nullptr);

    while (true == m_cmdQueue.receive(&cmd, 0U))
    {
        if (nullptr != cmd)
        {
            delete cmd;
            cmd = nullptr;
        }
    }

    m_client.end();

    while (true == m_taskProxy.receive(msg))
    {
        if (nullptr != msg)
        {
            delete msg;
            msg = nullptr;
        }
    }

    m_cmdQueue.destroy();
}

void RestService::process()
{
    if (false == m_isWaitingForResponse)
    {
        bool isError = false;
        Cmd* cmd     = nullptr;

        if (true == m_cmdQueue.receive(&cmd, 0U))
        {
            m_isWaitingForResponse = true;

            if (nullptr != cmd)
            {
                m_activeRestId             = cmd->restId;
                m_activePreProcessCallback = cmd->preProcessCallback;

                if (true == m_client.begin(String(cmd->url)))
                {
                    switch (cmd->id)
                    {
                    case CMD_ID_GET:
                        if (false == m_client.GET())
                        {
                            isError = true;
                        }
                        break;

                    case CMD_ID_POST:
                        if (false == m_client.POST(cmd->u.data.data, cmd->u.data.size))
                        {
                            isError = true;
                        }
                        break;

                    default:
                        break;
                    };
                }
                else
                {
                    LOG_ERROR("URL could not be parsed");
                    isError = true;
                }
            }
            else
            {
                isError = true;
            }

            if (true == isError)
            {
                Msg* msg = new (std::nothrow) Msg();

                if (nullptr == msg)
                {
                    LOG_FATAL("Couldn't allocate enough memory for a msg.");
                }
                else
                {
                    msg->restId = cmd->restId;
                    msg->isMsg  = false;

                    if (false == this->m_taskProxy.send(msg))
                    {
                        LOG_ERROR("Msg could not be sent to Msg-Queue");
                    }
                }

                m_activeRestId             = INVALID_REST_ID;
                m_activePreProcessCallback = nullptr;
                m_isWaitingForResponse     = false;
            }
        }

        if (nullptr != cmd)
        {
            delete cmd;
            cmd = nullptr;
        }

        removeExpiredResponses();
    }
}

uint32_t RestService::get(const String& url, PreProcessCallback preProcessCallback)
{
    bool     isSuccessful = true;
    uint32_t restId;
    Cmd*     cmd = new (std::nothrow) Cmd();

    if (true == m_isRunning)
    {
        getRestId(restId);

        if (nullptr == cmd)
        {
            LOG_ERROR("Couldn't allocate enough memory.");
            isSuccessful = false;
        }
        else
        {
            cmd->id                 = CMD_ID_GET;
            cmd->restId             = restId;
            cmd->preProcessCallback = preProcessCallback;
            cmd->url                = url;
            isSuccessful            = m_cmdQueue.sendToBack(cmd, portMAX_DELAY);
        }
    }
    else
    {
        isSuccessful = false;
    }

    if (false == isSuccessful)
    {
        if (nullptr != cmd)
        {
            delete cmd;
            cmd = nullptr;
        }

        restId = INVALID_REST_ID;
    }

    return restId;
}

uint32_t RestService::post(const String& url, PreProcessCallback preProcessCallback, const uint8_t* payload, size_t size)
{
    bool     isSuccessful = true;
    uint32_t restId;
    Cmd*     cmd = new (std::nothrow) Cmd();

    if (true == m_isRunning)
    {
        getRestId(restId);

        if (nullptr == cmd)
        {
            LOG_ERROR("Couldn't allocate enough memory.");
            isSuccessful = false;
        }
        else
        {
            cmd->id                 = CMD_ID_POST;
            cmd->restId             = restId;
            cmd->preProcessCallback = preProcessCallback;
            cmd->url                = url;
            cmd->u.data.data        = payload;
            cmd->u.data.size        = size;
            isSuccessful            = m_cmdQueue.sendToBack(cmd, portMAX_DELAY);
        }
    }
    else
    {
        isSuccessful = false;
    }

    if (false == isSuccessful)
    {
        if (nullptr != cmd)
        {
            delete cmd;
            cmd = nullptr;
        }

        restId = INVALID_REST_ID;
    }

    return restId;
}

uint32_t RestService::post(const String& url, const String& payload, PreProcessCallback preProcessCallback)
{
    bool     isSuccessful = true;
    uint32_t restId;
    Cmd*     cmd = new (std::nothrow) Cmd();

    if (true == m_isRunning)
    {
        getRestId(restId);

        if (nullptr == cmd)
        {
            LOG_ERROR("Couldn't allocate enough memory.");
            isSuccessful = false;
        }
        else
        {
            cmd->id                 = CMD_ID_POST;
            cmd->restId             = restId;
            cmd->url                = url;
            cmd->preProcessCallback = preProcessCallback;
            cmd->u.data.data        = reinterpret_cast<const uint8_t*>(payload.c_str());
            cmd->u.data.size        = payload.length();
            isSuccessful            = m_cmdQueue.sendToBack(cmd, portMAX_DELAY);
        }
    }
    else
    {
        isSuccessful = false;
    }

    if (false == isSuccessful)
    {
        if (nullptr != cmd)
        {
            delete cmd;
            cmd = nullptr;
        }

        restId = INVALID_REST_ID;
    }

    return restId;
}

bool RestService::getResponse(uint32_t restId, bool& isValidRsp, DynamicJsonDocument& payload)
{
    bool isSuccessful = false;
    Msg* msg          = nullptr;

    if (true == m_isRunning)
    {
        if (true == m_taskProxy.peek(msg))
        {
            if (restId == msg->restId)
            {
                isValidRsp   = msg->isMsg;
                payload      = std::move(msg->rsp);
                isSuccessful = true;

                if (false == m_taskProxy.receive(msg))
                {
                    LOG_FATAL("Two clients with the same restId exist!");
                }
            }
        }
    }
    else
    {
        isValidRsp = false;
    }

    return isSuccessful;
}

void RestService::addToRemovedPluginIds(uint32_t restId)
{
    if (INVALID_REST_ID == restId)
    {
        LOG_ERROR("Cannot add INVALID_REST_ID to removedPluginIds!");
    }
    else
    {
        removedPluginIds.push_back(restId);
    }
}

void RestService::handleAsyncWebResponse(const HttpResponse& rsp)
{
    Msg* msg     = new (std::nothrow) Msg();
    bool isError = false;

    if (nullptr != msg)
    {
        msg->restId = m_activeRestId;

        if (HttpStatus::STATUS_CODE_OK == rsp.getStatusCode())
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

            /* If a callback is found, it shall be applied. */
            else if (nullptr != m_activePreProcessCallback)
            {
                if (true == m_activePreProcessCallback(payload, payloadSize, msg->rsp))
                {
                    msg->isMsg = true;

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
                DeserializationError error = deserializeJson(msg->rsp, payload, payloadSize);

                if (DeserializationError::Ok != error.code())
                {
                    LOG_WARNING("JSON parse error: %s", error.c_str());
                    isError = true;
                }
                else
                {
                    msg->isMsg = true;

                    if (false == m_taskProxy.send(msg))
                    {
                        isError = true;
                    }
                }
            }
        }
        else
        {
            isError = true;
            LOG_ERROR("Http-Status not ok");
        }

        if (true == isError)
        {
            msg->isMsg = false;
            msg->rsp.clear();

            if (false == m_taskProxy.send(msg))
            {
                LOG_ERROR("Msg could not be sent to Msg-Queue");
            }
        }
    }
    else
    {
        LOG_FATAL("Couldn't allocate enough memory for a msg.");
    }

    m_activeRestId             = INVALID_REST_ID;
    m_activePreProcessCallback = nullptr;
}

void RestService::handleFailedWebRequest()
{
    Msg* msg = new (std::nothrow) Msg();

    if (nullptr != msg)
    {
        msg->restId = m_activeRestId;
        msg->isMsg  = false;

        if (false == m_taskProxy.send(msg))
        {
            LOG_ERROR("Msg could not be sent to Msg-Queue");
        }
    }
    else
    {
        LOG_FATAL("Couldn't allocate enough memory for a msg.");
    }

    m_activeRestId             = INVALID_REST_ID;
    m_activePreProcessCallback = nullptr;
}

void RestService::removeExpiredResponses()
{
    bool                   isValidRsp = false;
    DynamicJsonDocument    jsonDoc(0U);
    PluginIdList::iterator idIterator = removedPluginIds.begin();

    while (idIterator != removedPluginIds.end())
    {
        if (true == getResponse(*idIterator, isValidRsp, jsonDoc))
        {
            idIterator = removedPluginIds.erase(idIterator);
        }
        else
        {
            ++idIterator;
        }
    }
}

void RestService::getRestId(uint32_t& restId)
{
    if (INVALID_REST_ID == m_restIdCounter)
    {
        /* Skip the INVALID_REST_ID and use the next one. */
        ++m_restIdCounter;
        restId = m_restIdCounter;
        ++m_restIdCounter;
    }
    else
    {
        restId = m_restIdCounter;
        ++m_restIdCounter;
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
