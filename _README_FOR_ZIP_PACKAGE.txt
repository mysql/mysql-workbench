This readme describes the dependencies of MySQL Workbench when installed from the zip package on Windows.

The normal setup using the msi package or an installation using the MySQL Installer both check if
any of the prerequisites MySQL Workbench needs are missing on the target system and warns the user
about this. Additionally, both allow to download the missing prerequisites.

This doesn't happen when using the zip package and if any of the prerequisites is missing
MySQL Workbench will refuse to work without a good error message. So make sure you installed everything.

MySQL Workbench needs the following prerequisites:

    Microsoft .NET Framework 4 Client Profile (http://www.microsoft.com/download/en/details.aspx?id=17113)
    Visual C++ Redistributable for Visual Studio 2013 (http://www.microsoft.com/en-us/download/details.aspx?id=40784)

Depending on which MySQL Workbench package you downloaded you either need the 32bit or 64bit version of the
Visual C++ runtime. It is essential that you pick the correct architecture.
