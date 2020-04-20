function Decoder(bytes, port) {
  
  var decoded = {};

  if (port === 6){
    decoded.mode = bytes[0] >> 6;
    decoded.temperature = (((bytes[0] & 0x3F) * 256 + bytes[1]) /100) - 55;
    decoded.flow_1 = (bytes[2] * 256 + bytes[3]) / 100;
    decoded.flow_2 = (bytes[4] * 256 + bytes[5]) / 100;
    decoded.flow_3 = (bytes[6] * 256 + bytes[7]) / 100;
    decoded.flow_4 = (bytes[8] * 256 + bytes[9]) / 100;
    decoded.flow_5 = (bytes[10] * 256 + bytes[11]) / 100;
  }else if(port === 7){
    decoded.battery = (bytes[0] * 256 + bytes[1]) / 100;
  }

  return {
    field1: decoded.mode,
    field2: decoded.temperature,
    field3: decoded.flow_1,
    field4: decoded.flow_2,
    field5: decoded.flow_3,
    field6: decoded.flow_4,
    field7: decoded.flow_5,
    field8: decoded.battery,
  };
}