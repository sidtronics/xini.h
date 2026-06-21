# xini.h

A tiny stb-style header-only INI parser library for C.

`xini.h` lets you define your entire configuration schema at compile time using a
small set of macros. The library then generates the required types and parsing
logic for that schema, providing type-safe access to configuration values
through a single `xini_config` structure.

## Features

* Schema-driven configuration parsing
* Library specializes itself to your schema at compile time
* Static sections with strict key validation
* Dynamic sections with user-defined handlers
* Custom enumeration generation / parsing
* Mechanism to easily extend this library to support your own types
* Supports default values for entires
* Configuration dumping and cleanup helpers

## Getting Started

1. Copy `xini.h` into your project.
2. Define your configuration schema using the X-macro API before including `xini.h`.
3. Define `XINI_IMPLEMENTATION` before including `xini.h` in exactly one TU.
4. Create a context using `xini_context_init()`.
5. Set default values using `xini_config_init()`.
6. Parse a config file with `xini_config_parse()`.
7. Access values through `xini_config.<section>.<key>`.
8. Free `xini_config` using `xini_config_free()`.

See `example/example.c` for a complete working example.

## Documentation

Documentation is present at top of `xini.h`. If anything is unclear, refer to
the example in `example/`. If you still have questions, feel free to open an
issue.

## License

MIT License. See `LICENSE` for details.
