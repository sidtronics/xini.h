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

#include "xini_defs.h"
#define XINI_IMPLEMENTATION

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

// generate enums
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

typedef struct _xini_entry {
  const char *key;
  const char *value;
} xini_entry;

typedef struct _xini_context {
  const char *filename;
  void *userdata;
  xini_entry entry;
  size_t line;
} xini_context;

// context
xini_context xini_context_init(const char *filename, void *userdata);

// config
void xini_config_init(xini_config *cfg);
void xini_config_free(xini_config *cfg);
bool xini_config_parse(xini_context *ctx, xini_config *cfg);
void xini_config_print(FILE *f, const xini_config *cfg);

#define xini_is_section_parsed(cfg, section)                                   \
  (XINI__GET_SECTION_MASK((cfg), section) != 0)

#define xini_is_entry_parsed(cfg, section, entry)                              \
  (XINI__GET_SECTION_MASK((cfg), section) &                                    \
   XINI__GET_ENTRY_FLAG(section, entry))

// dynamic section handlers
#define XINI_DYNAMIC_SECTION(sname)                                            \
  bool xini_##sname##_entry_handler(xini_context *ctx);
#define XINI_STATIC_SECTION(sname, entries)
XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION

#endif // !XINI_H_

////////////////////////////////////////////////////////////////////////////////

#ifdef XINI_IMPLEMENTATION

xini_context xini_context_init(const char *filename, void *userdata) {
  return (xini_context){.filename = filename,
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

static inline void xini__free_str(xini_str *s) { free((void *)*s); }
static inline void xini__free_int(xini_int *s) { (void)s; }
static inline void xini__free_dbl(xini_dbl *s) { (void)s; }

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
        xini_dbl: xini__free_dbl)(&cfg->sname.name);                           \
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

static inline void xini__error(const xini_context *ctx, const char *fmt, ...) {
  va_list ap;
  fputs("[xini.h]:", stderr);
  if (ctx->filename) {
    fprintf(stderr, " %s:", ctx->filename);
    if (ctx->line > 0)
      fprintf(stderr, "%zu:", ctx->line);
  }
  fputc(' ', stderr);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
}

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
              xini_dbl: xini__parse_dbl))(&cfg->sname.name,                    \
                                          ctx->entry.value)) {                 \
      XINI_ERROR(ctx, "invalid value for key '%s' in section '%s'",          \
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

  FILE *file = fopen(ctx->filename, "r");
  if (!file) {
    XINI_ERROR(ctx, "error opening file: %s", strerror(errno));
    return false;
  }

  xini_parse_status status;
  char line[XINI_LINE_BUFFER_SIZE];
  xini_section section = XINI__SECTION_NONE;
  while ((status = xini__parse_next(file, ctx, line))) {

    // not using switch-case to avoid using yet another goto-label to break
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

  if (value)
    fprintf(file, "%s = \"%s\"\n", key, value);
  else
    fprintf(file, "%s = %s\n", key, "NULL");
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

void xini_config_print(FILE *file, const xini_config *cfg) {
#define XINI_ENUM(name, values) xini_enum_##name : xini__print_enum_##name,
#define XINI_ENTRY(sname, type, name, default_value)                           \
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
