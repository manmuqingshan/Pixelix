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

/**
 * The REST service handles outgoing REST-API calls and their responses.
 */
class RestService : public IService
{
public:

    /**
     * Prototype of a preprocessing callback for a successful response.
     */
    typedef std::function<bool(const char*, size_t, DynamicJsonDocument&)> PreProcessCallback;

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
     * Send GET request to host.
     *
     * @param[in] url                 URL
     * @param[in] preProcessCallback  PreProcessCallback which will be called by the RestService to filter the received date.
     *
     * @return If request is successful sent, it will return a valid restId otherwise it will return INVALID_REST_ID.
     */
    uint32_t get(const String& url, PreProcessCallback preProcessCallback);

    /**
     * Send POST request to host.
     *
     * @param[in] url                 URL
     * @param[in] preProcessCallback  PreProcessCallback which will be called by the RestService to filter the received date.
     * @param[in] payload             Payload, which must be kept alive until response is available!
     * @param[in] size                Payload size in byte
     *
     * @return If request is successful sent, it will return a valid restId otherwise it will return INVALID_REST_ID.
     */
    uint32_t post(const String& url, PreProcessCallback preProcessCallback, const uint8_t* payload = nullptr, size_t size = 0U);

    /**
     * Send POST request to host.
     *
     * @param[in] url                 URL
     * @param[in] payload             Payload, which must be kept alive until response is available!
     * @param[in] preProcessCallback  PreProcessCallback which will be called by the RestService to filter the received date.
     *
     * @return If request is successful sent, it will return a valid restId otherwise it will return INVALID_REST_ID.
     */
    uint32_t post(const String& url, const String& payload, PreProcessCallback preProcessCallback);

    /**
     * Get Response to a previously started request.
     *
     * @param[in]  restId      Unique Id to identify plugin
     * @param[out] isValidRsp  Does Response have a payload
     * @param[out] payload     Content of the Response. The output variable is move-assigned.
     *
     * @return If a response is available, it will return true otherwise false
     */
    bool getResponse(uint32_t restId, bool& isValidRsp, DynamicJsonDocument& payload);

    /**
     * Removes an expired Response from the RestService.
     *
     * @param[in] restId Unique Id to identify plugin
     */
    void removeExpiredResponse(uint32_t restId);

    /**
     *  Used to indicate that HTTP request could not be started.
     */
    static constexpr uint32_t INVALID_REST_ID = 0U;

private:

    /**
     * A message for HTTP client/server handling.
     */
    struct Msg
    {
        uint32_t            restId; /**< Used to identify plugin in RestService. */
        bool                isMsg;  /**< true: successful Response, false: request failed */
        DynamicJsonDocument data;   /**< Content of the Response. Only valid if isMsg == true- */

        /**
         * Constructs a message.
         */
        Msg() :
            isMsg(false),
            data(4096U)
        {
        }

        explicit Msg(size_t dataSize) :
            isMsg(false),
            data(dataSize)
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
        CmdId              id;                 /**< The command id identifies the kind of request. */
        uint32_t           restId;             /**< Used to identify plugin in RestService */
        PreProcessCallback preProcessCallback; /**< Individual callback called when response arrives */
        String             url;                /**< URL */

        /**
         * Data parameters, only valid for CMD_ID_POST.
         */
        struct
        {
            const uint8_t* data; /**< Command specific data. */
            size_t         size; /**< Command specific data size in byte. */
        } data;
    };

    /** Cmd Queue*/
    typedef std::vector<Cmd> CmdQueue;

    /** Msg Queue*/
    typedef std::vector<Msg> MsgQueue;

    AsyncHttpClient          m_client;                   /**< Asynchronous HTTP client. */
    CmdQueue                 m_cmdQueue;                 /**< Command queue */
    MsgQueue                 m_msgQueue;                 /**< Message queue */
    bool                     m_isRunning;                /**< Signals the status of the service. True means it is running, false means it is stopped. */
    uint32_t                 m_restIdCounter;            /**< Used to generate restIds. */
    bool                     m_isWaitingForResponse;     /**< Used to protect against concurrent access */
    uint32_t                 m_activeRestId;             /**< Saves the  restId of a request until the callback triggered by the corresponding response is finished. */
    PreProcessCallback       m_activePreProcessCallback; /**< Saves the callback sent by a request until it is called when the response arrives. */
    Mutex                    m_mutex;                    /**< Mutex to protect against concurrent access. */

    /**
     * Constructs the service instance.
     */
    RestService() :
        IService(),
        m_client(),
        m_cmdQueue(),
        m_msgQueue(),
        m_isRunning(false),
        m_restIdCounter(INVALID_REST_ID),
        m_isWaitingForResponse(false),
        m_activeRestId(INVALID_REST_ID),
        m_activePreProcessCallback(),
        m_mutex()
    {
        (void)m_mutex.create();
    }

    /**
     * Destroys the service instance.
     */
    ~RestService()
    {
        /* Never called. */
        m_mutex.destroy();
    }

    /* An instance shall not be copied. */
    RestService(const RestService& service);
    RestService& operator=(const RestService& service);

    /**
     * Handles asynchronous web responses from the server. Filtering is delegated to Plugin-callbacks.
     * This will be called in LwIP context! Don't modify any member here directly!
     *
     * @param[in] rsp     Web Response
     */
    void handleAsyncWebResponse(const HttpResponse& rsp);

    /**
     * Handles failed web requests.
     * This will be called in LwIP context! Don't modify any member here directly!
     */
    void handleFailedWebRequest();

    /**
     * Generates a valid restId.
     *
     * @return A valid restId.
     */
    uint32_t getRestId();
};

/******************************************************************************
 * Variables
 *****************************************************************************/

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif /* REST_SERVICE_H */

/** @} */