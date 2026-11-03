import http from 'node:http';
import {createHmac, timingSafeEqual} from 'node:crypto';
import {mkdir, readFile, writeFile, open, unlink} from 'node:fs/promises';
import {spawn} from 'node:child_process';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

export function authenticate(secret, body, timestamp, signature, now = Date.now()) {
  if (!/^\d{13}$/.test(timestamp ?? '') || Math.abs(now - Number(timestamp)) > 300_000) return false;
  if (!/^[a-f0-9]{64}$/.test(signature ?? '')) return false;
  const expected = createHmac('sha256', secret).update(timestamp + '.').update(body).digest();
  return timingSafeEqual(expected, Buffer.from(signature, 'hex'));
}
export function validateRequest(value) {
  if (!value || Object.keys(value).some(k => !['id', 'bump', 'mode'].includes(k))) throw Error('Invalid fields');
  if (!/^[a-zA-Z0-9-]{8,64}$/.test(value.id ?? '')) throw Error('Invalid id');
  if (!['patch', 'minor', 'major'].includes(value.bump)) throw Error('Invalid bump');
  if (!['check', 'publish'].includes(value.mode)) throw Error('Invalid mode');
  return value;
}
export async function createPipeline({secret, directory, runner, allowPublish = false}) {
  if (!secret || secret.length < 32) throw Error('TSPR3_WEBHOOK_SECRET must contain at least 32 characters');
  await mkdir(directory, {recursive: true});
  // A second server must never operate on the same queue. A stale lock requires operator review.
  const lock = await open(path.join(directory, 'server.lock'), 'wx');
  await lock.writeFile(String(process.pid));
  let busy = false;
  let activeTask = Promise.resolve();
  const server = http.createServer(async (req, res) => {
    const reply = (code, data) => { res.writeHead(code, {'content-type': 'application/json'}); res.end(JSON.stringify(data)); };
    try {
      if (req.method === 'GET' && req.url === '/') {
        res.writeHead(200, {'content-type': 'text/html; charset=utf-8', 'cache-control': 'no-store',
          'content-security-policy': "default-src 'none'; style-src 'unsafe-inline'; base-uri 'none'; frame-ancestors 'none'"});
        res.end(`<!doctype html><html lang="es"><meta charset="utf-8"><meta name="viewport" content="width=device-width">
<title>TSPR3 Pipeline</title><style>body{font:16px system-ui;max-width:48rem;margin:4rem auto;padding:0 1rem;color:#172033}code{background:#eef2f7;padding:.15rem .35rem;border-radius:.3rem}.ok{color:#087443;font-weight:700}</style>
<h1>TSPR3 Pipeline local</h1><p class="ok">Servidor activo</p><p>Escucha únicamente en <code>127.0.0.1</code>.</p>
<p>Publicación: <strong>${allowPublish ? 'habilitada' : 'deshabilitada'}</strong>. Las operaciones requieren HMAC y se ejecutan con <code>request.mjs</code>.</p>
<p>Estado de ejecución: <strong>${busy ? 'ocupado' : 'disponible'}</strong>.</p></html>`);
        return;
      }
      if (req.method === 'GET' && req.url === '/health') { reply(200, {service: 'tspr3-release', status: busy ? 'busy' : 'ready', publishEnabled: allowPublish}); return; }
      const chunks = []; let length = 0;
      for await (const chunk of req) { length += chunk.length; if (length > 4096) { reply(413, {error: 'Body too large'}); return; } chunks.push(chunk); }
      const body = Buffer.concat(chunks);
      // Sign method and URL as well as body, so a signature cannot be reused for another operation.
      const signed = Buffer.concat([Buffer.from(req.method + ' ' + req.url + '\n'), body]);
      if (!authenticate(secret, signed, req.headers['x-tspr3-timestamp'], req.headers['x-tspr3-signature'])) { reply(401, {error: 'Unauthorized'}); return; }
      const match = /^\/jobs\/([a-zA-Z0-9-]{8,64})$/.exec(req.url);
      if (req.method === 'GET' && match) {
        try { reply(200, JSON.parse(await readFile(path.join(directory, match[1] + '.json'), 'utf8'))); }
        catch (error) { if (error.code === 'ENOENT') reply(404, {error: 'Unknown job'}); else throw error; }
        return;
      }
      if (req.method !== 'POST' || req.url !== '/release') { reply(404, {error: 'Unknown route'}); return; }
      let job;
      try { job = validateRequest(JSON.parse(body.toString('utf8'))); } catch { reply(400, {error: 'Invalid request'}); return; }
      const jobPath = path.join(directory, job.id + '.json');
      try { const existing = JSON.parse(await readFile(jobPath, 'utf8')); reply(200, existing); return; }
      catch (error) { if (error.code !== 'ENOENT') throw error; }
      if (job.mode === 'publish' && !allowPublish) { reply(403, {error: 'Publication is disabled'}); return; }
      if (busy) { reply(409, {error: 'Another release is running'}); return; }
      busy = true;
      job = {...job, status: 'running', started: new Date().toISOString()};
      try { await writeFile(jobPath, JSON.stringify(job), {flag: 'wx'}); }
      catch (error) { busy = false; throw error; }
      reply(202, job);
      activeTask = (async () => {
        try { await runner(job); job.status = 'succeeded'; }
        catch { job.status = 'failed'; job.error = 'Inspect the local job log; do not blindly retry a publication.'; }
        finally {
          job.finished = new Date().toISOString();
          try { await writeFile(jobPath, JSON.stringify(job)); } finally { busy = false; }
        }
      })().catch(() => { console.error('Unable to persist job status; stop the server and inspect the state directory.'); });
    } catch { if (!res.headersSent) reply(500, {error: 'Local pipeline error'}); }
  });
  server.requestTimeout = 10_000;
  server.headersTimeout = 10_000;
  server.on('close', () => {
    void activeTask.finally(async () => { await lock.close(); await unlink(path.join(directory, 'server.lock')); });
  });
  return server;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const directory = path.resolve(process.env.TSPR3_PIPELINE_HOME ?? path.join(process.env.LOCALAPPDATA, 'TSPR3', 'pipeline'));
  const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
  const runner = async job => {
    const log = await open(path.join(directory, job.id + '.log'), 'a');
    const secrets = ['CENTRAL_TOKEN_USERNAME', 'CENTRAL_TOKEN_PASSWORD', 'MAVEN_GPG_PASSPHRASE'];
    if (job.mode === 'publish' && secrets.some(name => !process.env[name])) { await log.close(); throw Error('Missing publication secrets'); }
    try {
      await new Promise((resolve, reject) => {
        const child = spawn('powershell.exe', ['-NoProfile', '-NonInteractive', '-File', path.join(root, 'scripts/release/run.ps1'),
          '-JobId', job.id, '-Bump', job.bump, '-Mode', job.mode, '-StateDirectory', directory],
          {cwd: root, windowsHide: true, stdio: ['ignore', log.fd, log.fd]});
        child.on('error', reject);
        child.on('exit', code => code === 0 ? resolve() : reject(Error('Release failed')));
      });
    } finally { await log.close(); }
  };
  const server = await createPipeline({secret: process.env.TSPR3_WEBHOOK_SECRET, directory, runner,
    allowPublish: process.env.TSPR3_ALLOW_PUBLISH === '1'});
  const port = Number(process.env.TSPR3_WEBHOOK_PORT ?? 8787);
  for (const signal of ['SIGINT', 'SIGTERM']) process.on(signal, () => server.close());
  server.listen(port, '127.0.0.1', () => console.log(`TSPR3 webhook: http://127.0.0.1:${port}; state: ${directory}`));
}
