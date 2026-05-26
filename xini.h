#ifndef XINI_H_
#define XINI_H_

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xini_defs.h"
#define XINI_IMPLEMENTATION

#ifndef XINI_LINE_BUFFER_SIZE
#define XINI_LINE_BUFFER_SIZE 256
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

// generate enums
#define XINI_ENUM_VAL(id, string) id,
#define XINI_ENUM(name, values)                                                \
  typedef enum _xini_enum_##name{values} xini_enum_##name;
XINI_ENUMS
#undef XINI_ENUM
#undef XINI_ENUM_VAL

typedef int XINI_INT;
typedef const char *XINI_STR;
typedef double XINI_DBL;

typedef struct _xini_config {
#define XINI_ENTRY(sname, name, type) type name;
#define XINI_DYNAMIC_SECTION(sname)
#define XINI_STATIC_SECTION(sname, entries)                                    \
  struct {                                                                     \
    entries(sname)                                                             \
  } sname;
  XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
#undef XINI_ENTRY
} xini_config;

typedef struct _xini_entry {
  const char *key;
  const char *value;
} xini_entry;

typedef struct _xini_context {
  const char *filename;
  void *userdata;
  xini_entry entry;
} xini_context;

bool xini_parse_config(xini_context *ctx, xini_config *cfg);
void xini_dump_config(FILE *f, xini_config *cfg);

// generate dynamic section handlers
#define XINI_DYNAMIC_SECTION(sname)                                            \
  bool xini_##sname##_entry_handler(xini_context *ctx);
#define XINI_STATIC_SECTION(sname, entries)
XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION

#ifdef XINI_IMPLEMENTATION

static inline bool _xini_parse_str(XINI_STR *dest, const char *src) {
  *dest = strdup(src);
  if (!*dest)
    return false;
  else
    return true;
}

static inline bool _xini_parse_int(XINI_INT *dest, const char *src) {
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

static inline bool _xini_parse_dbl(XINI_DBL *dest, const char *src) {

  char *endptr = NULL;
  double val;

  errno = 0;
  val = strtod(src, &endptr);

  if (errno == ERANGE || *endptr != '\0')
    return false;

  *dest = val;
  return true;
}

// generate enum parsers
#define XINI_ENUM_VAL(id, string)                                              \
  else if (strcmp(src, string) == 0) {                                         \
    *dest = id;                                                                \
  }

#define XINI_ENUM(name, values)                                                \
  static inline bool _xini_parse_enum_##name(xini_enum_##name *dest,           \
                                             const char *src) {                \
    if (0) {                                                                   \
    }                                                                          \
    values else { /* error */                                                  \
      return 0;                                                                \
    }                                                                          \
    return 1;                                                                  \
  }

XINI_ENUMS

#undef XINI_ENUM
#undef XINI_ENUM_VAL

typedef enum _xini_section {
  XINI_SECTION_NONE,
  XINI_SECTION_UNKNOWN,
#define XINI_DYNAMIC_SECTION(sname) F_##sname,
#define XINI_STATIC_SECTION(sname, entries) F_##sname,
  XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
} xini_section;

