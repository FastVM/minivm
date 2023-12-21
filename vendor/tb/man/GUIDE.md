# Module creation

Modules are the largest logical unit of code in TB, they contain functions and globals which can be exported. Get started by writing:

```c
	// this will use the host machine for the target architecture and system, this is
	// helpful when doing JIT or non-cross AOT compilation
	TB_Module* module = tb_module_create_for_host(arch, TB_SYSTEM_WINDOWS, TB_DEBUGFMT_NONE, NULL);
```

```c
	// See TB_Arch, TB_System
	TB_Arch arch  = TB_ARCH_X86_64;
	TB_System sys = TB_SYSTEM_WINDOWS;

	// See TB_DebugFormat. When exporting the binary this decides
	// how the line info, type table and other debug information is
	// encoded.
	TB_DebugFormat debug_fmt = TB_DEBUGFMT_CODEVIEW;

	// See TB_FeatureSet, this allows us to tell the code generator
	// what extensions are active in the platform, an example is enabling
	// AVX or BF16
	TB_FeatureSet features = { 0 };

	TB_Module* module = tb_module_create_for_host(arch, sys, debug_fmt, &features);
```

# Exporter API

The exporting API allows for packaging compiled code into objects, shared/static or executable form. Once you've compiled all your functions in TB you may export to a file using:

```c
	// see TB_OutputFlavor for the full list
	TB_ModuleExporter* e = tb_make_exporter(module, TB_FLAVOR_OBJECT);
	if (!tb_exporter_to_file(e, module, "hello.obj")) {
		/* failed to export */
	}

	/* file has been exported! */
```

If instead you need to output a buffer in memory:

```c
	// see TB_OutputFlavor for the full list
	TB_ModuleExporter* e = tb_make_exporter(module, TB_FLAVOR_OBJECT);

	ptrdiff_t length;
	uint8_t* buffer = tb_exporter_to_buffer(e, module, &length);
	if (length < 0) {
		/* failed to export */
	}

	...

	tb_exporter_free_buffer(buffer);
```

# Builder API