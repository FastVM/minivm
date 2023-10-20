TildeBackend (Tilde or TB for short)

  TB is compiler backend in the form of a reasonable C library. This is built as an alternative to other larger compiler toolchains while providing the optimizations, machine code generation and object file export functionality necessary for the development of compilers.

  # Roadmap

    Code generation:
      We're starting with x64 but will be moving focus to Aarch64 soon.

    Optimizer:
      It's almost complete with all the -O1 level passes (mostly missing inlining).
      After that we can move towards -O2 level stuff (the goal is to compete with
      LLVM so we need to be a bit ambitious).

    Debug info:
      Codeview support and DWARF has not yet been started, there's plans on making a
      new debug info format eventually.

    Output targets:
     We currently have basic ELF64, COFF64, some current work is being done for
     PE and Macho-O. We got exporting object files but wanna go further because
     linkers ain't supposed to be separate programs.
