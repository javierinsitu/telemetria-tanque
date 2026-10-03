import {presets as e, access as ea} from 'zigbee-herdsman-converters/lib/exposes';
import {deviceAddCustomCluster} from 'zigbee-herdsman-converters/lib/modernExtend';

const MIN_REPORT_SECONDS = 30;
const HEARTBEAT_SECONDS = 600;
const parameters = [
    ['acquisition_ms', 0, 100, 10000, 'ms'],
    ['publication_s', 1, MIN_REPORT_SECONDS, HEARTBEAT_SECONDS, 's'],
    ['shunt_ohm', 2, 100, 200, 'Ω'],
    ['current_min_ma', 3, 2, 6, 'mA'],
    ['current_max_ma', 4, 15, 22, 'mA'],
    ['height_span_m', 5, 0.01, 10, 'm'],
    ['height_min_m', 6, 0, 10, 'm'],
    ['height_max_m', 7, 0.01, 10, 'm'],
    ['level_filter_alpha', 8, 0.001, 1, undefined],
    ['flow_slope', 9, 0.1, 100, undefined],
    ['flow_intercept', 10, -100, 100, 'Hz'],
    ['flow_threshold', 11, 0, 30, 'L/min'],
    ['flow_filter_alpha', 12, 0.001, 1, undefined],
    ['flow_enabled', 13, 0, 1, undefined],
];
const measurements = [
    ['level_m', 'm', 0.01], ['level_percent', '%', 1],
    ['flow_l_min', 'L/min', 0.2], ['volume_l', 'L', 1],
    ['sensor_current_ma', 'mA', 0.1], ['adc_voltage_v', 'V', 0.015],
];
const levelStates = ['ok', 'adc_error', 'under_range', 'over_range'];
const flowStates = ['no_flow', 'flowing', 'pcnt_error', 'out_of_range', 'disabled'];
const cluster = {
    name: 'hydraulicConfig', ID: 0xfc00,
    attributes: Object.fromEntries([
        ...parameters.map(([name, ID]) => [name, {name, ID, type: 0x39, write: true}]),
        ['resetVolume', {name: 'resetVolume', ID: 0x100, type: 0x10, write: true}],
        ['clusterRevision', {name: 'clusterRevision', ID: 0xfffd, type: 0x21}],
    ]),
    commands: {}, commandsResponse: {},
};
const analog = {
    cluster: 'genAnalogInput', type: ['attributeReport', 'readResponse'],
    convert: (model, msg) => {
        const index = msg.endpoint.ID - 10;
        if (index < 0 || index >= measurements.length) return;
        if (!('presentValue' in msg.data)) return;
        const value = msg.data.presentValue;
        return {[measurements[index][0]]: Number.isFinite(value) ? value : null};
    },
};
const states = {
    cluster: 'genMultistateInput', type: ['attributeReport', 'readResponse'],
    convert: (model, msg) => {
        if (!('presentValue' in msg.data)) return;
        const list = msg.endpoint.ID === 16 ? levelStates : msg.endpoint.ID === 17 ? flowStates : undefined;
        if (!list) return;
        return {[msg.endpoint.ID === 16 ? 'level_state' : 'flow_state']: list[msg.data.presentValue - 1] ?? 'unknown'};
    },
};
const configFrom = {
    cluster: 'hydraulicConfig', type: ['attributeReport', 'readResponse'],
    convert: (model, msg) => Object.fromEntries(parameters.filter(([name]) => name in msg.data)
        .map(([name]) => [name, msg.data[name]])),
};
const configTo = {
    key: [...parameters.map(([name]) => name), 'reset_volume'],
    convertSet: async (entity, key, value, meta) => {
        const ep = meta.device.getEndpoint(1);
        if (key === 'reset_volume') {
            if (value !== 'RESET') throw new Error('Use RESET para solicitar el reinicio del volumen');
            await ep.write('hydraulicConfig', {resetVolume: 1});
            // Firmware persists the reset outside the stack callback. This is
            // an accepted request; the volume report confirms completion.
            return;
        }
        const parameter = parameters.find(([name]) => name === key);
        if (!parameter || typeof value !== 'number' || !Number.isFinite(value) ||
            value < parameter[2] || value > parameter[3]) throw new Error(`Valor inválido: ${key}`);
        if (['acquisition_ms', 'publication_s', 'flow_enabled'].includes(key) && !Number.isInteger(value)) {
            throw new Error(`${key} debe ser entero`);
        }
        await ep.write('hydraulicConfig', {[key]: value});
        await ep.read('hydraulicConfig', [key]);
        return; // Confirm state by readResponse, not an optimistic echo.
    },
    convertGet: async (entity, key, meta) => {
        if (key !== 'reset_volume') await meta.device.getEndpoint(1).read('hydraulicConfig', [key]);
    },
};
const measurementsTo = {
    key: [...measurements.map(([name]) => name), 'level_state', 'flow_state'],
    convertGet: async (entity, key, meta) => {
        const index = measurements.findIndex(([name]) => name === key);
        await meta.device.getEndpoint(index >= 0 ? index + 10 : key === 'level_state' ? 16 : 17)
            .read(index >= 0 ? 'genAnalogInput' : 'genMultistateInput', ['presentValue']);
    },
};

