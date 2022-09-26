console.log = text => {
  log.textContent += `${text}\r\n`;
};


var MEMORYBUFFER = null;
var SELECTEDDEVICE = null;
var MEMORY = new WebAssembly.Memory({
  initial: 256,
  maximum: 512
});
let OFFSET = 0;
const enumNotify = {
  CRYPTO_GUARD_IF_CONNECTED_EVT: 0,
  CRYPTO_GUARD_IF_DISCONNECTED_EVT: 1,
  CRYPTO_GUARD_IF_SEND_STATUS_EVT: 2,
  CRYPTO_GUARD_IF_RECIEVED_DATA_EVT: 3,
  CRYPTO_GUARD_IF_INVALID_EVT: 4,
}
async function usbSend() {
  try {
    console.log("this is called from c")
    // let res=0;
    // let devices=await navigator.hid.getDevices();
    // if (devices.length == 0) {
    //     console.log(`No HID devices selected. Press the "request device" button.`);
    //     return;
    // }
    // SELECTEDDEVICE=devices[0];
    // SELECTEDDEVICE.open().then(() => {
    //     // LOADEDHIDDEVICE=true;
    //     // console.log(ARRAYBYTEOFFSET);
    //     // const result = new Int32Array(
    //     //     MEMORY.buffer,
    //     //     ARRAYBYTEOFFSET,
    //     //     length);
    //         OFFSET += 5 * Int32Array.BYTES_PER_ELEMENT
    //         const array2 = new Int32Array(MEMORYBUFFER.buffer, OFFSET, 5)
    //         array2.set([6, 7, 8, 9, 10])
    //     res = SELECTEDDEVICE.sendReport(0, array2).then(() => {
    //         console.log("Sent input report " + array2);
    //     });
    // });
    // return res;
  } catch (err) {
    return err;
  }
}
async function testWasm() {
  WebAssembly.instantiateStreaming(fetch("crypto_guard_if.wasm"), {
    js: {
      mem: MEMORY
    },
    env: {
      curTime: () => Date.now(),
      emscripten_resize_heap: MEMORY.grow,
      allocateOnMemory: console.log,
      usbSend: usbSend,
      // consoleLog:console.log,
      onGetXpubResult: console.log,
      // __assert_fail:console.log,//testing
      // _embind_register_void:console.log, 
      // _embind_register_bool:console.log, 
      // _embind_register_std_string:console.log, 
      // _embind_register_std_wstring:console.log, 
      // _embind_register_emval:console.log, 
      // _embind_register_integer:console.log, 
      // _embind_register_float:console.log, 
      // _embind_register_memory_view:console.log, 
      // _embind_register_bigint:console.log, 
      // __indirect_function_table:console.log, 
      // emscripten_memcpy_big:console.log,
    }
  }).then(results => {
    alert("jjjjjjjjjjj")
    console.log("wasm success")
    exports = results.instance.exports;
    MEMORYBUFFER = results.instance.exports.memory;
    exports.crypto_guard_if_notify(enumNotify.CRYPTO_GUARD_IF_CONNECTED_EVT, null, 0);
  });
}
testWasm()