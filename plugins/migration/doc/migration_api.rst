The Migration Process
######################

The typical migration process is described in the following picture:

.. figure:: /images/migration_process.png
   :width: 80%
..   :align: center 

..   The typical migration workflow

The source database is hosted in the source RDBMS. The first step in the process involves "talking" to the
source RDBMS to get information about catalogs, schemas, tables, indices, etc. For each supported source
RDBMS there is one GRT module that provides functionality for this *reverse engineering process*. So, for
example, the module for Microsoft SQL Server reverse engineering is :mod:`grt.modules.DbMssql`.

At the reverse engineering stage the primary objective is to obtain information about the source database
objects and to store it in a handy way. This process often involves the creation of RDBMS specific objects
that store information about catalogs, schemas, tables, table columns, indices, primary and foreign keys,
constraints, etc. These objects are instances of source RDBMS specific classes that store information about
such database objects. Here's an example: for each table that is to be migrated from a MS SQL Server database
an instance of the :class:`grt.classes.db_mssql_Table` class is created and filled with proper values upon
reverse engineering.

After all the relevant information about the structure of the database is gathered in these GRT objects it
has to be ported into their corresponding GRT target database objects. This is the actual *migration process*.

.. note:: The transformation stage that would allow modifications of the database object upon migration (e.g.
    to rename a table column before migration) is yet to be implemented in Migration Tool 1.1.

Once the target database objects are created in the migration stage the next step would be to *forward 
engineer* these objects to create and run an SQL script for the target RDBMS. An optional *bulk data migration
step* might follow to port the data from the source database to the target database.

The Migration API
###################

Migration Plans
****************

Each migration is planned using an instance of the class :class:`grt.classes.MigrationPlan`.

.. method:: MigrationPlan.setMigrationSource(source)

    Sets the source RDBMS (the RDBMS where the database that will be ported is hosted). *source* is either
    a string with the RDBMS name or a direct instance of the RDBMS GRT class. If a string is used it should be
    one of these: ``'MSSQL'`` for Microsoft SQL Server, ``'SQLITE'`` for SQLite, ``'MYSQL'`` for MySQL.
    If you choose to pass an instance it should be one of the objects contained in the 
    :obj:`grt.root.wb.rdbmsMgmt.rdbms` list.

.. method:: MigrationPlan.supportedSources()

    Returns a list with information about the supported source RDBMSes. Each element in a list is a two elements
    tuple in which the first element is a string with the code of the RDBMS and the second element is another
    string with a human friendly name for the given RDBMS. Here's a sample return list: ::
    
        [ ('MSSQL', 'Microsoft SQL Server'),
          ('SQLITE', 'SQLite'),
          ('MYSQL', 'MySQL') ]

.. method:: MigrationPlan.setMigrationTarget(target)

    Like :meth:`setMigrationSource()` this method sets the target RDBMS (the RDBMS where the database is to be
    ported). *target* can either be a string with the RDBMS name or a direct instance of the RDBMS GRT class. 
    See the documentation of :meth:`setMigrationSource()` for further details.

    .. note:: In the current version of the Migration Tool the only supported target RDBMS is MySQL. Passing
        an unsupported value in *target* will result in an :class:`grt.classes.NotImplementedException` been
        thrown.

.. method:: MigrationPlan.supportedTargets()

    Like :meth:`supportedSources()` but the returned list corresponds to the supported target RDBMSes.

Setting Server Instances
************************

Each :class:`MigrationPlan` instance has a :attr:`migrationSource` and a :attr:`migrationTarget` attributes
that are initialized when :meth:`setMigrationSource()` and :meth:`setMigrationTarget()` are first called,
respectively. These are instances of the :class:`grt.classes.MigrationSource` and :class:`grt.classes.MigrationTarget`
classes.

The :class:`MigrationSource` class has several attributes and methods to establish a connection with the source
RDBMS database and perform its reverse engineering.

In order to query the source and target RDBMS a server instance must be instantiated and properly filled with
the needed connection data. There's a :class:`ServerInstance` class associated with the :attr:`migrationSource`
and :attr:`migrationTarget` attributes of the :class:`MigrationPlan` class. You should create an object of
the :class:`ServerInstance` class, assign values for each needed connection parameter and set it as the server
instance for the source/target RDBMS. Here's an example: ::

    plan = grt.classes.MigrationPlan()

    plan.setMigrationSource('MSSQL')
    source_server_instance = plan.migrationSource.ServerInstance()
    source_server_instance['host'] = '127.0.0.1'
    source_server_instance['port'] = 1333
    source_server_instance['user'] = 'mssqlusername'
    source_server_instance['password'] = 'mssqlpassword'
    plan.migrationSource.setServerInstance(source_server_instance)

    plan.setMigrationTarget('MYSQL')
    target_server_instance = plan.migrationTarget.ServerInstance()
    target_server_instance['host'] = '127.0.0.1'
    target_server_instance['port'] = 3306
    target_server_instance['user'] = 'mysqlusername'
    target_server_instance['password'] = 'mysqlpassword'
    plan.migrationTarget.setServerInstance(target_server_instance)

Reverse Engineering
********************

Database Object Classes
------------------------

In order to make the information about the database objects usable within the Migration Tool it has to be
ported into RDBMS specific class instances that store information for each object involved in the migration
process. You can get each class through the attribute :attr:`migrationSource` in your :class:`MigrationPlan`
instance. Here's a list of the available database object classes:

- :class:`MigrationSource.classes.Catalog`
    Stores information about a catalog.

- :class:`MigrationSource.classes.Schema`
    Stores information about a schema.

- :class:`MigrationSource.classes.Table`
    Stores information about a table.

- :class:`MigrationSource.classes.Column`
    Stores information about a table column.

- :class:`MigrationSource.classes.PrimaryKey`
    Stores information about the primary key(s) of a table.

- :class:`MigrationSource.classes.ForeignKey`
    Stores information about the foreign key(s) of a table.

- :class:`MigrationSource.classes.Index`
    Stores information about a database index.

General Informative Functions
------------------------------

These methods provide general information about database objects:

.. method:: MigrationSource.getCatalogs()

    Returns a list of the available catalogs for the source RDBMS server instance. Each element of the returned
    list is a string with the name of a catalog.

.. method:: MigrationSource.getSchemas(catalog)

    Returns a list of the available schemas for the given catalog
    


.. rubric:: Sample 1

.. literalinclude:: api_planning.py

