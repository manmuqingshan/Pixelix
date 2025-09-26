
# PIXELIX <!-- omit in toc -->

![PIXELIX](./images/LogoBlack.png)

## Home Assistant <!-- omit in toc -->

- [Purpose](#purpose)
- [Communication](#communication)
  - [REST API](#rest-api)
    - [Installation (REST Command)](#installation-rest-command)
    - [Automation (REST Command)](#automation-rest-command)
  - [MQTT API](#mqtt-api)
    - [Installation (MQTT)](#installation-mqtt)
    - [MQTT Discovery](#mqtt-discovery)
    - [Automation (MQTT)](#automation-mqtt)
- [Automation Blueprint](#automation-blueprint)
- [Issues, Ideas And Bugs](#issues-ideas-and-bugs)
- [License](#license)
- [Contribution](#contribution)

## Purpose

[Home Assistant](https://www.home-assistant.io/) is a popular open-source platform for smart home automation. Pixelix supports displaying sensor data from Home Assistant using both the REST API and the MQTT API. Especially the MQTT automatic discovery support is fast and seamless.

## Communication

Every Pixelix device provides a REST API and some may provide the MQTT API. To see whether MQTT is supported by your Pixelix device, open the *Settings* web page and search
for *MQTT broker URL*. If its there, MQTT is supported.

### REST API

With the [RESTful Command integration](https://www.home-assistant.io/integrations/rest_command) PIXELIX can be controlled by REST API and with the [RESTful integration](https://www.home-assistant.io/integrations/rest) sensor entities can be created.

#### Installation (REST Command)

Setup corresponding entities in the Home Assistant `configuration.yaml` and use them via the automation wizard or manually in the `automations.yaml`.

A simple REST command example looks like:

```yaml
rest_command:
  pixelix_notify:
    url: 'http://<IP-ADDRESS>/rest/api/v1/display/uid/{{ uid }}/iconText?text={{ text | urlencode() }}'
    method: POST
```

The REST API is desribed in detail on the [SwaggerHub](https://app.swaggerhub.com/apis/BlueAndi/Pixelix/1.7.0).

#### Automation (REST Command)

Extend the `automations.yaml` manually for using the Pixelix MQTT device, e.g.

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
    - service: rest_command.pixelix_notify
      data:
        uid: 42798
        text: "{{ states('cover.garage_door') }}"
  mode: single
```

[Home Assistant automation blueprints](https://www.home-assistant.io/docs/automation/using_blueprints/) are a game changer in sense of simple integration, without the necessity to battle in low level yaml and jinja2 programming. See [Automation Blueprint chapter](#automation-blueprint) how to import it for Pixelix.

### MQTT API

Using the MQTT API requires that a MQTT broker is available in network and Pixelix as well as Home Assistant have access to it.

#### Installation (MQTT)

If not already installed, you will need first to install the MQTT integration to your Home Assistant instance.

[![MQTT Integration](https://my.home-assistant.io/badges/config_flow_start.svg)](https://my.home-assistant.io/redirect/config_flow_start?domain=mqtt)

Then configure Pixelix in the *Settings* web page like:

1. Enter the MQTT broker URL to *MQTT broker URL*.
2. If necessary, update the *Home Assistant Discovery Prefix*.
3. Activate the checkbox *Enable Home Assistant Discovery*.
4. Restart PIXELIX.

#### MQTT Discovery

The Home Assistant MQTT discovery is supported by several plugins and features, here are some examples:

- Display on/off
- Device restart
- Sensor information
- IconTextPlugin
- IconTextLampPlugin
- MultiIconPlugin

Pixelix will be shown as device with its entities. Every installed plugin will be shown as at least one entity.

[More technical details about MQTT](./MQTT.md)

#### Automation (MQTT)

Extend the `automations.yaml` manually for using the Pixelix MQTT device, e.g.

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
      topic: pixelix-6F1AD6B8/display/uid/42798/iconText/set
      payload: '{ "text": "{{ states(''cover.garage_door'') }}" }'
  mode: single
```

[Home Assistant automation blueprints](https://www.home-assistant.io/docs/automation/using_blueprints/) are a game changer in sense of simple integration, without the necessity to battle in low level yaml and jinja2 programming. See [Automation Blueprint chapter](#automation-blueprint) how to import it for Pixelix.

## Automation Blueprint

Home Assistant Automation Blueprints are reusable templates that simplify the creation of automations by providing a guided, fill-in-the-blanks interface. They help users quickly set up complex automations without needing to write YAML code from scratch, making smart home customization more accessible and efficient.

Pixelix provides with the following link such an automation blueprint to your Home Assistant installation. It supports to communicate with a Pixelix device by REST API and via MQTT API.

[![Open your Home Assistant instance and show the blueprint import dialog with a specific blueprint pre-filled.](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fgithub.com%2FBlueAndi%2FPixelix%2Fblob%2Fmaster%2Fdoc%2Fhomeassistant%2Fblueprint%2Fpixelix_send_sensor_data.yaml)

As an alternative copy the [pixelix_send_sensor_data.yaml](./homeassistant/blueprint/pixelix_send_sensor_data.yaml) file to  `/config/blueprints/automation/homeassistant/`.

Only for communicate via REST API with Pixelix, the following REST command needs to be added to the `configuration.yaml`:

```yaml
rest_command:
  pixelix_plugin_command:
    url: "http://{{ hostname }}{{ endpoint }}?{{ url_parameter }}"
    method: POST
```

## Issues, Ideas And Bugs

If you have further ideas or you found some bugs, great! Create a [issue](https://github.com/BlueAndi/Pixelix/issues) or if you are able and willing to fix it by yourself, clone the repository and create a pull request.

## License

The whole source code is published under the [MIT license](http://choosealicense.com/licenses/mit/).
Consider the different licenses of the used third party libraries too!

## Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in the work by you, shall be licensed as above, without any
additional terms or conditions.
