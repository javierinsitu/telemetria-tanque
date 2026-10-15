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
assert.equal(events.filter(([kind])=>kind==='report').length,9);
assert.equal(definition.exposes.length,24);
for (const name of ['level_m','level_percent','flow_l_min','volume_l','sensor_current_ma','adc_voltage_v','stored_volume_l']) {
    const payload={unique_id:`0xacebe6fffe2c8fd0_${name}_zigbee2mqtt`};
    definition.meta.overrideHaDiscoveryPayload(payload);
    assert.equal(payload.state_class,name==='volume_l'?'total_increasing':'measurement');
}
const diagnostic={unique_id:'0xacebe6fffe2c8fd0_flow_state_zigbee2mqtt'};
definition.meta.overrideHaDiscoveryPayload(diagnostic);
assert.equal(diagnostic.state_class,undefined);
for(const [index,name] of ['level_m','level_percent','flow_l_min','volume_l','sensor_current_ma','adc_voltage_v','stored_volume_l'].entries()) {
    assert.deepEqual(definition.fromZigbee[0].convert({}, {device,endpoint:{ID:index===6?18:index+10},data:{presentValue:2.5}}),{[name]:2.5});
}
assert.deepEqual(definition.fromZigbee[0].convert({}, {device,endpoint:{ID:10},data:{presentValue:NaN}}),{level_m:null});
assert.equal(definition.fromZigbee[0].convert({}, {device,endpoint:{ID:16},data:{presentValue:7}}),undefined);
assert.deepEqual(definition.fromZigbee[0].convert({}, {device,endpoint:{ID:18},data:{presentValue:NaN}}),{stored_volume_l:null});
const legacyDevice={...device,getEndpoint(ID){return ID===18?undefined:device.getEndpoint(ID);}};
events.length=0;
await ready.configure(legacyDevice,{});
assert.equal(events.filter(([kind])=>kind==='report').length,8);
for (const height of [0,0.35,0.7,0.8,1,2,-0.1]) {
    const data=definition.fromZigbee[0].convert({}, {device:legacyDevice,endpoint:{ID:10},data:{presentValue:height}});
    assert.equal(data.level_m,height);
    assert.equal(data.stored_volume_l,Math.PI/4*Math.min(1,Math.max(0,height))*1000);
}
assert.deepEqual(definition.fromZigbee[0].convert({}, {device:legacyDevice,endpoint:{ID:10},data:{presentValue:NaN}}),{level_m:null,stored_volume_l:null});
assert.deepEqual(definition.fromZigbee[0].convert({}, {device:legacyDevice,endpoint:{ID:10},data:{presentValue:0.7}},null,null,{state:{level_state:'adc_error'}}),{level_m:0.7,stored_volume_l:null});
assert.deepEqual(definition.fromZigbee[0].convert({}, {device,endpoint:{ID:18},data:{presentValue:550,statusFlags:2}}),{stored_volume_l:null});
assert.deepEqual(definition.fromZigbee[1].convert({}, {endpoint:{ID:16},data:{presentValue:2}}),{level_state:'adc_error',stored_volume_l:null});
await definition.toZigbee[1].convertGet({},'stored_volume_l',{device});
assert.deepEqual(events.at(-1),['read',18,'genAnalogInput',['presentValue']]);
await definition.toZigbee[1].convertGet({},'stored_volume_l',{device:legacyDevice});
assert.deepEqual(events.at(-1),['read',10,'genAnalogInput',['presentValue']]);
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
console.log('OK: prepareDefinition, 9 reporting bindings, 24 exposes, endpoint 18, legacy cylinder fallback, NaN, configuration validation, reset request');
