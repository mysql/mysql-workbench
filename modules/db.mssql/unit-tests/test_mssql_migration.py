# Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
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

from grt.modules import DbMssqlRE, DbMssqlMigration

class TestMSSQLMigration(db_mssql_test_main.MssqlTestCase):
    @classmethod
    def setUpClass(cls):
        DbMssqlRE.connect(cls.connection, db_mssql_test_main.test_params['password'])
        self.state = db_migration_Migration()
        #version = DbMssqlRE.getServerVersion(cls.connection)
        #cls.context = grt.Dict()
        #cls.context['serverVersion'] = version
    
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

    def test_migrate_catalog(self):
        source_catalog = grt.classes.db_mssql_Catalog()
        source_catalog.name = '[My Test Name]'
        target_catalog = DbMssqlMigration.migrateCatalog(self.state, source_catalog)
        self.assertTrue(target_catalog != None)
        version = (target_catalog.version.majorNumber, target_catalog.version.minorNumber,
                   target_catalog.version.releaseNumber, target_catalog.version.buildNumber)
        self.assertEqual(version, (5, 5, 0, 0))
        self.assertEqual(target_catalog.name, 'My Test Name')

    def test_migrate_identifier(self):
        self.assertEqual('simple_identifier_01', DbMssqlMigration.migrateIdentifier('"simple_identifier_01"', None))
        self.assertEqual('simple identifier 02', DbMssqlMigration.migrateIdentifier('[simple identifier 02]', None))
        #l = grt.modules.Migration.get_log_size()
        DbMssqlMigration.migrateIdentifier('[]', None)
        #self.assertEqual(l+1, grt.modules.Migration.get_log_size())

    def test_migrate_schema(self):
        source_schema = grt.classes.db_mssql_Schema()
        source_schema.name = '[My Test Name]'
        target_catalog = grt.classes.db_mysql_Catalog()
        target_schema = DbMssqlMigration.migrateSchema(self.state, source_schema, target_catalog)
        self.assertTrue(target_schema != None)
        self.assertEqual(target_schema.name, 'My Test Name')

    def test_migrate_table(self):
        source_table = grt.classes.db_mssql_Table()
        source_table.name = '[My Test Name]'
        target_schema = grt.classes.db_mysql_Schema()
        target_table = DbMssqlMigration.migrateTableToMySQL(self.state, source_table, target_schema)
        self.assertTrue(target_table != None)
        self.assertEqual(target_table.name, 'My Test Name')

    def test_migrate_table_columns(self):
        # Prepare source objects:
        source_catalog, source_schema, source_table = self._set_catalog_schema_table('AdventureWorks', 'Person', 'Contact')
        res = DbMssqlRE.reverseEngineerUserDatatypes(self.state, self.connection, source_catalog)
        self.assertEqual(res, 0)
        res = DbMssqlRE.reverseEngineerTableColumns(self.state, self.connection, source_table)
        self.assertEqual(res, 0)

        # Do migration:
        target_catalog = DbMssqlMigration.migrateCatalog(self.state, source_catalog)
        self.assertTrue(target_catalog != None)
        target_table = grt.classes.db_mysql_Schema()
        res = DbMssqlMigration.migrateTableToMySQL(self.state, source_table, target_schema)
        self.assertEqual(res, 0)
        self.assertEqual(target_table.name, 'Contact')
        res = DbMssqlMigration.migrateTableColumnsToMySQL(source_table, target_table)
        self.assertEqual(res, 0)
        self.assertEqual(len(target_table.columns), len(source_table.columns))
        # TODO: Extend this test


    @unittest.skip('FIXME: grt table addPrimaryKeyColumn function is not adding the column into the columns list of the PK')
    def test_migrate_table_pks(self):
        # Prepare source objects:
        source_catalog, source_schema, source_table = self._set_catalog_schema_table('AdventureWorks', 'Purchasing', 'VendorContact')
        res = DbMssqlRE.reverseEngineerUserDatatypes(self.connection, source_catalog)
        self.assertEqual(res, 0)
        res = DbMssqlRE.reverseEngineerTableColumns(self.connection, source_table, grt.Dict())
        self.assertEqual(res, 0)
        res = DbMssqlRE.reverseEngineerTablePK(self.connection, source_table)
        self.assertEqual(res, 0)

        # Do migration:
        target_catalog = grt.classes.db_mysql_Catalog()
        res = DbMssqlMigration.migrateCatalog(source_catalog, target_catalog)
        self.assertEqual(res, 0)
        target_table = grt.classes.db_mysql_Table()
        res = DbMssqlMigration.migrateTableToMySQL(source_table, target_table)
        self.assertEqual(res, 0)
        res = DbMssqlMigration.migrateTableColumnsToMySQL(source_table, target_table)
        self.assertEqual(res, 0)
        res = DbMssqlMigration.migrateTablePrimaryKeyToMySQL(source_table, target_table)
        self.assertEqual(res, 0)

        pk_cols = [ column.name for column in target_table.columns if target_table.isPrimaryKeyColumn(column) ]
        self.assertEqual(set(['VendorID', 'ContactID']), set(pk_cols))

    def test_migrate_indices(self):
        # Prepare source objects:
        source_catalog, source_schema, source_table = self._set_catalog_schema_table('AdventureWorks', 'HumanResources', 'EmployeeAddress')
        res = DbMssqlRE.reverseEngineerUserDatatypes(self.connection, source_catalog)
        self.assertEqual(res, 0)
        res = DbMssqlRE.reverseEngineerTableColumns(self.connection, source_table)
        self.assertEqual(res, 0)
        res = DbMssqlRE.reverseEngineerTableIndices(self.connection, source_table)
        self.assertEqual(res, 0)

        # Do migration:
        target_catalog = grt.classes.db_mysql_Catalog()
        res = DbMssqlMigration.migrateCatalog(source_catalog, target_catalog)
        self.assertEqual(res, 0)
        target_table = grt.classes.db_mysql_Table()
        res = DbMssqlMigration.migrateTableToMySQL(source_table, target_table)
        self.assertEqual(res, 0)
        res = DbMssqlMigration.migrateTableColumnsToMySQL(source_table, target_table)
        self.assertEqual(res, 0)
        res = DbMssqlMigration.migrateTableIndicesToMySQL(source_table, target_table)
        self.assertEqual(res, 0)

        # Additional tests:
        detected_index = 0
        for index in target_table.indices:
            if index.name == 'PK_EmployeeAddress_EmployeeID_AddressID':
                detected_index += 1
            elif index.name == 'AK_EmployeeAddress_rowguid':
                detected_index += 1
                self.assertEqual(index.unique, 1)
                self.assertEqual(index.isPrimary, 0)
                self.assertEqual(index.indexType, 'UNIQUE')
                self.assertEqual(set(icol.referencedColumn.name for icol in index.columns), set(['rowguid']))
        self.assertEqual(detected_index, 1)

    def test_migrate_foreign_keys(self):
        # Prepare the scenario migrating needed objects along the way:
        source_catalog = grt.classes.db_mssql_Catalog()
        source_catalog.name = 'AdventureWorks'

        res = DbMssqlRE.reverseEngineerUserDatatypes(self.connection, source_catalog)
        self.assertEqual(res, 0)

        for schema_name in ['Person', 'Purchasing']:
            source_schema = grt.classes.db_mssql_Schema()
            source_schema.name = schema_name
            source_schema.owner = source_catalog
            res = DbMssqlRE.reverseEngineerTables(self.connection, source_schema)
            self.assertEqual(res, 0)
            for table in source_schema.tables:
                # Only rev eng the relevant tables
                if source_schema.name + '.' + table.name not in ['Person.Contact', 'Person.ContactType', 'Purchasing.Vendor', 'Purchasing.VendorContact']:
                    continue
                res = DbMssqlRE.reverseEngineerTableColumns(self.connection, table)
                self.assertEqual(res, 0)
            source_catalog.schemata.append(source_schema)

        target_catalog = grt.classes.db_mysql_Catalog()
        res = DbMssqlMigration.migrateCatalog(source_catalog, target_catalog)
        self.assertEqual(res, 0)

        # Second pass through the tables, to be sure that all of the columns are there and to RevEng those
        # objects that require rev eng of table columns:
        source_target_schemata = list(zip(source_catalog.schemata, target_catalog.schemata))
        self.assertEqual(len(source_target_schemata), 2)
        test_target_table = None
        for source_schema, target_schema in source_target_schemata:
            res = DbMssqlMigration.migrateSchema(source_schema, target_schema)
            self.assertEqual(res, 0)

            self.assertTrue(len(source_schema.tables) > 0)
            self.assertEqual(len(source_schema.tables), len(target_schema.tables))
            for source_table, target_table in zip(source_schema.tables, target_schema.tables):
                self.assertEqual(source_table.name, target_table.name)
                # Only migrate the relevant tables
                if source_schema.name + '.' + source_table.name not in ['Person.Contact', 'Person.ContactType', 'Purchasing.Vendor', 'Purchasing.VendorContact']:
                    continue
                res = DbMssqlMigration.migrateTableToMySQL(source_table, target_table)
                self.assertEqual(res, 0)
                res = DbMssqlMigration.migrateTableColumnsToMySQL(source_table, target_table)
                self.assertEqual(res, 0)

                if source_table.name == 'VendorContact':
                    res = DbMssqlRE.reverseEngineerTableFKs(self.connection, source_table)
                    self.assertEqual(res, 0)
                    self.assertEqual(len(source_table.foreignKeys), 3)
                    res = DbMssqlMigration.migrateTableForeignKeysToMySQL(source_table, target_table)
                    self.assertEqual(res, 0)
                    test_target_table = target_table

        # The actual tests for the foreign keys migration:
        self.assertIsNotNone(test_target_table)
        self.assertEqual(len(test_target_table.foreignKeys), 3)
        found_foreign_keys = 0
        for foreign_key in test_target_table.foreignKeys:
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
