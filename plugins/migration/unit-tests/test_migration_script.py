# Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA

import migration_test_main
import grt

import migration

class TestMigrationScripts(migration_test_main.MigrationTestCase):
    '''Test case to try different variants of migration scripts.'''

    def test_script_full_mssql_migration(self):
        # Sample migration plan:
        plan = migration.MigrationPlan()

        # Each migration source instance needs a concrete connection to the server with the source database:
        source_server_instance = grt.unserialize('./fixtures/mssql_connection.xml')
        self.assertEqual(source_server_instance.parameterValues['database'], 'AdventureWorks')
        plan.setSourceConnection(source_server_instance)

        # Each plan must set a migration target
        plan.setMigrationTarget('MySQL')        # a string with the name of the instance to be used

        # Now reverse engineer the source catalog:
        plan.migrationSource.reverseEngineer('AdventureWorks')

        # Proceed to the actual migration:
        plan.migrate()  # Now the target should have been populated with the migrated objects

        # Get the resulting MySQL script:
        script = plan.migrationTarget.generateScript()

        self.assertIsInstance(script, str)
