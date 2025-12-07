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
 * @file   MqttService.h
 * @brief  MQTT service
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @addtogroup MQTT_SERVICE
 *
 * @{
 */

#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <IService.hpp>
#include <KeyValueString.h>
#include "MqttTypes.h"
#include "MqttBrokerConnection.h"

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
 * The MQTT service provides access via MQTT.
 */
class MqttService : public IService
{
public:

    /**
     * Get the MQTT service instance.
     *
     * @return MQTT service instance
     */
    static MqttService& getInstance()
    {
        static MqttService instance; /* idiom */

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
     * Get current MQTT connection state.
     *
     * @return MQTT connection state
     */
    MqttTypes::State getState() const;

    /**
     * Publish a message for a topic.
     *
     * @param[in] topic     Message topic
     * @param[in] msg       Message itself
     * @param[in] retained  Retained message? Default is false.
     *
     * @return If successful published, it will return true otherwise false.
     */
    bool publish(const String& topic, const String& msg, bool retained = false);

    /**
     * Publish a message for a topic.
     *
     * @param[in] topic     Message topic
     * @param[in] msg       Message itself
     * @param[in] retained  Retained message? Default is false.
     *
     * @return If successful published, it will return true otherwise false.
     */
    bool publish(const char* topic, const char* msg, bool retained = false);

    /**
     * Subscribe for a topic. The callback will be called every time a message
     * is received for the topic.
     *
     * @param[in] topic     The topic which to subscribe for.
     * @param[in] callback  The callback which to call for any received topic message.
     *
     * @return If successful subscribed, it will return true otherwise false.
     */
    bool subscribe(const String& topic, MqttTypes::TopicCallback callback);

    /**
     * Subscribe for a topic. The callback will be called every time a message
     * is received for the topic.
     *
     * @param[in] topic     The topic which to subscribe for.
     * @param[in] callback  The callback which to call for any received topic message.
     *
     * @return If successful subscribed, it will return true otherwise false.
     */
    bool subscribe(const char* topic, MqttTypes::TopicCallback callback);

    /**
     * Unsubscribe topic.
     *
     * @param[in] topic The topic which to unsubscribe.
     */
    void unsubscribe(const String& topic);

    /**
     * Unsubscribe topic.
     *
     * @param[in] topic The topic which to unsubscribe.
     */
    void unsubscribe(const char* topic);

private:

    /** MQTT port */
    static const uint16_t MQTT_PORT = 1883U;

    /** MQTT broker URL key */
    static const char* KEY_MQTT_BROKER_URL;

    /** MQTT broker URL name */
    static const char* NAME_MQTT_BROKER_URL;

    /** MQTT broker URL default value */
    static const char* DEFAULT_MQTT_BROKER_URL;

    /** MQTT broker URL min. length */
    static const size_t MIN_VALUE_MQTT_BROKER_URL  = 0U;

    /** MQTT broker URL max. length */
    static const size_t  MAX_VALUE_MQTT_BROKER_URL = 64U;

    KeyValueString       m_mqttBrokerUrlSetting; /**< URL of the MQTT broker setting */
    MqttBrokerConnection m_brokerConnection;     /**< MQTT broker connection */
    String               m_hostname;             /**< MQTT hostname */

    /**
     * Constructs the service instance.
     */
    MqttService() :
        IService(),
        m_mqttBrokerUrlSetting(KEY_MQTT_BROKER_URL, NAME_MQTT_BROKER_URL, DEFAULT_MQTT_BROKER_URL, MIN_VALUE_MQTT_BROKER_URL, MAX_VALUE_MQTT_BROKER_URL),
        m_brokerConnection(),
        m_hostname()
    {
    }

    /**
     * Destroys the service instance.
     */
    ~MqttService()
    {
        /* Never called. */
    }

    /* An instance shall not be copied. */
    MqttService(const MqttService& service);
    MqttService& operator=(const MqttService& service);

    /**
     * Parse MQTT broker URL and derive the raw URL, the user and password.
     *
     * @param[in] mqttBrokerUrl The MQTT broker URL.
     * @param[out] url          The raw MQTT broker URL.
     * @param[out] port         The MQTT broker port.
     * @param[out] user         The MQTT broker user.
     * @param[out] password     The MQTT broker password.
     * @param[out] useTLS       Use TLS connection?
     */
    void parseMqttBrokerUrl(const String& mqttBrokerUrl, String& url, uint16_t& port, String& user, String& password, bool& useTLS);
};

/******************************************************************************
 * Variables
 *****************************************************************************/

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif /* MQTT_SERVICE_H */

/** @} */