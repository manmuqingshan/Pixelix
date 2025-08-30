
# PIXELIX <!-- omit in toc -->

![PIXELIX](./images/LogoBlack.png)

## Home Assistant <!-- omit in toc -->

- [Purpose](#purpose)
- [REST API](#rest-api)
  - [Installation (REST API)](#installation-rest-api)
  - [Setting up REST command](#setting-up-rest-command)
  - [Import Blueprint](#import-blueprint)
  - [Generate an automation](#generate-an-automation)
- [MQTT](#mqtt)
  - [Installation (MQTT)](#installation-mqtt)
  - [MQTT Discovery](#mqtt-discovery)
- [Issues, Ideas And Bugs](#issues-ideas-and-bugs)
- [License](#license)
- [Contribution](#contribution)

### Purpose

To display e.g. sensor data from your [Home Assistant](https://www.home-assistant.io/) instance, there are two possibilitis. Using the REST API or MQTT.

### REST API

The following wrapper supports for now only the JustTextPlugin.

#### Installation (REST API)

If you want to add the wrapper to your Home Assistant instance you first need to add a REST command to your `configuration.yaml`.

#### Setting up REST command

Add the following lines of code to your `configuration.yaml`:

```yaml
rest_command:
  pixelix_just_text:
    url: 'http://192.168.178.10/rest/api/v1/display/uid/{{ uid }}/text?text={{ "%5C" + align + "%5C" + color + text }}'
    method: POST
```

You need to replace the IP `192.168.178.10` with your Pixelix instance IP.

#### Import Blueprint

You can use this link to add the wrapper to your Home Assistant installation.

[![Open your Home Assistant instance and show the blueprint import dialog with a specific blueprint pre-filled.](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fgithub.com%2FBlueAndi%2FPixelix%2Fblob%2Fmaster%2Fdoc%2Fhomeassistant%2Fpixelix-justtext.yaml)

As an alternative you can copy the yaml to  `/config/blueprints/automation/homeassistant/`.
You can find the [raw YAML file here](./homeassistant/pixelix-justtext.yaml "here")

#### Generate an automation

First of all you need to install a "JustText"-Plugin in a slot in your Pixelix installation. Then note down the uid of the plugin.

![Example Home Assistant Automation](./../doc/images/HomeassistantJustTextAutomation.png)

The fields should be pretty self-explanatory.
Just note that you have to enter the sensor labels in the following format:

```Markdown
- Sensor Label 1
- Sensor Label 2
- Sensor Label 3
```

See it in the example screenshot above.
**Important:** Each sensor entry **must have** an corresponding sensor label entry!

### MQTT

MQTT is not part of every Pixelix installation. It depends on the development board specific configuration or in other words, mainly on the available flash size. For development boards with less than 8 MB flash, its disabled by default, except the Ulanzi TC001. You can enable it, but need to compile it by yourself and throw other plugins out to get it fit.

#### Installation (MQTT)

If not already installed, you will need to add the MQTT integration

[![MQTT Integration](https://my.home-assistant.io/badges/config_flow_start.svg)](https://my.home-assistant.io/redirect/config_flow_start?domain=mqtt)

#### MQTT Discovery

The Home Assistant MQTT discovery is supported by several plugins and features, here are some examples:

- Display on/off
- Device restart
- Sensor information
- JustTextPlugin
- IconTextPlugin
- IconTextLampPlugin
- MultiIconPlugin

Pixelix will be shown as device with its entities. Every installed plugin will be shown as at least one entity.

[More technical details about MQTT](./MQTT.md)

## Show Data On Display

1. Install on Pixelix e.g. the JustTextPlugin, which is able to show just text.
2. In Home Assistant go to the MQTT integration, select configuration and enter the following data:
    1. Topic: ```pixelix-6F1AD6B8/display/uid/42798/text/set```, replace the 6F1AD6B8 with your Pixelix id and the  plugin UID 42798 with your one.
    2. Payload: E.g. show the state of the garage door.

        ```json
        {
            "text": "{{ states('cover.garage_door') }}"
        }
        ```

    3. Publish.

If the information is shown, we ensure that the information will be automatically updated by using an automation in Home Assistant.

```yaml
- id: garage_door_state_on_pixelix
  alias: Garage door state on Pixelix
  description: ''
  triggers:
  - trigger: state
    entity_id:
    - cover.garage_door
  conditions: []
  actions:
  - action: mqtt.publish
    metadata: {}
    data:
      evaluate_payload: false
      qos: '0'
      topic: pixelix-6F1AD6B8/display/uid/42798/text/set
      payload: '{ "text": "{{ states(''cover.garage_door'') }}" }'
  mode: single
```

## Issues, Ideas And Bugs

If you have further ideas or you found some bugs, great! Create a [issue](https://github.com/BlueAndi/Pixelix/issues) or if you are able and willing to fix it by yourself, clone the repository and create a pull request.

## License

The whole source code is published under the [MIT license](http://choosealicense.com/licenses/mit/).
Consider the different licenses of the used third party libraries too!

## Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in the work by you, shall be licensed as above, without any
additional terms or conditions.
