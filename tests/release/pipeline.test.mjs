import {test} from 'node:test';
import assert from 'node:assert/strict';
import {createHmac} from 'node:crypto';
import {mkdtemp, rm, readFile} from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import {createPipeline, authenticate, validateRequest} from '../../scripts/release/pipeline.mjs';
const secret = 'test-only-'.repeat(8);
test('rejects expired signatures and unsafe input', () => {
  assert.equal(authenticate(secret, Buffer.from(''), '1', '00'.repeat(32)), false);
  assert.throws(() => validateRequest({id: '../../escape', bump: 'patch', mode: 'publish'}));
  assert.throws(() => validateRequest({id: 'release-123', bump: 'patch', mode: 'check', command: 'evil'}));
});
test('authenticated jobs: serialization, replay protection, persistence and publish gate', async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), 'tspr3-pipeline-'));
  let count = 0, finish;
  const server = await createPipeline({secret, directory, runner: async () => { count++; await new Promise(resolve => {finish = resolve;}); }});
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const send = async (value, valid = true) => {
    const body = JSON.stringify(value), timestamp = String(Date.now());
    const signature = createHmac('sha256', secret).update(timestamp + '.POST /release\n' + body).digest('hex');
    return fetch(`http://127.0.0.1:${server.address().port}/release`, {method: 'POST', body,
      headers: {'x-tspr3-timestamp': timestamp, 'x-tspr3-signature': valid ? signature : '00'.repeat(32)}});
  };
  try {
    const dashboard = await fetch(`http://127.0.0.1:${server.address().port}/`);
    assert.equal(dashboard.status, 200);
    assert.match(await dashboard.text(), /Servidor activo/);
    const health = await fetch(`http://127.0.0.1:${server.address().port}/health`);
    assert.deepEqual(await health.json(), {service: 'tspr3-release', status: 'ready', publishEnabled: false});
    const value = {id: 'release-123', bump: 'patch', mode: 'check'};
    assert.equal((await send(value, false)).status, 401);
    assert.equal((await send({...value, mode: 'publish'})).status, 403);
    assert.equal((await send(value)).status, 202);
    assert.equal((await send(value)).status, 200);
    assert.equal((await send({...value, id: 'release-456'})).status, 409);
    assert.equal(count, 1);
    finish();
    for (let i = 0; i < 100; i++) {
      await new Promise(resolve => setTimeout(resolve, 10));
      if (JSON.parse(await readFile(path.join(directory, 'release-123.json'))).status === 'succeeded') break;
    }
    assert.equal(JSON.parse(await readFile(path.join(directory, 'release-123.json'))).status, 'succeeded');
    for (let i = 0; i < 19; i++) assert.equal((await send({...value, id: `invalid-${String(i).padStart(3, '0')}`}, false)).status, 401);
    assert.equal((await send({...value, id: 'invalid-limit'}, false)).status, 429);
    await assert.rejects(createPipeline({secret, directory, runner: async () => {}}), {code: 'EEXIST'});
  } finally {
    await new Promise(resolve => server.close(resolve));
    // close event releases the lock handle asynchronously.
    await new Promise(resolve => setTimeout(resolve, 30));
    await rm(directory, {recursive: true});
  }
});
