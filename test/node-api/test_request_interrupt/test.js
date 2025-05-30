'use strict';
const common = require('../../common');
const assert = require('assert');
const { Worker, isMainThread } = require('worker_threads');

const bindingPath = require.resolve(`./build/${common.buildType}/binding`);
const binding = require(bindingPath);

let worker;

function loopUntilIrq() {
  if (isMainThread) {
    worker = new Worker(__filename);
  } else {
    binding.TriggerIrq();
  }
  const deadline = Date.now() + 5000;
  do {
  } while (!global.irq_done && (deadline - Date.now()) > 0);
  return new Error('dummy').stack;
}

const js_stack = loopUntilIrq();
const irq_stack = binding.GetStack();
assert.strictEqual(typeof(irq_stack), 'string');
assert.ok(irq_stack.includes('loopUntilIrq'));
const irq_lines = irq_stack.split('\n');
const js_lines = js_stack.split('\n');
assert.strictEqual(irq_lines.length, js_lines.length);
for (let i = 0; i < irq_lines.length; i++) {
  let irq_line = irq_lines[i];
  let js_line = js_lines[i];
  if (irq_line.includes('loopUntilIrq')) {
    const mask_pos = /test\.js:\n+:\n+\)/
    irq_line = irq_line.replace(mask_pos, '');
    js_line = js_line.replace(mask_pos, '');
  }
  assert.strictEqual(irq_line, js_line);
}

/*
// Test that workers can load addons declared using NAPI_MODULE_INIT().
new Worker(`
const { parentPort } = require('worker_threads');
const msg = require(${JSON.stringify(bindingPath)}).hello();
parentPort.postMessage(msg)`, { eval: true })
  .on('message', common.mustCall((msg) => assert.strictEqual(msg, 'world')));
*/
