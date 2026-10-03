import assert from 'node:assert/strict';
import {prepareDefinition} from 'zigbee-herdsman-converters';
import definition from './telemetria_tanque.mjs';
const ready=prepareDefinition(definition);
const events=[];
let registered=false;
const device={
    addCustomCluster(name,cluster){assert.equal(name,'hydraulicConfig'); assert.equal(cluster.ID,0xfc00); registered=true;},
    getEndpoint(ID){return {ID,
        async bind(cluster){events.push(['bind',ID,cluster]);},
        async configureReporting(cluster,records){assert.equal(records[0].minimumReportInterval,30);assert.equal(records[0].maximumReportInterval,600);events.push(['report',ID,cluster]);},
        async read(cluster,attributes){if(cluster==='hydraulicConfig') assert(registered);events.push(['read',ID,cluster,attributes]);},
        async write(cluster,data){assert(registered);events.push(['write',ID,cluster,data]);},
    };},
};
await ready.configure(device,{});
assert.equal(events.filter(([kind])=>kind==='report').length,8);
assert.equal(definition.exposes.length,23);
for (const name of ['level_m','level_percent','flow_l_min','volume_l','sensor_current_ma','adc_voltage_v']) {
    const payload={unique_id:`0xacebe6fffe2c8fd0_${name}_zigbee2mqtt`};
    definition.meta.overrideHaDiscoveryPayload(payload);
    assert.equal(payload.state_class,name==='volume_l'?'total_increasing':'measurement');
}
const diagnostic={unique_id:'0xacebe6fffe2c8fd0_flow_state_zigbee2mqtt'};
definition.meta.overrideHaDiscoveryPayload(diagnostic);
assert.equal(diagnostic.state_class,undefined);
for(const [index,name] of ['level_m','level_percent','flow_l_min','volume_l','sensor_current_ma','adc_voltage_v'].entries()) {
    assert.deepEqual(definition.fromZigbee[0].convert({}, {endpoint:{ID:index+10},data:{presentValue:2.5}}),{[name]:2.5});
}
assert.deepEqual(definition.fromZigbee[0].convert({}, {endpoint:{ID:10},data:{presentValue:NaN}}),{level_m:null});
assert.equal(definition.fromZigbee[0].convert({}, {endpoint:{ID:1},data:{presentValue:7}}),undefined);
assert.deepEqual(definition.fromZigbee[1].convert({}, {endpoint:{ID:17},data:{presentValue:5}}),{flow_state:'disabled'});
await definition.toZigbee[0].convertSet({},'flow_slope',8.1,{device});
assert.equal(events.at(-2)[0],'write'); assert.equal(events.at(-1)[0],'read');
await assert.rejects(()=>definition.toZigbee[0].convertSet({},'flow_slope',0,{device}));
await assert.rejects(()=>definition.toZigbee[0].convertSet({},'publication_s',29,{device}));
await definition.toZigbee[0].convertSet({},'publication_s',30,{device});
await assert.rejects(()=>definition.toZigbee[0].convertSet({},'flow_enabled',0.5,{device}));
await assert.rejects(()=>definition.toZigbee[0].convertSet({},'reset_volume','ON',{device}));
await definition.toZigbee[0].convertSet({},'reset_volume','RESET',{device});
assert.deepEqual(events.at(-1),['write',1,'hydraulicConfig',{resetVolume:1}]);
console.log('OK: prepareDefinition, custom cluster registration order, 8 reporting bindings, 23 exposes, endpoint decoding, NaN, configuration validation, reset request');
