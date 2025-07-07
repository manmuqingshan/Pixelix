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
    MutexGuard<Mutex>           guard(m_mutex);

    AsyncHttpClient::OnResponse rspCallback = [this](const HttpResponse& rsp) {
        handleAsyncWebResponse(rsp);
    };
    AsyncHttpClient::OnError errCallback = [this]() {
        handleFailedWebRequest();
    };
    AsyncHttpClient::OnClosed closedCallback = [this]() {
        m_isWaitingForResponse = false;
    };

    m_client.regOnResponse(rspCallback);
    m_client.regOnError(errCallback);
    m_client.regOnClosed(closedCallback);
    m_isRunning = true;

    return true;
}

void RestService::stop()
{
    MutexGuard<Mutex> guard(m_mutex);

    m_isRunning = false;
    m_client.regOnResponse(nullptr);
    m_client.regOnError(nullptr);
    m_client.regOnClosed(nullptr);
    m_cmdQueue.clear();
    m_client.end();
    m_msgQueue.clear();
}

void RestService::process()
{
    MutexGuard<Mutex> guard(m_mutex);

    if (false == m_isWaitingForResponse)
    {
        bool isError = false;
        Cmd  cmd;

        if (false == m_cmdQueue.empty())
        {
            m_isWaitingForResponse = true;

            /* Take first cmd from queue. */
            cmd                    = std::move(m_cmdQueue.front());
            m_cmdQueue.erase(m_cmdQueue.begin());
            m_activeRestId             = cmd.restId;
            m_activePreProcessCallback = cmd.preProcessCallback;

            if (true == m_client.begin(String(cmd.url)))
            {
                switch (cmd.id)
                {
                case CMD_ID_GET:
                    if (false == m_client.GET())
                    {
                        isError = true;
                    }
                    break;

                case CMD_ID_POST:
                    if (false == m_client.POST(cmd.data.data, cmd.data.size))
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

        if (true == isError)
        {
            Msg msg(0U);

            msg.restId = cmd.restId;
            msg.isMsg  = false;
            m_msgQueue.push_back(std::move(msg));
            m_activeRestId             = INVALID_REST_ID;
            m_activePreProcessCallback = nullptr;
            m_isWaitingForResponse     = false;
        }
    }
}

uint32_t RestService::get(const String& url, PreProcessCallback preProcessCallback)
{
    MutexGuard<Mutex> guard(m_mutex);
    bool              isSuccessful = true;
    uint32_t          restId;

    if (true == m_isRunning)
    {
        Cmd cmd;

        restId                 = getRestId();
        cmd.id                 = CMD_ID_GET;
        cmd.restId             = restId;
        cmd.preProcessCallback = preProcessCallback;
        cmd.url                = url;

        m_cmdQueue.push_back(std::move(cmd));
    }
    else
    {
        restId = INVALID_REST_ID;
    }

    return restId;
}

uint32_t RestService::post(const String& url, PreProcessCallback preProcessCallback, const uint8_t* payload, size_t size)
{
    MutexGuard<Mutex> guard(m_mutex);
    bool              isSuccessful = true;
    uint32_t          restId;

    if (true == m_isRunning)
    {
        Cmd cmd;

        restId                 = getRestId();
        cmd.id                 = CMD_ID_POST;
        cmd.restId             = restId;
        cmd.preProcessCallback = preProcessCallback;
        cmd.url                = url;
        cmd.data.data          = payload;
        cmd.data.size          = size;

        m_cmdQueue.push_back(std::move(cmd));
    }
    else
    {
        restId = INVALID_REST_ID;
    }

    return restId;
}

uint32_t RestService::post(const String& url, const String& payload, PreProcessCallback preProcessCallback)
{
    MutexGuard<Mutex> guard(m_mutex);
    bool              isSuccessful = true;
    uint32_t          restId;

    if (true == m_isRunning)
    {
        Cmd cmd;

        restId                 = getRestId();
        cmd.id                 = CMD_ID_POST;
        cmd.restId             = restId;
        cmd.url                = url;
        cmd.preProcessCallback = preProcessCallback;
        cmd.data.data          = reinterpret_cast<const uint8_t*>(payload.c_str());
        cmd.data.size          = payload.length();

        m_cmdQueue.push_back(std::move(cmd));
    }
    else
    {
        restId = INVALID_REST_ID;
    }

    return restId;
}

bool RestService::getResponse(uint32_t restId, bool& isValidRsp, DynamicJsonDocument& payload)
{
    MutexGuard<Mutex> guard(m_mutex);
    bool              isSuccessful = false;

    if (true == m_isRunning)
    {
        MsgQueue::iterator msgIterator = m_msgQueue.begin();

        while (msgIterator != m_msgQueue.end())
        {
            if (restId == msgIterator->restId)
            {
                isValidRsp   = msgIterator->isMsg;
                payload      = std::move(msgIterator->data);
                msgIterator  = m_msgQueue.erase(msgIterator);
                isSuccessful = true;
                break;
            }
            else
            {
                ++msgIterator;
            }
        }
    }
    else
    {
        isValidRsp   = false;
        isSuccessful = true;
    }

    return isSuccessful;
}

void RestService::removeExpiredResponse(uint32_t restId)
{
    MutexGuard<Mutex>  guard(m_mutex);
    bool               wasFound    = false;
    CmdQueue::iterator cmdIterator = m_cmdQueue.begin();
    MsgQueue::iterator msgIterator = m_msgQueue.begin();

    while ((false == wasFound) && (cmdIterator != m_cmdQueue.end()))
    {
        if (restId == cmdIterator->restId)
        {
            cmdIterator = m_cmdQueue.erase(cmdIterator);
            wasFound    = true;
        }
        else
        {
            ++cmdIterator;
        }
    }

    if (m_activeRestId == restId)
    {
        m_client.end();
        wasFound = true;
    }

    while ((false == wasFound) && (msgIterator != m_msgQueue.end()))
    {
        if (restId == msgIterator->restId)
        {
            msgIterator = m_msgQueue.erase(msgIterator);
            wasFound    = true;
        }
        else
        {
            ++msgIterator;
        }
    }
}

void RestService::handleAsyncWebResponse(const HttpResponse& rsp)
{
    MutexGuard<Mutex> guard(m_mutex);
    Msg               msg;
    bool              isError = false;

    msg.restId                = m_activeRestId;

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
            if (true == m_activePreProcessCallback(payload, payloadSize, msg.data))
            {
                msg.isMsg = true;
                m_msgQueue.push_back(std::move(msg));
            }
            else
            {
                isError = true;
                LOG_ERROR("Error while preprocessing!");
            }
        }
        else
        {
            DeserializationError error = deserializeJson(msg.data, payload, payloadSize);

            if (DeserializationError::Ok != error.code())
            {
                LOG_WARNING("JSON parse error: %s", error.c_str());
                isError = true;
            }
            else
            {
                msg.isMsg = true;
                m_msgQueue.push_back(std::move(msg));
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
        msg.isMsg = false;
        msg.data.clear();
        m_msgQueue.push_back(std::move(msg));
    }

    m_activeRestId             = INVALID_REST_ID;
    m_activePreProcessCallback = nullptr;
}

void RestService::handleFailedWebRequest()
{
    MutexGuard<Mutex> guard(m_mutex);
    Msg               msg(0U);

    msg.restId = m_activeRestId;
    msg.isMsg  = false;
    m_msgQueue.push_back(std::move(msg));

    m_activeRestId             = INVALID_REST_ID;
    m_activePreProcessCallback = nullptr;
}

uint32_t RestService::getRestId()
{
    uint32_t restId;

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

    return restId;
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
