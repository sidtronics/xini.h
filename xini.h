/*
 * xini.h - X-macro powered header-only INI parser - v0.1.0
 * ========================================================
 *
 * Just define your INI schema using the provided X-macro API. The library
 * specializes itself to that schema. All you need to do is call
 * xini_parse_config(...), which parses the entire configuration into an
 * xini_config structure. Individual entries can then be accessed through
 * xini_config.<section>.<key>.
 *
 * Schema definition:
 * ~~~~~~~~~~~~~~~~~~
 *
 * XINI_SECTIONS
 *  Required. List of all sections.
 *  Can be either XINI_STATIC_SECTION() or XINI_DYNAMIC_SECTION()
 *
 * XINI_STATIC_SECTION(name, ENTRIES)
 *  Section with a fixed key set. Unknown keys in the section are hard errors.
 *  Parameters:
 *      - name: Section name. Matches [name] in the INI file.
 *      - ENTRIES: Macro function ENTRIES(S) containing XINI_ENTRY lines.
 *
 * XINI_DYNAMIC_SECTION(name)
 *  Section whose keys are not known ahead of time. Each key-value pair is
 *  forwarded to a handler you implement:
 *      bool xini_<name>_entry_handler(xini_context *ctx);
 *  Parameters:
 *      - name: Section name. Matches [name] in the INI file.
 *
 * XINI_ENTRY(S, type, key, default)
 *  Declares one entry inside an ENTRIES(S) macro.
 *  Parameters:
 *      - S: Used internally by the library. Keep it as it is.
 *      - type: data type of the entry. See the supported types below.
 *      - key: Key name. Matches the key in the INI file.
 *      - default: default value assigned by xini_config_init().
 *
 * Custom enumerations:
 * ~~~~~~~~~~~~~~~~~~~
 *
 * XINI_ENUMS
 *  Optional. List of XINI_ENUM lines.
 *
 * XINI_ENUM(name, VALUES)
 *  Defines the custom enum.
 *  Parameters:
 *      - name: name of the custom enum. Use xini_enum_<name> as the type
 *              argument in XINI_ENTRY.
 *      - VALUES: list of XINI_ENUM_VAL lines.
 *
 * XINI_ENUM_VAL(IDENTIFIER, string)
 *  Defines an enumerator and its corresponding string value used in INI.
 *  Parameters:
 *      - IDENTIFIER: Enumerator constant used in the code.
 *      - string: String used in the INI file and mapped to this enumerator.
 *
 * Supported types:
 * ~~~~~~~~~~~~~~~~
 *
 *  xini_int: Integer
 *  xini_dbl: Float
 *  xini_str: String
 *  xini_bool: Boolean
 *  xini_enum_<name>: Custom user enum
 *
 * Options:
 * ~~~~~~~~
 *
 * XINI_LINE_BUFFER_SIZE
 *  Maximum size of line supported in a file (incl. newline). default: 256
 *
 * XINI_SECTION_MASK_TYPE
 *  Data type of entry mask used internally. default: uint32_t
 *
 * XINI_ERROR(ctx, fmt, ...)
 *  Override the error message reporter. default: xini__error
 *  Check out xini__error() for reference.
 *  Parameters:
 *      - ctx: xini_context type. Used for filename and line number.
 *      - fmt: Error message format string.
 *      - ...: Variadic arguments for fmt.
 *
 * API:
 * ~~~~
 *
 * API documentation can be found below.
 *
 */

#ifndef XINI_H_
#define XINI_H_

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define XINI_IMPLEMENTATION

// options
#ifndef XINI_SECTION_MASK_TYPE
#define XINI_SECTION_MASK_TYPE uint32_t
#endif

#ifndef XINI_LINE_BUFFER_SIZE
#define XINI_LINE_BUFFER_SIZE 256
#endif

#ifndef XINI_ERROR
#define XINI_ERROR(ctx, ...) xini__error((ctx), __VA_ARGS__)
#endif

#ifndef XINI_SECTIONS
#error                                                                         \
    "[xini.h]: ERROR: XINI_SECTIONS not defined. Please define XINI_SECTIONS before including xini.h"
#endif

// silence LSP
#ifndef XINI_SECTIONS
#define XINI_SECTIONS
#endif

