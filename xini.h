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

typedef int xini_int;
typedef const char *xini_str;
typedef double xini_dbl;

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

static inline xini_context xini_context_init(const char *filename,
                                             void *userdata) {
  return (xini_context){.filename = filename,
                        .userdata = userdata,
                        .entry = {.key = NULL, .value = NULL}};
}

bool xini_parse_config(xini_context *ctx, xini_config *cfg);
void xini_print_config(FILE *f, const xini_config *cfg);

// generate dynamic section handlers
#define XINI_DYNAMIC_SECTION(sname)                                            \
  bool xini_##sname##_entry_handler(xini_context *ctx);
#define XINI_STATIC_SECTION(sname, entries)
XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION

#endif // !XINI_H_

////////////////////////////////////////////////////////////////////////////////

#ifdef XINI_IMPLEMENTATION

static inline bool xini__parse_str(xini_str *dest, const char *src) {
  *dest = strdup(src);
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

// generate enum parsers
#define XINI_ENUM_VAL(id, string)                                              \
  else if (strcmp(src, string) == 0) {                                         \
    *dest = id;                                                                \
  }

#define XINI_ENUM(name, values)                                                \
  static inline bool xini__parse_enum_##name(xini_enum_##name *dest,           \
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
    /* handle entries without any section */
    break;

  case XINI__SECTION_UNKNOWN:
    /* handle unknown section */
    break;

#define XINI_ENUM(name, values) xini_enum_##name : xini__parse_enum_##name,
#define XINI_ENTRY(sname, name, type)                                          \
  else if (strcmp(ctx->entry.key, #name) == 0) {                               \
    if (!(_Generic((cfg->sname.name),                                          \
              XINI_ENUMS xini_str: xini__parse_str,                            \
              xini_int: xini__parse_int,                                       \
              xini_dbl: xini__parse_dbl))(&cfg->sname.name,                    \
                                          ctx->entry.value)) {                 \
      /* conversion error */                                                   \
      return 0;                                                                \
    }                                                                          \
  }

#define XINI_DYNAMIC_SECTION(sname)                                            \
  case XINI__SECTION_##sname:                                                  \
    if (!xini_##sname##_entry_handler(ctx)) {                                  \
      return 0;                                                                \
    }                                                                          \
    break;

#define XINI_STATIC_SECTION(sname, entries)                                    \
  case XINI__SECTION_##sname:                                                  \
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

static char *xini__trim(char *s) {

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
  XINI__PARSED_ENTRY,
  XINI__PARSED_SECTION,
  XINI__PARSED_EOF,
  XINI__PARSED_ERROR,
} xini_parse_status;

static inline xini_parse_status xini__parse_next(FILE *file, xini_entry *pair,
                                                 char *line) {

  while (fgets(line, XINI_LINE_BUFFER_SIZE, file)) {

    char *start = xini__trim(line);
    if (*start == '\0' || *start == '#')
      continue;

    if (*start == '[') {
      char *end = strchr(start + 1, ']');
      if (!end) {
        return XINI__PARSED_ERROR;
        /* error */
      }

      *end = '\0';

      pair->key = start + 1;
      return XINI__PARSED_SECTION;
    }

    char *eq = strchr(start, '=');
    if (!eq) {
      /* error */
      return XINI__PARSED_ERROR;
    }

    *eq = 0;
    char *key = xini__trim(start);
    if (!*key) {
      /* error */
      return XINI__PARSED_ERROR;
    }

    char *value = xini__trim(eq + 1);
    if (!*value) {
      /* error */
      return XINI__PARSED_ERROR;
    }

    if (*value == '"') {
      value = value + 1;
      char *end = strchr(value, '"');
      if (!end) {
        /* error */
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

bool xini_parse_config(xini_context *ctx, xini_config *cfg) {

  FILE *file = fopen(ctx->filename, "r");
  if (!file) {
    /* error */
    return 0;
  }

  bool ret = 0;
  xini_parse_status status;
  char line[XINI_LINE_BUFFER_SIZE];
  xini_section section = XINI__SECTION_NONE;
  while ((status = xini__parse_next(file, &ctx->entry, line)) !=
         XINI__PARSED_EOF) {

    if (status == XINI__PARSED_SECTION) {
      section = xini__parse_section(ctx->entry.key);
      continue;
    }

    if (status == XINI__PARSED_ERROR) {
      /* error */
      goto bail;
    }

    if (!xini__parse_entry(section, ctx, cfg)) {
      /* error */
      goto bail;
    }
  }

  if (section == XINI__SECTION_NONE) {
    /* error */
    goto bail;
  }

  ret = 1;

bail:
  fclose(file);
  return ret;
}

static inline void xini__print_int(FILE *file, const char *key,
                                   xini_int value) {
  fprintf(file, "%s = %d\n", key, value);
}

static inline void xini__print_dbl(FILE *file, const char *key,
                                   xini_dbl value) {
  fprintf(file, "%s = %.2f\n", key, value);
}

static inline void xini__print_str(FILE *file, const char *key,
                                   xini_str value) {
  fprintf(file, "%s = \"%s\"\n", key, value);
}

// generate enum printers
#define XINI_ENUM_VAL(id, str) str,
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

void xini_print_config(FILE *file, const xini_config *cfg) {
#define XINI_ENUM(name, values) xini_enum_##name : xini__print_enum_##name,
#define XINI_ENTRY(sname, name, type)                                          \
  (_Generic((cfg->sname.name),                                                 \
       XINI_ENUMS xini_str: xini__print_str,                                   \
       xini_int: xini__print_int,                                              \
       xini_dbl: xini__print_dbl))(file, #name, cfg->sname.name);

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

#endif // XINI_IMPLEMENTATION
