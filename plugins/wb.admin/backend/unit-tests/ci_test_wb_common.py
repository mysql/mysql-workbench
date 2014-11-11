from unittest import TestCase
import wb_common

class TestWbCommon(TestCase):

    def test_split_path_linux(self):
        self.assertEqual(wb_common.splitpath('hello/world'), ('hello', 'world'))
        self.assertEqual(wb_common.splitpath('hello\\world'), ('hello', 'world'))
