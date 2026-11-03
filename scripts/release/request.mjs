import {createHmac, randomUUID} from 'node:crypto';
const secret = process.env.TSPR3_WEBHOOK_SECRET;
if (!secret || secret.length < 32) throw Error('Set TSPR3_WEBHOOK_SECRET in this process');
const [operation = 'check', argument = 'patch'] = process.argv.slice(2);
const status = operation === 'status';
if (!status && !['check', 'publish'].includes(operation)) throw Error('Use check|publish [patch|minor|major], or status JOB_ID');
if (status && !/^[a-zA-Z0-9-]{8,64}$/.test(argument)) throw Error('Invalid job id');
const method = status ? 'GET' : 'POST';
const route = status ? '/jobs/' + argument : '/release';
const body = status ? '' : JSON.stringify({id: randomUUID(), bump: argument, mode: operation});
const timestamp = String(Date.now());
const signature = createHmac('sha256', secret).update(timestamp + '.' + method + ' ' + route + '\n' + body).digest('hex');
const response = await fetch(`http://127.0.0.1:${process.env.TSPR3_WEBHOOK_PORT ?? 8787}${route}`, {
  method, headers: {'content-type': 'application/json', 'x-tspr3-timestamp': timestamp, 'x-tspr3-signature': signature},
  ...(status ? {} : {body})
});
console.log(await response.text());
if (!response.ok) process.exitCode = 1;
