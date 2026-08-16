import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import rodo

class TestRodo(unittest.TestCase):
    def setUp(self):
        self.tempfile = tempfile.NamedTemporaryFile(suffix='.rd', delete=False)
        self.path = self.tempfile.name
        self.tempfile.close()

    def tearDown(self):
        if os.path.exists(self.path):
            os.unlink(self.path)

    def test_basic_set_get(self):
        dados = rodo.link(self.path)
        dados.set('nome', 'Davi')
        self.assertEqual(dados.get('nome'), 'Davi')
        dados.close()

    def test_int_float_bool(self):
        dados = rodo.link(self.path)
        dados.set('level', 87)
        dados.set('pi', 3.14159)
        dados.set('ativo', True)
        self.assertEqual(dados.get('level'), 87)
        self.assertAlmostEqual(dados.get('pi'), 3.14159)
        self.assertTrue(dados.get('ativo'))
        dados.close()

    def test_has(self):
        dados = rodo.link(self.path)
        dados.set('x', 1)
        self.assertTrue(dados.has('x'))
        self.assertFalse(dados.has('y'))
        dados.close()

    def test_delete(self):
        dados = rodo.link(self.path)
        dados.set('x', 1)
        dados.delete('x')
        self.assertFalse(dados.has('x'))
        self.assertIsNone(dados.get('x'))
        dados.close()

    def test_persistence(self):
        dados = rodo.link(self.path)
        dados.set('nome', 'Davi')
        dados.close()
        dados2 = rodo.link(self.path)
        self.assertEqual(dados2.get('nome'), 'Davi')
        # Data should still exist even though we didn't set it again
        dados2.close()

    def test_keys(self):
        dados = rodo.link(self.path)
        dados.set('a', 1)
        dados.set('b', 2)
        keys = dados.keys()
        self.assertIn('a', keys)
        self.assertIn('b', keys)
        dados.close()

    def test_all(self):
        dados = rodo.link(self.path)
        dados.set('a', 1)
        dados.set('b', 'texto')
        all_data = dados.all()
        self.assertEqual(all_data['a'], 1)
        self.assertEqual(all_data['b'], 'texto')
        dados.close()

if __name__ == '__main__':
    unittest.main()