// disable XINI_ENUMS if unused
#ifndef XINI_ENUMS
#define XINI_ENUMS
#endif

typedef int xini_int;
typedef const char *xini_str;
typedef double xini_dbl;
typedef bool xini_bool;

// generate custom enums
#define XINI_ENUM_VAL(id, string) id,
#define XINI_ENUM(name, values)                                                \
  typedef enum _xini_enum_##name{values} xini_enum_##name;
XINI_ENUMS
#undef XINI_ENUM
#undef XINI_ENUM_VAL

#define XINI_ENTRY(sname, type, name, default_value)                           \
  xini__entry_id_##sname##_##name,
#define XINI_DYNAMIC_SECTION(sname)
#define XINI_STATIC_SECTION(sname, entries)                                    \
  typedef enum {                                                               \
    entries(sname) xini__##sname##_entry_count                                 \
  } xini_##sname##_entry_id;                                                   \
  _Static_assert(sizeof(XINI_SECTION_MASK_TYPE) * CHAR_BIT >=                  \
                     xini__##sname##_entry_count,                              \
                 "[xini.h]: ERROR: XINI_SECTION_MASK_TYPE cannot "             \
                 "accommodate current "                                        \
                 "number of "                                                  \
                 "entries in " #sname                                          \
                 "section. Please change XINI_SECTION_MASK_TYPE to "           \
                 "a type with larger size.");
XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
#undef XINI_ENTRY