export default {
    fingerprint: [{modelID: 'ESP32C6_HYDRAULIC_1', manufacturerName: 'MILANGAS'}],
    model: 'ESP32C6_HYDRAULIC_1', vendor: 'MILANGAS',
    description: 'Router ESP32-C6: nivel HY-5000/ADS1115 y caudal YF-B6',
    meta: {
        // Z2M 2.14.x does not accept state_class in expose.homeassistant.
        // Apply statistical semantics to the generated MQTT discovery payload.
        overrideHaDiscoveryPayload: (payload) => {
            const measurement = measurements.find(([name]) =>
                payload.unique_id?.endsWith(`_${name}_zigbee2mqtt`));
            if (measurement) payload.state_class = measurement[0] === 'volume_l' ? 'total_increasing' : 'measurement';
        },
    },
    extend: [deviceAddCustomCluster('hydraulicConfig', cluster)],
    fromZigbee: [analog, states, configFrom], toZigbee: [configTo, measurementsTo],
    exposes: [
        ...measurements.map(([name, unit]) => e.numeric(name, ea.STATE_GET).withUnit(unit)
            .withDescription(name.replaceAll('_', ' '))),
        e.enum('level_state', ea.STATE_GET, [...levelStates, 'unknown']).withCategory('diagnostic'),
        e.enum('flow_state', ea.STATE_GET, [...flowStates, 'unknown']).withCategory('diagnostic'),
        ...parameters.map(([name, id, min, max, unit]) => {
            const expose = e.numeric(name, ea.ALL).withValueMin(min).withValueMax(max)
                .withValueStep(['acquisition_ms', 'publication_s', 'flow_enabled'].includes(name) ? 1 :
                    name.endsWith('alpha') ? 0.001 : 0.01)
                .withCategory('config').withDescription(`Calibración/configuración: ${name}`);
            return unit ? expose.withUnit(unit) : expose;
        }),
        e.enum('reset_volume', ea.SET, ['RESET']).withCategory('config')
            .withDescription('Reinicio voluntario persistente del volumen acumulado'),
    ],
    configure: async (device, coordinatorEndpoint) => {
        // Definition.configure runs before extend.configure in ZHC 26.x.
        device.addCustomCluster('hydraulicConfig', cluster);
        for (let i = 0; i < measurements.length; ++i) {
            const ep = device.getEndpoint(i + 10);
            await ep.bind('genAnalogInput', coordinatorEndpoint);
            await ep.configureReporting('genAnalogInput', [{attribute: 'presentValue',
                minimumReportInterval: MIN_REPORT_SECONDS, maximumReportInterval: HEARTBEAT_SECONDS,
                reportableChange: measurements[i][2]}]);
            await ep.read('genAnalogInput', ['presentValue', 'engineeringUnits', 'statusFlags']);
        }
        for (const id of [16, 17]) {
            const ep = device.getEndpoint(id);
            await ep.bind('genMultistateInput', coordinatorEndpoint);
            await ep.configureReporting('genMultistateInput', [{attribute: {ID: 0x55, type: 0x21},
                minimumReportInterval: MIN_REPORT_SECONDS, maximumReportInterval: HEARTBEAT_SECONDS, reportableChange: 1}]);
            await ep.read('genMultistateInput', ['presentValue']);
        }
        const ep = device.getEndpoint(1);
        // Small requests avoid oversized APS frames.
        for (let i = 0; i < parameters.length; i += 4) {
            await ep.read('hydraulicConfig', parameters.slice(i, i + 4).map(([name]) => name));
        }
    },
};
