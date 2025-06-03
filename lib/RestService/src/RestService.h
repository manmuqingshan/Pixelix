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
 *
 * @addtogroup REST_SERVICE
 *
 * @{
 */

#ifndef REST_SERVICE_H
#define REST_SERVICE_H

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <IService.hpp>
#include <AsyncHttpClient.h>
#include <TaskProxy.hpp>
#include <Mutex.hpp>
#include <ArduinoJson.h>
#include <map>
#include <HttpStatus.h>
#include <Logging.h>
#include <vector>
#include <utility>

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

static const int memoryLocationTest = 15;

/**
 * The REST service handles outgoing REST-API calls and their responses.
 */
class RestService : public IService
{
public:

    /**
     * Get the REST service instance.
     *
     * @return REST service instance
     */
    static RestService& getInstance()
    {
        static RestService instance; /* idiom */

        return instance;
    }

    /**
     * Start the service.
     *
     * @return If successful started, it will return true otherwise false.
     */
    bool start() final;

    /**
     * Stop the service.
     */
    void stop() final;

    /**
     * Process the service.
     */
    void process() final;

    /**
     * Set a Callbacks. Only one pair of callbacks per plugin can be set at a time.
     *
     * @param[in] restId Unique Id to identify plugin
     * @param[in] rsp_cb Callback which shall be called when a successful response arrives.
     * @param[in] err_cb Callback which shall be called when an error happens.
     */
    void setCallbacks(void* restId, AsyncHttpClient::OnResponse rsp_cb, AsyncHttpClient::OnError err_cb);

    /**
     * Delete Callbacks if any exist.
     *
     * @param[in] restId Unqiue Id to identify plugin
     */
    void deleteCallbacks(void* restId);

    /**
     * Send GET request to host.
     *
     * @param[in] restId  Unique Id to identify plugin
     * @param[in] url     URL
     *
     * @return If request is successful sent, it will return true otherwise false.
     */
    bool get(void* restId, const String& url);

    /**
     * Send POST request to host.
     *
     * @param[in] restId   Unique Id to identify plugin
     * @param[in] url      URL
     * @param[in] payload  Payload, which must be kept alive until response is available!
     * @param[in] size     Payload size in byte
     *
     * @return If request is successful sent, it will return true otherwise false.
     */
    bool post(void* restId, const String& url, const uint8_t* payload = nullptr, size_t size = 0U);

    /**
     * Send POST request to host.
     * @param[in] restId   Unique Id to identify plugin
     * @param[in] url      URL
     * @param[in] payload  Payload, which must be kept alive until response is available!
     *
     * @return If request is successful sent, it will return true otherwise false.
     */
    bool post(void* restId, const String& url, const String& payload);

    /**
     * Send a Msg to the Taskproxy.
     *
     * @param[in] restId Unique Id to identify plugin
     * @param[in] isValidRsp Does Response have a payload
     * @param[in] payload Payload, which must be kept alive until response is available!
     *
     * @If a Msg is successfully sent to Taskproxy, it will return true otherwise false.
     */
    void sendToTaskProxy(void* restId, bool isValidRsp, DynamicJsonDocument* payload);

    /**
     * Give Mutex used for serialization of api-requests.
     */
    void giveMutex();

    /**
     * Get Response to a previously started request.
     *
     * @param[in]  restId      Unique Id to identify plugin
     * @param[out] isValidRsp  Does Response have a payload
     * @param[out] payload     Content of the Response
     *
     * @return If a response is available, it will return true otherwise false
     */
    bool getResponse(void* restId, bool& isValidRsp, DynamicJsonDocument* payload);

private:

    /**
     *  Max. number of commands which can be queued. Must be increased when new user of RestService is added
     */
    static const size_t CMD_QUEUE_SIZE = 9U;

    /**
     * A message for HTTP client/server handling.
     */
    struct Msg
    {
        void*                restId; /**< Used to identify plugin in RestService */
        bool                 isMsg;  /**< true: successful Response, false: request failed*/
        DynamicJsonDocument* rsp;    /**< Response, only valid if isMsg == true */

        /**
         * Constructs a message.
         */
        Msg() :
            isMsg(false),
            rsp(nullptr)
        {
        }
    };

    /**
     * Command ids are used to identify what the user requests.
     */
    enum CmdId
    {
        CMD_ID_GET = 0, /**< GET request */
        CMD_ID_POST     /**< POST request */
    };

    /**
     * A command is a combination of request and its corresponding data.
     */
    struct Cmd
    {
        CmdId  id;     /**< The command id identifies the kind of request. */
        void*  restId; /**< Used to identify plugin in RestService */
        String url;    /**< URL */

        /**
         * The union contains the event id specific parameters.
         * Note not every command id must have parameters.
         */
        union
        {
            /**
             * Data parameters, only valid for CMD_ID_POST.
             */
            struct
            {
                const uint8_t* data; /**< Command specific data. */
                size_t         size; /**< Command specific data size in byte. */
            } data;

        } u;
    };

    AsyncHttpClient        m_client;    /**< Asynchronous HTTP client. */
    Queue<Cmd>             m_cmdQueue;  /**< Command queue */
    TaskProxy<Msg, 9U, 0U> m_taskProxy; /**< Task proxy used to decouple server responses, which happen in a different task context.*/
    Mutex                  m_mutex;     /**< Used to protect against concurrent access */

    /**
     * Saves filters
     * key: restId of plugin
     * value: filter
     */
    std::map<void*, std::pair<AsyncHttpClient::OnResponse, AsyncHttpClient::OnError>> m_Callbacks;

    /**
     * Constructs the service instance.
     */
    RestService() :
        IService(),
        m_Callbacks(),
        m_client(),
        m_taskProxy(),
        m_cmdQueue(),
        m_mutex()
    {
        (void)m_cmdQueue.create(CMD_QUEUE_SIZE);
        (void)m_mutex.create();
        LOG_INFO("Adresse von memoryLocationTest: %p", (void*)&memoryLocationTest);
    }

    /**
     * Destroys the service instance.
     */
    ~RestService()
    {
        /* Never called. */
    }

    /* An instance shall not be copied. */
    RestService(const RestService& service);
    RestService& operator=(const RestService& service);
};

/******************************************************************************
 * Variables
 *****************************************************************************/

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif /* REST_SERVICE_H */

/** @} */