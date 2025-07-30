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
    m_requestQueue.clear();
    m_client.end();
    m_responseQueue.clear();
    m_activeRestId             = INVALID_REST_ID;
    m_activePreProcessCallback = nullptr;
}

void RestService::process()
{
    MutexGuard<Mutex> guard(m_mutex);

    if (false == m_isWaitingForResponse)
    {
        bool    isError = false;
        Request req;

        if (false == m_requestQueue.empty())
        {
            m_isWaitingForResponse = true;

            /* Take first request from queue. */
            req                    = std::move(m_requestQueue.front());
            m_requestQueue.erase(m_requestQueue.begin());
            m_activeRestId             = req.restId;
            m_activePreProcessCallback = req.preProcessCallback;

            if (true == m_client.begin(String(req.url)))
            {
                switch (req.id)
                {
                case REQUEST_ID_GET:
                    if (false == m_client.GET())
                    {
                        isError = true;
                    }
                    break;

                case REQUEST_ID_POST:
                    if (false == m_client.POST(req.data.data, req.data.size))
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
            Response rsp(0U);

            rsp.restId = req.restId;
            rsp.isRsp  = false;
            m_responseQueue.push_back(std::move(rsp));
            m_activeRestId             = INVALID_REST_ID;
            m_activePreProcessCallback = nullptr;
            m_isWaitingForResponse     = false;
        }
    }
}

uint32_t RestService::get(const String& url, PreProcessCallback preProcessCallback)
{
    MutexGuard<Mutex> guard(m_mutex);
    uint32_t          restId = INVALID_REST_ID;

    if (true == m_isRunning)
    {
        Request req;

        restId                 = getRestId();
        req.id                 = REQUEST_ID_GET;
        req.restId             = restId;
        req.preProcessCallback = preProcessCallback;
        req.url                = url;

        m_requestQueue.push_back(std::move(req));
    }

    return restId;
}

uint32_t RestService::post(const String& url, PreProcessCallback preProcessCallback, const uint8_t* payload, size_t size)
{
    MutexGuard<Mutex> guard(m_mutex);
    uint32_t          restId = INVALID_REST_ID;

    if (true == m_isRunning)
    {
        Request req;

        restId                 = getRestId();
        req.id                 = REQUEST_ID_POST;
        req.restId             = restId;
        req.preProcessCallback = preProcessCallback;
        req.url                = url;
        req.data.data          = payload;
        req.data.size          = size;

        m_requestQueue.push_back(std::move(req));
    }

    return restId;
}

uint32_t RestService::post(const String& url, const String& payload, PreProcessCallback preProcessCallback)
{
    MutexGuard<Mutex> guard(m_mutex);
    uint32_t          restId = INVALID_REST_ID;

    if (true == m_isRunning)
    {
        Request req;

        restId                 = getRestId();
        req.id                 = REQUEST_ID_POST;
        req.restId             = restId;
        req.url                = url;
        req.preProcessCallback = preProcessCallback;
        req.data.data          = reinterpret_cast<const uint8_t*>(payload.c_str());
        req.data.size          = payload.length();

        m_requestQueue.push_back(std::move(req));
    }

    return restId;
}

bool RestService::getResponse(uint32_t restId, bool& isValidRsp, DynamicJsonDocument& payload)
{
    bool isSuccessful = false;

    if (INVALID_REST_ID != restId)
    {
        MutexGuard<Mutex> guard(m_mutex);

        if (true == m_isRunning)
        {
            ResponseQueue::iterator rspIterator = m_responseQueue.begin();

            while (rspIterator != m_responseQueue.end())
            {
                if (restId == rspIterator->restId)
                {
                    isValidRsp   = rspIterator->isRsp;
                    payload      = std::move(rspIterator->jsonDocData);
                    rspIterator  = m_responseQueue.erase(rspIterator);
                    isSuccessful = true;
                    break;
                }
                else
                {
                    ++rspIterator;
                }
            }
        }
        /* When the RestService is stopped, isSuccessful is always true, ensuring that any user still waiting receives an empty response. */
        else
        {
            isValidRsp   = false;
            isSuccessful = true;
        }
    }

    return isSuccessful;
}

void RestService::abortRequest(uint32_t restId)
{
    MutexGuard<Mutex>       guard(m_mutex);
    bool                    isRequestFound = false;
    RequestQueue::iterator  reqIterator    = m_requestQueue.begin();
    ResponseQueue::iterator rspIterator    = m_responseQueue.begin();

    while ((false == isRequestFound) && (reqIterator != m_requestQueue.end()))
    {
        if (restId == reqIterator->restId)
        {
            reqIterator    = m_requestQueue.erase(reqIterator);
            isRequestFound = true;
        }
        else
        {
            ++reqIterator;
        }
    }

    if (m_activeRestId == restId)
    {
        m_client.end();
        m_activeRestId             = INVALID_REST_ID;
        m_activePreProcessCallback = nullptr;
        isRequestFound             = true;
        m_isWaitingForResponse     = false;
    }

    while ((false == isRequestFound) && (rspIterator != m_responseQueue.end()))
    {
        if (restId == rspIterator->restId)
        {
            rspIterator    = m_responseQueue.erase(rspIterator);
            isRequestFound = true;
        }
        else
        {
            ++rspIterator;
        }
    }
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

void RestService::handleAsyncWebResponse(const HttpResponse& httpRsp)
{
    MutexGuard<Mutex> guard(m_mutex);
    Response          rsp;
    bool              isError = false;

    rsp.restId                = m_activeRestId;

    if (HttpStatus::STATUS_CODE_OK == httpRsp.getStatusCode())
    {
        size_t      payloadSize = 0U;
        const void* vPayload    = httpRsp.getPayload(payloadSize);
        const char* payload     = static_cast<const char*>(vPayload);

        if ((nullptr == payload) ||
            (0U == payloadSize))
        {
            LOG_ERROR("No payload.");
            isError = true;
        }

        /* If a callback is found, it shall be applied. */
        else if (nullptr != m_activePreProcessCallback)
        {
            if (true == m_activePreProcessCallback(payload, payloadSize, rsp.jsonDocData))
            {
                rsp.isRsp = true;
                m_responseQueue.push_back(std::move(rsp));
            }
            else
            {
                isError = true;
                LOG_ERROR("Error while preprocessing!");
            }
        }
        else
        {
            DeserializationError error = deserializeJson(rsp.jsonDocData, payload, payloadSize);

            if (DeserializationError::Ok != error.code())
            {
                LOG_WARNING("JSON parse error: %s", error.c_str());
                isError = true;
            }
            else
            {
                rsp.isRsp = true;
                m_responseQueue.push_back(std::move(rsp));
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
        rsp.isRsp = false;
        rsp.jsonDocData.clear();
        m_responseQueue.push_back(std::move(rsp));
    }

    m_activeRestId             = INVALID_REST_ID;
    m_activePreProcessCallback = nullptr;
}

void RestService::handleFailedWebRequest()
{
    MutexGuard<Mutex> guard(m_mutex);
    Response          rsp(0U);

    rsp.restId = m_activeRestId;
    rsp.isRsp  = false;
    m_responseQueue.push_back(std::move(rsp));

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
    }

    restId = m_restIdCounter;
    ++m_restIdCounter;

    return restId;
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
