import argparse
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import urllib.error

import elevenlabs as app


class Response(io.BytesIO):
    headers = {'Content-Type': 'audio/mpeg', 'request-id': 'test-request', 'character-cost': '5'}


class FakeTransport:
    def __init__(self, data=b'ID3fake-audio', failure=None):
        self.data, self.failure, self.calls = data, failure, []

    def open(self, request, timeout):
        self.calls.append(request)
        if self.failure:
            raise self.failure
        return Response(self.data)


class SoundTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        subprocess.run(['git', 'init', '-q', str(self.root)], check=True)
        (self.root / '.gitignore').write_text('/.secrets/\n')
        self.args = argparse.Namespace(prompt='A quiet click', duration=1.5, loop=False,
                                       prompt_influence=0.3, output='assets/click.mp3')
        self.env = patch.dict(os.environ, {'ELEVENLABS_API_KEY': 'test-only-placeholder'})
        self.env.start()
        self.addCleanup(self.env.stop)

    def test_request_download_and_provenance(self):
        transport = FakeTransport()
        app.generate(self.root, self.args, transport)
        self.assertEqual((self.root / self.args.output).read_bytes(), transport.data)
        record = json.loads((self.root / (self.args.output + '.provenance.json')).read_text())
        self.assertEqual(record['request']['duration_seconds'], 1.5)
        self.assertEqual(record['request_id'], 'test-request')
        self.assertNotIn('test-only-placeholder', json.dumps(record))
        self.assertEqual(len(transport.calls), 1)
        self.assertEqual(transport.calls[0].get_header('Xi-api-key'), 'test-only-placeholder')

    def test_preflight_prevents_spending_and_overwrite(self):
        for field, value in [('duration', 31), ('duration', float('nan')),
                             ('prompt_influence', -1), ('prompt', ''),
                             ('output', '../escape.mp3'), ('output', '.secrets/audio.mp3')]:
            with self.subTest(field=field, value=value):
                args = argparse.Namespace(**vars(self.args))
                setattr(args, field, value)
                transport = FakeTransport()
                with self.assertRaises(ValueError):
                    app.generate(self.root, args, transport)
                self.assertFalse(transport.calls)
        path = self.root / self.args.output
        path.parent.mkdir()
        path.write_bytes(b'original')
        with self.assertRaises(ValueError):
            app.generate(self.root, self.args, FakeTransport())
        self.assertEqual(path.read_bytes(), b'original')

    def test_failures_cleanup_and_no_retry_or_secret_leak(self):
        for transport in [FakeTransport(b'{"error":"bad"}'),
                          FakeTransport(failure=TimeoutError()),
                          FakeTransport(failure=urllib.error.HTTPError(app.ENDPOINT, 401, 'secret body', {}, None))]:
            with self.assertRaises(ValueError) as caught:
                app.generate(self.root, self.args, transport)
            self.assertNotIn('secret body', str(caught.exception))
            self.assertEqual(len(transport.calls), 1)
            self.assertFalse((self.root / self.args.output).exists())
            self.assertFalse((self.root / (self.args.output + '.provenance.json')).exists())

    def test_secret_requires_ignore_and_refuses_tracked_files(self):
        app.secret_path(self.root)
        (self.root / '.gitignore').write_text('')
        with self.assertRaises(ValueError):
            app.secret_path(self.root)
        path = self.root / app.SECRET
        path.parent.mkdir()
        path.write_bytes(b'fake encrypted bytes')
        subprocess.run(['git', '-C', str(self.root), 'add', app.SECRET], check=True)
        (self.root / '.gitignore').write_text('/.secrets/\n')
        with self.assertRaises(ValueError):
            app.secret_path(self.root)

    @unittest.skipUnless(os.name == 'nt', 'DPAPI requires Windows')
    def test_real_dpapi_round_trip_and_corruption(self):
        original = b'test-only-not-a-real-api-key'
        encrypted = app.crypt(original)
        self.assertNotIn(original, encrypted)
        self.assertEqual(app.crypt(encrypted, decrypt=True), original)
        with self.assertRaises(ValueError):
            app.crypt(b'corrupted data', decrypt=True)
        (self.root / '.secrets').mkdir()
        (self.root / app.SECRET).write_bytes(encrypted)
        with patch.dict(os.environ, {'ELEVENLABS_API_KEY': ''}):
            self.assertEqual(app.get_key(self.root), original.decode())


if __name__ == '__main__':
    unittest.main()
