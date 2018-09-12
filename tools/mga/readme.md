MySQL GUI Automator (MGA)
---

This tool is a cross platform command line application that can be used to automate GUI applications, particularly for testing scenarios.

Building the tool requires the same 3rd party library setup as used by MySQL Workbench.

Recreating duktape Headers + Source
---
There's a configuration yaml file which you can use with `configuration.py` as described in the [duktape documentation](https://github.com/svaarala/duktape/blob/master/doc/duk-config.rst#purpose-of-duk_configh). In order to run `configure.py` you need the full repo of duktape. Copy the yaml file to the root of the duktape repo folder and run this command:

```bash
python tools/configure.py --architecture x64 --option-file mga_duk_config.yaml --output-directory ./output
```

The generated files will be in the `output` folder from where you can copy them to MGA.