static inline bool _parse_entry(xini_section section, xini_context *ctx,
                                xini_config *cfg) {

  switch (section) {

  case XINI_SECTION_NONE:
    /* handle entries without any section */
    break;

  case XINI_SECTION_UNKNOWN:
    /* handle unknown section */
    break;

#define XINI_ENUM(name, values) xini_enum_##name : _xini_parse_enum_##name,
#define XINI_ENTRY(sname, name, type)                                          \
  else if (strcmp(ctx->entry.key, #name) == 0) {                               \
    if (!(_Generic((cfg->sname.name),                                          \
              XINI_ENUMS XINI_STR: _xini_parse_str,                            \
              XINI_INT: _xini_parse_int,                                       \
              XINI_DBL: _xini_parse_dbl))(&cfg->sname.name,                    \
                                          ctx->entry.value)) {                 \
      /* conversion error */                                                   \
      return 0;                                                                \
    }                                                                          \
  }

#define XINI_DYNAMIC_SECTION(sname)                                            \
  case F_##sname:                                                              \
    if (!xini_##sname##_entry_handler(ctx)) {                                  \
      return 0;                                                                \
    }                                                                          \
    break;

#define XINI_STATIC_SECTION(sname, entries)                                    \
  case F_##sname:                                                              \
    if (0) {                                                                   \
    }                                                                          \
    entries(sname) else { /* error */ }                                        \
    break;

    XINI_SECTIONS

#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
#undef XINI_ENTRY
#undef XINI_ENUM
  }

  return 1;
}

static char *_trim(char *s) {

  while (isspace(*s))
    s++;
  if (*s == 0)
    return s;

  char *end = s + strlen(s) - 1;
  while (end > s && isspace(*end))
    *end-- = 0;

  return s;
}

typedef enum _xini_parse_status {
  XINI_PARSED_ENTRY,
  XINI_PARSED_SECTION,
  XINI_PARSED_EOF,
  XINI_PARSED_ERROR,
} xini_parse_status;

static inline xini_parse_status _parse_next(FILE *file, xini_entry *pair,
                                            char *line) {

  while (fgets(line, XINI_LINE_BUFFER_SIZE, file)) {

    char *start = _trim(line);
    if (*start == '\0' || *start == '#')
      continue;

    if (*start == '[') {
      char *end = strchr(start + 1, ']');
      if (!end) {
        return XINI_PARSED_ERROR;
        /* error */
      }

      *end = '\0';

      pair->key = start + 1;
      return XINI_PARSED_SECTION;
    }

    char *eq = strchr(start, '=');
    if (!eq) {
      /* error */
      return XINI_PARSED_ERROR;
    }

    *eq = 0;
    char *key = _trim(start);
    if (!*key) {
      /* error */
      return XINI_PARSED_ERROR;
    }

    char *value = _trim(eq + 1);
    if (!*value) {
      /* error */
      return XINI_PARSED_ERROR;
    }

    if (*value == '"') {
      value = value + 1;
      char *end = strchr(value, '"');
      if (!end) {
        /* error */
        return XINI_PARSED_ERROR;
      }
      *end = 0;
    }

    pair->key = key;
    pair->value = value;
    return XINI_PARSED_ENTRY;
  }

  return XINI_PARSED_EOF;
}

static inline xini_section _parse_section(const char *section) {
  if (0) {
  }
#define XINI_DYNAMIC_SECTION(sname)                                            \
  else if (strcmp(section, #sname) == 0) return F_##sname;
#define XINI_STATIC_SECTION(sname, entries)                                    \
  else if (strcmp(section, #sname) == 0) return F_##sname;
  XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
  else return XINI_SECTION_UNKNOWN;
}

bool xini_parse_config(xini_context *ctx, xini_config *cfg) {

  FILE *file = fopen(ctx->filename, "r");
  if (!file) {
    /* error */
    return 0;
  }

  bool ret = 0;
  xini_parse_status status;
  char line[XINI_LINE_BUFFER_SIZE];
  xini_section section = XINI_SECTION_NONE;
  while ((status = _parse_next(file, &ctx->entry, line)) != XINI_PARSED_EOF) {

    if (status == XINI_PARSED_SECTION) {
      section = _parse_section(ctx->entry.key);
      continue;
    }

    if (status == XINI_PARSED_ERROR) {
      /* error */
      goto bail;
    }

    if (!_parse_entry(section, ctx, cfg)) {
      /* error */
      goto bail;
    }
  }

  if (section == XINI_SECTION_NONE) {
    /* error */
    goto bail;
  }

  ret = 1;

bail:
  fclose(file);
  return ret;
}

static inline void _xini_print(FILE *f, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(f, fmt, ap);
  va_end(ap);
}

static inline void _xini_print_int(FILE *f, const char *key, XINI_INT value) {
  _xini_print(f, "%s = %d\n", key, value);
}

static inline void _xini_print_dbl(FILE *f, const char *key, XINI_DBL value) {
  _xini_print(f, "%s = %.2f\n", key, value);
}

static inline void _xini_print_str(FILE *f, const char *key, XINI_STR value) {
  _xini_print(f, "%s = \"%s\"\n", key, value);
}

// generate enum printers
#define XINI_ENUM_VAL(id, str) str,
#define XINI_ENUM(name, values)                                                \
  static inline void _xini_print_enum_##name(FILE *f, const char *key,         \
                                             xini_enum_##name value) {         \
                                                                               \
    const char *enum_to_str[] = {values};                                      \
    _xini_print(f, "%s = \"%s\"\n", key, enum_to_str[value]);                  \
  }

XINI_ENUMS

#undef XINI_ENUM
#undef XINI_ENUM_VAL

void xini_dump_config(FILE *f, xini_config *cfg) {
#define XINI_ENUM(name, values) xini_enum_##name : _xini_print_enum_##name,
#define XINI_ENTRY(sname, name, type)                                          \
  (_Generic((cfg->sname.name),                                                 \
       XINI_ENUMS XINI_STR: _xini_print_str,                                   \
       XINI_INT: _xini_print_int,                                              \
       XINI_DBL: _xini_print_dbl))(f, #name, cfg->sname.name);

#define XINI_DYNAMIC_SECTION(sname)
#define XINI_STATIC_SECTION(sname, entries)                                    \
  _xini_print(f, "[%s]\n", #sname);                                            \
  entries(sname) _xini_print(f, "\n");

  XINI_SECTIONS

#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
#undef XINI_ENTRY
#undef XINI_ENUM
}

#endif // XINI_IMPLEMENTATION
#endif // !XINI_H_
