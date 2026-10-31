#!/usr/bin/env python3
"""Local HTTP server for the Android emulator test (android.yml).

The emulator reaches the runner as 10.0.2.2. Serves the files of one folder
(the APK for the update test) and answers /echo for any method, recording each
request (method, path, body length, User-Agent, Content-Type) as one JSON line
in requests.jsonl, so the test can check that the app sends the Boss Rush
ranking's GET/PATCH/POST correctly.

    android-test-server.py <port> <folder to serve> <log file>
"""
import http.server
import json
import sys
from pathlib import Path

PORT, ROOT, LOG = int(sys.argv[1]), Path(sys.argv[2]).resolve(), Path(sys.argv[3])


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(ROOT), **kwargs)

    def _echo(self):
        length = int(self.headers.get('Content-Length') or 0)
        body = self.rfile.read(length) if length else b''
        record = {'method': self.command, 'path': self.path, 'bytes': len(body),
                  'user_agent': self.headers.get('User-Agent', ''),
                  'content_type': self.headers.get('Content-Type', '')}
        with LOG.open('a', encoding='utf-8') as f:
            f.write(json.dumps(record) + '\n')
        reply = json.dumps({'ok': True, 'method': self.command}).encode()
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(reply)))
        self.end_headers()
        self.wfile.write(reply)

    def do_GET(self):
        if self.path.startswith('/echo'):
            return self._echo()
        return super().do_GET()

    do_POST = do_PATCH = do_PUT = _echo


http.server.ThreadingHTTPServer(('0.0.0.0', PORT), Handler).serve_forever()
