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
 * @file   MqttService.cpp
 * @brief  MQTT service
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "MqttService.h"

#include <Logging.h>
#include <SettingsService.h>

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

/* Initialize MQTT service variables */
const char* MqttService::KEY_MQTT_BROKER_URL     = "mqtt_broker_url";
const char* MqttService::NAME_MQTT_BROKER_URL    = "MQTT broker URL [user:password@]host[:port]";
const char* MqttService::DEFAULT_MQTT_BROKER_URL = "";

/******************************************************************************
 * Public Methods
 *****************************************************************************/

bool MqttService::start()
{
    bool             isSuccessful = true;
    SettingsService& settings     = SettingsService::getInstance();

    if (false == settings.registerSetting(&m_mqttBrokerUrlSetting))
    {
        LOG_ERROR("Couldn't register MQTT broker URL setting.");
        isSuccessful = false;
    }
    else if (false == settings.open(true))
    {
        LOG_ERROR("Couldn't open settings.");
        isSuccessful = false;
    }
    else
    {
        String   mqttBrokerUrl = m_mqttBrokerUrlSetting.getValue();
        String   url;
        uint16_t port = 0U;
        String   user;
        String   password;
        bool     useTLS = false;

        /* Determine URL, user and password. */
        parseMqttBrokerUrl(mqttBrokerUrl, url, port, user, password, useTLS);

        m_hostname = settings.getHostname().getValue();

        settings.close();

        if (false == url.isEmpty())
        {
            m_brokerConnection.setLastWillTopic(m_hostname + "/status", "online", "offline");

            if (false == m_brokerConnection.connect(m_hostname, url, port, user, password, useTLS))
            {
                LOG_ERROR("Couldn't start MQTT broker connection.");
                isSuccessful = false;
            }
        }
    }

    if (false == isSuccessful)
    {
        stop();
    }
    else
    {
        LOG_INFO("MQTT service started.");
    }

    return isSuccessful;
}

void MqttService::stop()
{
    SettingsService& settings = SettingsService::getInstance();

    settings.unregisterSetting(&m_mqttBrokerUrlSetting);

    m_brokerConnection.disconnect();

    LOG_INFO("MQTT service stopped.");
}

void MqttService::process()
{
    m_brokerConnection.process();
}

MqttTypes::State MqttService::getState() const
{
    return m_brokerConnection.getState();
}

bool MqttService::publish(const String& topic, const String& msg, bool retained)
{
    return m_brokerConnection.publish(topic, msg, retained);
}

bool MqttService::publish(const char* topic, const char* msg, bool retained)
{
    return m_brokerConnection.publish(topic, msg, retained);
}

bool MqttService::subscribe(const String& topic, MqttTypes::TopicCallback callback)
{
    return m_brokerConnection.subscribe(topic, callback);
}

bool MqttService::subscribe(const char* topic, MqttTypes::TopicCallback callback)
{
    return m_brokerConnection.subscribe(topic, callback);
}

void MqttService::unsubscribe(const String& topic)
{
    m_brokerConnection.unsubscribe(topic);
}

void MqttService::unsubscribe(const char* topic)
{
    m_brokerConnection.unsubscribe(topic);
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

void MqttService::parseMqttBrokerUrl(const String& mqttBrokerUrl, String& url, uint16_t& port, String& user, String& password, bool& useTLS)
{
    int32_t idx = mqttBrokerUrl.indexOf("://");

    /* The MQTT broker URL format:
     * [mqtt[s]://][<USER>:<PASSWORD>@]<BROKER-URL>[:<PORT>]
     */
    url         = mqttBrokerUrl;
    useTLS      = false;

    if (0 <= idx)
    {
        String protocol = url.substring(0U, idx);

        /* MQTT over TLS? */
        if (0 == protocol.compareTo("mqtts"))
        {
            useTLS = true;
        }

        /* Remove protocol from URL. */
        url.remove(0U, idx + 3);
    }

    /* User and passwort */
    idx = url.indexOf("@");

    user.clear();
    password.clear();

    if (0 <= idx)
    {
        int32_t dividerIdx = url.indexOf(":");

        /* Only user name with empty password? */
        if (0 > dividerIdx)
        {
            user = url.substring(0U, idx);
        }
        /* At least one character for a user name must exist. */
        else if (0 < dividerIdx)
        {
            user = url.substring(0U, dividerIdx);

            /* Password not empty? */
            if (idx > (dividerIdx + 1))
            {
                password = url.substring(dividerIdx + 1, idx);
            }
        }

        url.remove(0U, idx + 1);
    }

    /* Port */
    idx  = url.indexOf(":");

    port = MQTT_PORT;

    if (0 <= idx)
    {
        String  portStr = url.substring(idx + 1);
        int32_t portNum = portStr.toInt();

        if ((0 <= portNum) && (portNum <= UINT16_MAX))
        {
            port = static_cast<uint16_t>(portNum);
        }

        url.remove(idx);
    }
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
