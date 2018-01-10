# Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import unittest

import db_mssql_test_main
import grt

from grt.modules import DbMssqlRE

class TestMSSQLRevEng(db_mssql_test_main.MssqlTestCase):
    @classmethod
    def setUpClass(cls):
        DbMssqlRE.connect(cls.connection, db_mssql_test_main.test_params['password'])
        version = DbMssqlRE.getServerVersion(cls.connection)
        cls.context = grt.Dict()

    def _set_catalog_schema_table(self, catalog_name, schema_name=None, table_name=None):
        catalog = grt.classes.db_mssql_Catalog()
        catalog.name = catalog_name
        schema = table = None
        if schema_name:
            schema = grt.classes.db_mssql_Schema()
            schema.name = schema_name
            schema.owner = catalog
            if table_name:
                table = grt.classes.db_mssql_Table()
                table.name = table_name
                table.owner = schema
        return catalog, schema, table

    def test_rdbms_version(self):
        version = DbMssqlRE.getServerVersion(self.connection)
        self.assertIsInstance(version, grt.classes.GrtVersion)

    def test_get_catalog_names(self):
        catalogs = DbMssqlRE.getCatalogNames(self.connection)
        self.assertIsInstance(catalogs, grt.List)
        self.assertTrue( len(catalogs) > 0 )
        self.assertIsInstance(catalogs[0], str)
        self.assertTrue('AdventureWorks' in catalogs)

    def test_get_schema_names(self):
        schemata = DbMssqlRE.getSchemaNames(self.connection, 'AdventureWorks')
        self.assertIsInstance(schemata, grt.List)
        self.assertTrue( len(schemata) > 0 )
        self.assertIsInstance(schemata[0], str)
        self.assertTrue('.' in schemata[0])
        self.assertTrue('AdventureWorks.Person' in schemata)

    def test_rev_eng_user_datatypes(self):
        catalog = grt.classes.db_mssql_Catalog()
        catalog.name = 'AdventureWorks'
        res = DbMssqlRE.reverseEngineerUserDatatypes(self.connection, catalog)
        self.assertEqual(res, 0)
        self.assertEqual(len(catalog.userDatatypes), 6)
        datatype = None
        for user_type in catalog.userDatatypes:
            if user_type.name == 'PHONE':
                datatype = user_type
                break
        self.assertIsNotNone(datatype)
        self.assertEqual(datatype.actualType.name, 'NVARCHAR')
        self.assertEqual(datatype.characterMaximumLength, 50)
        self.assertEqual(datatype.numericPrecision, 0)
        self.assertEqual(datatype.numericScale, 0)
        self.assertEqual(datatype.isNullable, 1)

    def test_rev_eng_columns(self):
        catalog, schema, table = self._set_catalog_schema_table('AdventureWorks', 'Person', 'Contact')
        res = DbMssqlRE.reverseEngineerUserDatatypes(self.connection, catalog)
        self.assertEqual(res, 0)
        res = DbMssqlRE.reverseEngineerTableColumns(self.connection, table)
        self.assertEqual(res, 0)
        column = None
        for col in table.columns:
            if col.name == 'ModifiedDate':
                column = col
                break
        self.assertIsNotNone(column)
        self.assertEqual(column.defaultValue, 'getdate()')
        self.assertEqual(column.simpleType.name.upper(), 'DATETIME')
        self.assertEqual(column.length, 8)
        column = None
        for col in table.columns:
            if col.name == 'AdditionalContactInfo':
                column = col
                break
        self.assertIsNotNone(column)
        self.assertEqual(column.simpleType.name.upper(), 'XML')
        self.assertEqual(column.length, -1)
        column = None
        for col in table.columns:
            if col.name == 'Phone':
                column = col
                break
        self.assertIsNotNone(column)
        self.assertIsNone(column.simpleType)
        self.assertIsNotNone(column.userType)
        self.assertEqual(column.userType.name, 'PHONE')

    @unittest.skip('FIXME: grt table addPrimaryKeyColumn function is not adding the column into the columns list of the PK')
    def test_rev_eng_pks(self):
        catalog, schema, table = self._set_catalog_schema_table('AdventureWorks', 'Purchasing', 'VendorContact')
        res = DbMssqlRE.reverseEngineerUserDatatypes(self.connection, catalog)
        self.assertEqual(res, 0)
        res = DbMssqlRE.reverseEngineerTableColumns(self.connection, table)
        self.assertEqual(res, 0)
        res = DbMssqlRE.reverseEngineerTablePK(self.connection, table)
        self.assertEqual(res, 0)

        pk_cols = [ column.name for column in table.columns if table.isPrimaryKeyColumn(column) ]
        self.assertEqual(set(['VendorID', 'ContactID']), set(pk_cols))

        # Check if the columns have been added to the table primary key columns list:
        pk_cols = [column.name for column in table.primaryKey.columns]
        self.assertEqual(set(['VendorID', 'ContactID']), set(pk_cols))

        # Check fail on table with empty columns:
        table1 = grt.classes.db_mssql_Table()
        table1.name = 'Vendor'
        table1.owner = schema
        res = DbMssqlRE.reverseEngineerTablePK(self.connection, table1)
        self.assertEqual(res, 1)
        pk_cols = [ column.name for column in table1.columns if table1.isPrimaryKeyColumn(column) ]
        self.assertEqual(pk_cols, [])

    def test_rev_eng_indices(self):
        catalog, schema, table = self._set_catalog_schema_table('AdventureWorks', 'HumanResources', 'EmployeeAddress')
        res = DbMssqlRE.reverseEngineerUserDatatypes(self.connection, catalog)
        self.assertEqual(res, 0)
        res = DbMssqlRE.reverseEngineerTableColumns(self.connection, table)
        self.assertEqual(res, 0)
        res = DbMssqlRE.reverseEngineerTableIndices(self.connection, table)
        self.assertEqual(res, 0)

        detected_index = 0
        for index in table.indices:
            if index.name == 'PK_EmployeeAddress_EmployeeID_AddressID':
                detected_index += 1
            elif index.name == 'AK_EmployeeAddress_rowguid':
                detected_index += 1
                self.assertEqual(index.unique, 1)
                self.assertEqual(index.isPrimary, 0)
                self.assertEqual(index.clustered, 0)
                self.assertEqual(index.indexType, 'UNIQUE')
                self.assertEqual(set(icol.referencedColumn.name for icol in index.columns), set(['rowguid']))
        self.assertEqual(detected_index, 1)

        # Check fail on table with empty columns:
        table1 = grt.classes.db_mssql_Table()
        table1.name = 'Vendor'
        table1.owner = schema
        res = DbMssqlRE.reverseEngineerTableIndices(self.connection, table1)
        self.assertEqual(res, 1)
        indices = [ index.name for index in table1.indices ]
        self.assertEqual(indices, [])

    def test_rev_eng_foreign_keys(self):
        catalog = grt.classes.db_mssql_Catalog()
        catalog.name = 'AdventureWorks'
        res = DbMssqlRE.reverseEngineerUserDatatypes(self.connection, catalog)
        self.assertEqual(res, 0)

        for schema_name in ['Person', 'Purchasing']:
            schema = grt.classes.db_mssql_Schema()
            schema.name = schema_name
            schema.owner = catalog
            res = DbMssqlRE.reverseEngineerTables(self.connection, schema)
            self.assertEqual(res, 0)
            for table in schema.tables:
                res = DbMssqlRE.reverseEngineerTableColumns(self.connection, table)
                self.assertEqual(res, 0)
            catalog.schemata.append(schema)

        test_table = None
        for table in catalog.schemata[1].tables:
            if table.name == 'VendorContact':
                test_table = table
                break

        self.assertIsNotNone(test_table)
        res = DbMssqlRE.reverseEngineerTableFKs(self.connection, test_table)
        self.assertEqual(res, 0)

        self.assertEqual(test_table.name, 'VendorContact')
        self.assertEqual(len(test_table.foreignKeys), 3)
        found_foreign_keys = 0
        for foreign_key in test_table.foreignKeys:
            if foreign_key.name == 'FK_VendorContact_Contact_ContactID':
                found_foreign_keys += 1
                self.assertEqual(set(['ContactID']), set([column.name for column in foreign_key.columns]))
                self.assertEqual(foreign_key.referencedTable.name, 'Contact')
                self.assertEqual(foreign_key.deleteRule, 'NO_ACTION')
                self.assertEqual(foreign_key.updateRule, 'NO_ACTION')
                self.assertEqual(set(['ContactID']), set([column.name for column in foreign_key.referencedColumns]))
            elif foreign_key.name == 'FK_VendorContact_ContactType_ContactTypeID':
                found_foreign_keys += 1
                self.assertEqual(set(['ContactTypeID']), set([column.name for column in foreign_key.columns]))
                self.assertEqual(foreign_key.referencedTable.name, 'ContactType')
                self.assertEqual(foreign_key.deleteRule, 'NO_ACTION')
                self.assertEqual(foreign_key.updateRule, 'NO_ACTION')
                self.assertEqual(set(['ContactTypeID']), set([column.name for column in foreign_key.referencedColumns]))
            elif foreign_key.name == 'FK_VendorContact_Vendor_VendorID':
                found_foreign_keys += 1
                self.assertEqual(set(['VendorID']), set([column.name for column in foreign_key.columns]))
                self.assertEqual(foreign_key.referencedTable.name, 'Vendor')
                self.assertEqual(foreign_key.deleteRule, 'NO_ACTION')
                self.assertEqual(foreign_key.updateRule, 'NO_ACTION')
                self.assertEqual(set(['VendorID']), set([column.name for column in foreign_key.referencedColumns]))
        self.assertEqual(found_foreign_keys, 3)

    def test_simple_datatypes(self):
        datatypes = None
        for rdbms in grt.root.wb.rdbmsMgmt.rdbms:
            if rdbms.name == 'Mssql':
                datatypes = rdbms.simpleDatatypes
                break
        #TODO: Expand this test
