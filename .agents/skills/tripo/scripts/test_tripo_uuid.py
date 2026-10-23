import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import tripo
import test_tripo as fixtures

class UuidTests(unittest.TestCase):
    def test_uuid_submission_persists_and_resume_only_queries(self):
        task='b9ffada7-af63-4724-b775-ddbf3b7ffcf0'
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder).resolve();job=root/'job.json'
            transport=fixtures.FakeTransport([json.dumps({'code':0,'data':{'task_id':task}}).encode(),
                b'{"code":0,"data":{"status":"success","output":{"model_url":"https://cdn.example/model.glb"}}}',fixtures.Tests.glb()])
            tripo.submit_job(job,{'output':'model.glb','request':{}},'/generation/image-to-model','test',transport)
            self.assertEqual(json.loads(job.read_text())['task_id'],task)
            tripo.wait_download(root,job,'test',1,transport)
            self.assertEqual([r.get_method() for r in transport.requests],['POST','GET','GET'])
            self.assertEqual(transport.requests[1].full_url,tripo.BASE+'/tasks/'+task)
            self.assertIsNone(transport.requests[2].get_header('Authorization'))
    def test_id_allowlist_rejects_paths_and_urls(self):
        for value in ['task_abc','39973309-cbe6-4895-944f-f4cd5b4ea092']:
            self.assertTrue(tripo.valid_task_id(value))
        for value in [None,'','../task_abc','https://example.com','task_abc?q=x','b9ffada7-bad']:
            self.assertFalse(tripo.valid_task_id(value))