#define XINI__GET_ENTRY_FLAG(sname, name)                                      \
  (1 << (xini__entry_id_##sname##_##name))

#define XINI__GET_SECTION_MASK(cfg, sname) (cfg->sname.xini__entry_mask)

// generate xini_config
typedef struct _xini_config {
#define XINI_ENTRY(sname, type, name, default_value) type name;
#define XINI_DYNAMIC_SECTION(sname)
#define XINI_STATIC_SECTION(sname, entries)                                    \
  struct {                                                                     \
    XINI_SECTION_MASK_TYPE xini__entry_mask;                                   \
    entries(sname)                                                             \
  } sname;
  XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
#undef XINI_ENTRY
} xini_config;

// ~~~ //
// API //
// ~~~ //

typedef struct _xini_entry {
  const char *key;
  const char *value;
} xini_entry;

typedef struct _xini_context {
  const char *filepath;
  void *userdata;
  xini_entry entry;
  size_t line;
} xini_context;

// context //
// ======= //

// Returns a fresh context.
// userdata is stored in xini_context for it be used inside dynamic section
// handlers. Pass NULL if unused.
xini_context xini_context_init(const char *filepath, void *userdata);

// config //
// ====== //

// Populates cfg with the default values declared in XINI_ENTRY.
// Call before xini_config_parse().
void xini_config_init(xini_config *cfg);

// Parses the file into cfg. Returns false on error.
// cfg is freed automatically on failure.
bool xini_config_parse(xini_context *ctx, xini_config *cfg);

// Dumps the cfg to file. Unset entries are skipped.
void xini_config_print(FILE *f, const xini_config *cfg);

// Frees all heap-allocated strings and zeroes cfg.
void xini_config_free(xini_config *cfg);

// helpers //
// ======= //

// Non-zero if at least one entry in section was parsed from the file.
#define xini_is_section_parsed(cfg, section)                                   \
  (XINI__GET_SECTION_MASK((cfg), section) != 0)

// Non-zero if key inside section was parsed from the file.
#define xini_is_entry_parsed(cfg, section, entry)                              \
  (XINI__GET_SECTION_MASK((cfg), section) &                                    \
   XINI__GET_ENTRY_FLAG(section, entry))

// handlers //
// ======== //

// One handler is generated for each dynamic section.
// Signature:
//      bool xini_<dynamic_section_name>_entry_handler(xini_context *);
#define XINI_DYNAMIC_SECTION(sname)                                            \
  bool xini_##sname##_entry_handler(xini_context *ctx);
#define XINI_STATIC_SECTION(sname, entries)
XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION

#endif // !XINI_H_

////////////////////////////////////////////////////////////////////////////////

#ifdef XINI_IMPLEMENTATION

static inline void xini__error(const xini_context *ctx, const char *fmt, ...) {
  va_list ap;
  fputs("[xini.h]:", stderr);
  if (ctx->filepath) {
    fprintf(stderr, " %s:", ctx->filepath);
    if (ctx->line > 0)
      fprintf(stderr, "%zu:", ctx->line);
  }
  fputc(' ', stderr);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
}

static inline char *xini__strdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *p = (char *)malloc(n);
  return (char *)(p ? memcpy(p, s, n) : NULL);
}

xini_context xini_context_init(const char *filepath, void *userdata) {
  return (xini_context){.filepath = filepath,
                        .userdata = userdata,
                        .entry = {.key = NULL, .value = NULL},
                        .line = 0};
}

void xini_config_init(xini_config *cfg) {
#define XINI_ENTRY(sname, type, name, default_value)                           \
  cfg->sname.name = default_value;
#define XINI_DYNAMIC_SECTION(sname)
#define XINI_STATIC_SECTION(sname, entries)                                    \
  XINI__GET_SECTION_MASK(cfg, sname) = 0;                                      \
  entries(sname)
  XINI_SECTIONS
#undef XINI_ENTRY
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
}

static inline bool xini__parse_str(xini_str *dest, const char *src) {
  *dest = xini__strdup(src);
  if (!*dest)
    return false;
  else
    return true;
}

static inline bool xini__parse_int(xini_int *dest, const char *src) {
  char *endptr = NULL;
  long val;

  errno = 0;
  val = strtol(src, &endptr, 10);

  if (errno == ERANGE || *endptr != '\0')
    return false;

  if (val < INT_MIN || val > INT_MAX)
    return false;

  *dest = (int)val;
  return true;
}

static inline bool xini__parse_dbl(xini_dbl *dest, const char *src) {

  char *endptr = NULL;
  double val;

  errno = 0;
  val = strtod(src, &endptr);

  if (errno == ERANGE || *endptr != '\0')
    return false;

  *dest = val;
  return true;
}

static inline bool xini__parse_bool(xini_bool *dest, const char *src) {

  if (strcmp(src, "true") == 0)
    *dest = true;
  else if (strcmp(src, "false") == 0)
    *dest = false;
  else
    return false;

  return true;
}

// generate enum parsers
#define XINI_ENUM_VAL(id, string)                                              \
  else if (strcmp(src, #string) == 0) {                                        \
    *dest = id;                                                                \
  }

#define XINI_ENUM(name, values)                                                \
  static inline bool xini__parse_enum_##name(xini_enum_##name *dest,           \
                                             const char *src) {                \
    if (0) {                                                                   \
    }                                                                          \
    values else {                                                              \
      return false;                                                            \
    }                                                                          \
                                                                               \
    return true;                                                               \
  }

XINI_ENUMS

#undef XINI_ENUM
#undef XINI_ENUM_VAL

typedef enum {
  XINI__SECTION_NONE,
  XINI__SECTION_UNKNOWN,
#define XINI_DYNAMIC_SECTION(sname) XINI__SECTION_##sname,
#define XINI_STATIC_SECTION(sname, entries) XINI__SECTION_##sname,
  XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
} xini_section;

static inline bool xini__parse_entry(xini_section section, xini_context *ctx,
                                     xini_config *cfg) {

  switch (section) {

  case XINI__SECTION_NONE:
    /* skip entries not under any section */
    break;

  case XINI__SECTION_UNKNOWN:
    /* skip entries under unknown section */
    break;

#define XINI_ENUM(name, values) xini_enum_##name : xini__parse_enum_##name,
#define XINI_ENTRY(sname, type, name, default_value)                           \
  else if (strcmp(ctx->entry.key, #name) == 0) {                               \
    if (!(_Generic((cfg->sname.name),                                          \
              XINI_ENUMS xini_str: xini__parse_str,                            \
              xini_int: xini__parse_int,                                       \
              xini_dbl: xini__parse_dbl,                                       \
              xini_bool: xini__parse_bool))(&cfg->sname.name,                  \
                                            ctx->entry.value)) {               \
      XINI_ERROR(ctx, "invalid value for key '%s' in section '%s'",            \
                 ctx->entry.key, #sname);                                      \
      return 0;                                                                \
    }                                                                          \
    XINI__GET_SECTION_MASK(cfg, sname) |= XINI__GET_ENTRY_FLAG(sname, name);   \
  }

#define XINI_DYNAMIC_SECTION(sname)                                            \
  case XINI__SECTION_##sname:                                                  \
    if (!xini_##sname##_entry_handler(ctx)) {                                  \
      XINI_ERROR(ctx, "dynamic section handler for section '%s' failed",       \
                 #sname);                                                      \
      return 0;                                                                \
    }                                                                          \
    break;

#define XINI_STATIC_SECTION(sname, entries)                                    \
  case XINI__SECTION_##sname:                                                  \
    if (0) {                                                                   \
    }                                                                          \
    entries(sname) else {                                                      \
      XINI_ERROR(ctx, "unknown key '%s' for section '%s'", ctx->entry.key,     \
                 #sname);                                                      \
      return 0;                                                                \
    }                                                                          \
    break;

    XINI_SECTIONS

#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
#undef XINI_ENTRY
#undef XINI_ENUM
  }

  return 1;
}

static inline char *xini__trim(char *s) {

  while (isspace(*s))
    s++;
  if (*s == 0)
    return s;

  char *end = s + strlen(s) - 1;
  while (end > s && isspace(*end))
    *end-- = 0;

  return s;
}

typedef enum {
  XINI__PARSED_EOF,
  XINI__PARSED_ERROR,
  XINI__PARSED_SECTION,
  XINI__PARSED_ENTRY,
} xini_parse_status;

static inline xini_parse_status xini__parse_next(FILE *file, xini_context *ctx,
                                                 char *line) {

  xini_entry *pair = &ctx->entry;

  while (fgets(line, XINI_LINE_BUFFER_SIZE, file)) {

    ctx->line++;

    char *start = xini__trim(line);
    if (*start == '\0' || *start == '#' || *start == ';')
      continue;

    if (*start == '[') {
      char *end = strchr(start + 1, ']');
      if (!end) {
        XINI_ERROR(ctx, "expected ']'");
        return XINI__PARSED_ERROR;
      }

      *end = '\0';

      pair->key = start + 1;
      return XINI__PARSED_SECTION;
    }

    char *eq = strchr(start, '=');
    if (!eq) {
      XINI_ERROR(ctx, "expected '='");
      return XINI__PARSED_ERROR;
    }

    *eq = 0;
    char *key = xini__trim(start);
    if (!*key) {
      XINI_ERROR(ctx, "missing key");
      return XINI__PARSED_ERROR;
    }

    char *value = xini__trim(eq + 1);
    if (!*value) {
      XINI_ERROR(ctx, "missing value");
      return XINI__PARSED_ERROR;
    }

    if (*value == '"') {
      value = value + 1;
      char *end = strchr(value, '"');
      if (!end) {
        XINI_ERROR(ctx, "expected '\"'");
        return XINI__PARSED_ERROR;
      }
      *end = 0;
    }

    pair->key = key;
    pair->value = value;
    return XINI__PARSED_ENTRY;
  }

  return XINI__PARSED_EOF;
}

static inline xini_section xini__parse_section(const char *section) {
  if (0) {
  }
#define XINI_DYNAMIC_SECTION(sname)                                            \
  else if (strcmp(section, #sname) == 0) return XINI__SECTION_##sname;
#define XINI_STATIC_SECTION(sname, entries)                                    \
  else if (strcmp(section, #sname) == 0) return XINI__SECTION_##sname;
  XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
  else return XINI__SECTION_UNKNOWN;
}

bool xini_config_parse(xini_context *ctx, xini_config *cfg) {

  FILE *file = fopen(ctx->filepath, "r");
  if (!file) {
    XINI_ERROR(ctx, "error opening file: %s", strerror(errno));
    return false;
  }

  xini_parse_status status;
  char line[XINI_LINE_BUFFER_SIZE];
  xini_section section = XINI__SECTION_NONE;
  while ((status = xini__parse_next(file, ctx, line))) {

    // Not using switch-case to avoid using yet another goto-label to break
    // out of while loop on EOF

    if (status == XINI__PARSED_EOF)
      break;

    if (status == XINI__PARSED_ERROR)
      goto bail;

    if (status == XINI__PARSED_SECTION) {
      section = xini__parse_section(ctx->entry.key);
      if (section == XINI__SECTION_UNKNOWN) {
        XINI_ERROR(ctx, "unknown section encountered '%s'", ctx->entry.key);
      }
      continue;
    }

    if (!xini__parse_entry(section, ctx, cfg))
      goto bail;
  }

  if (section == XINI__SECTION_NONE) {
    XINI_ERROR(ctx, "no valid sections found");
    goto bail;
  }

  return true;

bail:
  fclose(file);
  xini_config_free(cfg);
  return false;
}

static inline void xini__print_str(FILE *file, const char *key,
                                   xini_str value) {

  if (value)
    fprintf(file, "%s = \"%s\"\n", key, value);
  else
    fprintf(file, "%s = %s\n", key, "NULL");
}

static inline void xini__print_int(FILE *file, const char *key,
                                   xini_int value) {
  fprintf(file, "%s = %d\n", key, value);
}

static inline void xini__print_dbl(FILE *file, const char *key,
                                   xini_dbl value) {
  fprintf(file, "%s = %.2f\n", key, value);
}

static inline void xini__print_bool(FILE *file, const char *key,
                                    xini_bool value) {

  fprintf(file, "%s = %s\n", key, value ? "true" : "false");
}

// generate enum printers
#define XINI_ENUM_VAL(id, str) #str,
#define XINI_ENUM(name, values)                                                \
  static inline void xini__print_enum_##name(FILE *file, const char *key,      \
                                             xini_enum_##name value) {         \
                                                                               \
    const char *enum_to_str[] = {values};                                      \
    fprintf(file, "%s = \"%s\"\n", key, enum_to_str[value]);                   \
  }

XINI_ENUMS

#undef XINI_ENUM
#undef XINI_ENUM_VAL

void xini_config_print(FILE *file, const xini_config *cfg) {
#define XINI_ENUM(name, values) xini_enum_##name : xini__print_enum_##name,
#define XINI_ENTRY(sname, type, name, default_value)                           \
  (_Generic((cfg->sname.name),                                                 \
       XINI_ENUMS xini_str: xini__print_str,                                   \
       xini_int: xini__print_int,                                              \
       xini_dbl: xini__print_dbl,                                              \
       xini_bool: xini__print_bool))(file, #name, cfg->sname.name);

#define XINI_DYNAMIC_SECTION(sname)
#define XINI_STATIC_SECTION(sname, entries)                                    \
  fprintf(file, "[%s]\n", #sname);                                             \
  entries(sname) fprintf(file, "\n");

  XINI_SECTIONS

#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
#undef XINI_ENTRY
#undef XINI_ENUM
}

static inline void xini__free_str(xini_str *s) { free((void *)*s); }
static inline void xini__free_int(xini_int *s) { (void)s; }
static inline void xini__free_dbl(xini_dbl *s) { (void)s; }
static inline void xini__free_bool(xini_bool *s) { (void)s; }

#define XINI_ENUM(name, values)                                                \
  static inline void xini__free_enum_##name(xini_enum_##name *s) { (void)s; }
XINI_ENUMS
#undef XINI_ENUM

void xini_config_free(xini_config *cfg) {
#define XINI_ENUM(name, values) xini_enum_##name : xini__free_enum_##name,
#define XINI_ENTRY(sname, type, name, default_value)                           \
  if (xini_is_entry_parsed(cfg, sname, name)) {                                \
    _Generic((cfg->sname.name),                                                \
        XINI_ENUMS xini_str: xini__free_str,                                   \
        xini_int: xini__free_int,                                              \
        xini_dbl: xini__free_dbl,                                              \
        xini_bool: xini__free_bool)(&cfg->sname.name);                         \
  }

#define XINI_DYNAMIC_SECTION(sname)
#define XINI_STATIC_SECTION(sname, entries) entries(sname)
  XINI_SECTIONS
  *cfg = (xini_config){0};
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
#undef XINI_ENTRY
#undef XINI_ENUM
}

#endif // XINI_IMPLEMENTATION

/*  Revision History:
 *
 *   0.1.0 (2026-06-15) initial release
 *
 */

/*
 * Copyright 2026 Siddhesh Dharme <siddheshdharme18@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
