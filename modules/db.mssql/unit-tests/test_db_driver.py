import pyodbc

import db_mssql_test_main

from workbench import db_driver

class TestPythonDBDriver(db_mssql_test_main.MssqlTestCase):

    @classmethod
    def setUpClass(cls):
        cls.conn = db_driver.connect(cls.connection, db_mssql_test_main.test_params['password'])

    def test_connection(self):
        self.assertIsInstance(self.conn, pyodbc.Connection)

