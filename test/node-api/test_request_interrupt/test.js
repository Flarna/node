'use strict';
const common = require('../../common');
const assert = require('assert');
const { Worker, isMainThread, parentPort } = require('worker_threads');

const bindingPath = require.resolve(`./build/${common.buildType}/binding`);
const binding = require(bindingPath);

function loopUntilIrq() {
  const deadline = Date.now() + 5000;
  if (!isMainThread) {
    binding.TriggerIrqs();
  }
  do {
  } while (!global.irq_done && (deadline - Date.now()) > 0);
  assert.ok(global.irq_done);
  common.allowGlobals(global.irq_done);
  return new Error('dummy').stack;
}

function assertStacks(js_stacks) {
  const irq_stacks = binding.GetStacks();
  assert.ok(Array.isArray(irq_stacks));
  assert.strictEqual(js_stacks.length, irq_stacks.length);
  for (let i = 0; i < irq_stacks.length; i++) {
    const irq_stack = irq_stacks[i];
    const js_stack = js_stacks[i];
    assert.ok(typeof(irq_stack), 'string');
    assert.ok(irq_stack.includes('loopUntilIrq'));
    const irq_lines = irq_stack.split('\n');
    const js_lines = js_stack.split('\n');
    assert.strictEqual(irq_lines.length, js_lines.length);
    for (let j = 0; j < irq_lines.length; j++) {
      let irq_line = irq_lines[j];
      let js_line = js_lines[j];
      if (irq_line.includes('loopUntilIrq')) {
        const mask_pos = /:\d+:\d+/;
        irq_line = irq_line.replace(mask_pos, '');
        js_line = js_line.replace(mask_pos, '');
      }
      assert.strictEqual(irq_line, js_line);
    }
  }
}

if (isMainThread) {
  const worker = new Worker(__filename);
  const js_stacks = [];
  js_stacks[0] = loopUntilIrq();
  worker.on('message', common.mustCall((stack) => {
    js_stacks[1] = stack;
    assertStacks(js_stacks);
  }));
} else {
  const worker_stack = loopUntilIrq();
  parentPort.postMessage(worker_stack);
}
