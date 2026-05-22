#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define XINI_ENUMS                                                             \
  XINI_ENUM(origin, ORIGIN_VALUES)                                             \
  XINI_ENUM(comm, COMM_VALUES)

#define ORIGIN_VALUES                                                          \
  XINI_ENUM_VAL(ORIGIN_TOP_LEFT, "top-left")                                   \
  XINI_ENUM_VAL(ORIGIN_TOP_RIGHT, "top-right")                                 \
  XINI_ENUM_VAL(ORIGIN_BOTTOM_LEFT, "bottom-left")                             \
  XINI_ENUM_VAL(ORIGIN_BOTTOM_RIGHT, "bottom-right")

#define COMM_VALUES                                                            \
  XINI_ENUM_VAL(COMM_UNIX, "unix")                                             \
  XINI_ENUM_VAL(COMM_NETWORK, "network")

#define XINI_SECTIONS                                                          \
  XINI_STATIC_SECTION(global, GLOBAL_ENTRIES)                                  \
  XINI_STATIC_SECTION(comm, COMM_ENTRIES)                                      \
  XINI_DYNAMIC_SECTION(indicators)

#define GLOBAL_ENTRIES(S)                                                      \
  XINI_ENTRY(S, x_padding, XINI_INT)                                           \
  XINI_ENTRY(S, y_padding, XINI_INT)                                           \
  XINI_ENTRY(S, colour, XINI_STR)                                              \
  XINI_ENTRY(S, timeout, XINI_DBL)                                             \
  XINI_ENTRY(S, origin, xini_enum_origin)

#define COMM_ENTRIES(S)                                                        \
  XINI_ENTRY(S, sock_addr, XINI_STR)                                           \
  XINI_ENTRY(S, type, xini_enum_comm)                                          \
  XINI_ENTRY(S, ip_addr, XINI_STR)                                             \
  XINI_ENTRY(S, port, XINI_INT)                                                \
  XINI_ENTRY(S, timeout, XINI_DBL)

typedef int XINI_INT;
typedef const char *XINI_STR;
typedef double XINI_DBL;

#ifdef XINI_ENUMS
#define XINI_ENUM_VAL(id, str) id,
#define XINI_ENUM(name, values)                                                \
  typedef enum _xini_enum_##name{values} xini_enum_##name;
XINI_ENUMS
#undef XINI_ENUM
#undef XINI_ENUM_VAL
#endif

typedef enum _xini_section {
#define XINI_DYNAMIC_SECTION(sname)         F_##sname,
#define XINI_STATIC_SECTION(sname, entries) F_##sname,
  XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
      XINI_STATIC_SECTION_NONE,
  XINI_STATIC_SECTION_UNKNOWN,
} xini_section;

typedef struct _xini_config {
#define XINI_ENTRY(s, name, type) type name;
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

typedef struct _xini_context {
  FILE *file;
  void *userdata;
  xini_section section;
  char *key;
  char *val;
  char line[256];
} xini_context;

static char *_trim(char *s) {

  while (isspace(*s)) s++;
  if (*s == 0) return s;

  char *end = s + strlen(s) - 1;
  while (end > s && isspace(*end)) *end-- = 0;

  return s;
}

#ifdef XINI_ENUMS
#define XINI_ENUM_VAL(id, str)                                                 \
  else if (strcmp(val, str) == 0) { *key = id; }

#define XINI_ENUM(name, values)                                                \
  static inline bool _xini_parse_enum_##name(xini_enum_##name *key,            \
                                             const char *val) {                \
    if (0) {}                                                                  \
    values else { /* error */                                                  \
      return 0;                                                                \
    }                                                                          \
    return 1;                                                                  \
  }

XINI_ENUMS

#undef XINI_ENUM
#undef XINI_ENUM_VAL
#endif

bool _xini_parse_str(XINI_STR *key, const char *val);
bool _xini_parse_int(XINI_INT *key, const char *val);
bool _xini_parse_dbl(XINI_DBL *key, const char *val);

static inline bool xini_open(xini_context *xc, const char *filename) {

  xc->file = fopen(filename, "r");
  if (!xc->file) {
    // error
  }

  return 1;
}

static inline bool xini_close(xini_context *xc) {
  fclose(xc->file);
  return 1;
}

static inline xini_section _parse_section(const char *section) {
#define XINI_DYNAMIC_SECTION(sname)                                            \
  if (strcmp(section, #sname) == 0) { return F_##sname; }
#define XINI_STATIC_SECTION(sname, entries)                                    \
  if (strcmp(section, #sname) == 0) { return F_##sname; }
  XINI_SECTIONS
#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
  return XINI_STATIC_SECTION_UNKNOWN;
}

#ifdef XINI_ENUMS
#define XINI_ENUM(name, values) xini_enum_##name : _xini_parse_enum_##name,
#define XINI_ENTRY(sname, name, type)                                          \
  else if (strcmp(ctx->key, #name) == 0) {                                     \
    if (!(_Generic((cfg->sname.name),                                          \
              XINI_ENUMS XINI_STR: _xini_parse_str,                            \
              XINI_INT: _xini_parse_int,                                       \
              XINI_DBL: _xini_parse_dbl))(&cfg->sname.name, ctx->val)) {       \
      /* conversion error */                                                   \
    }                                                                          \
  }
#else
#define XINI_ENTRY(sname, name, type)                                          \
  else if (strcmp(ctx->key, #name) == 0) {                                     \
    if (!(_Generic((cfg->sname.name),                                          \
              XINI_STR: _xini_parse_str,                                       \
              XINI_INT: _xini_parse_int,                                       \
              XINI_DBL: _xini_parse_dbl))(&cfg->sname.name, ctx->val)) {       \
      /* conversion error */                                                   \
    }                                                                          \
  }
#endif // XINI_ENUMS
#define XINI_DYNAMIC_SECTION(sname)                                            \
  bool xini_entry_handler_##sname(xini_context *ctx);
#define XINI_STATIC_SECTION(sname, entries)                                    \
  static inline bool _parse_section_entry_##sname(xini_context *ctx,           \
                                                  xini_config *cfg) {          \
    if (0) {}                                                                  \
    entries(sname) else { /* error */ }                                        \
    return 1;                                                                  \
  }

XINI_SECTIONS

#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION
#undef XINI_ENTRY
#undef XINI_ENUM

static inline bool _parse_entry(xini_context *ctx, xini_config *cfg) {

  switch (ctx->section) {

#define XINI_DYNAMIC_SECTION(sname)                                            \
  case F_##sname:                                                              \
    if (!xini_entry_handler_##sname(ctx)) { return 0; }                        \
    break;

#define XINI_STATIC_SECTION(sname, entries)                                    \
  case F_##sname:                                                              \
    if (!_parse_section_entry_##sname(ctx, cfg)) { return 0; }                 \
    break;

    XINI_SECTIONS

#undef XINI_DYNAMIC_SECTION
#undef XINI_STATIC_SECTION

  case XINI_STATIC_SECTION_NONE:
    /* handle entries without any section */
    break;

  case XINI_STATIC_SECTION_UNKNOWN:
    /* handle unknown section */
    break;
  }

  return 1;
}

typedef enum _xini_parse_status {
  XINI_PARSED_ENTRY,
  XINI_PARSED_SECTION,
  XINI_PARSED_EOF,
  XINI_PARSED_ERROR,
} xini_parse_status;

static inline xini_parse_status _parse_next(xini_context *ctx) {

  char *line = ctx->line;
  while (fgets(line, sizeof(ctx->line), ctx->file)) {

    char *start = _trim(line);
    if (*start == '\0' || *start == '#') continue;

    if (*start == '[') {
      char *end = strchr(start + 1, ']');
      if (!end) { return XINI_PARSED_ERROR; }

      *end         = '\0';

      ctx->section = _parse_section(start + 1);
      return XINI_PARSED_SECTION;
    }

    char *eq = strchr(start, '=');
    if (!eq) { return XINI_PARSED_ERROR; }

    *eq       = 0;
    char *key = _trim(start);
    if (!*key) { return XINI_PARSED_ERROR; }

    char *val = _trim(eq + 1);
    if (!*val) { return XINI_PARSED_ERROR; }

    if (*val == '"') {
      val       = val + 1;
      char *end = strchr(val, '"');
      if (!end) { return XINI_PARSED_ERROR; }
      *end = 0;
    }

    ctx->key = key;
    ctx->val = val;
    return XINI_PARSED_ENTRY;
  }

  return XINI_PARSED_EOF;
}

// #define XINI_DYNAMIC_SECTION(sname)
// #define XINI_STATIC_SECTION(sname, entries) \
//   bool xini_parse_section_##sname(xini_context *ctx, xini_config *cfg) { \
//     if (ctx->section != F_##sname) { \
//       /* error */ \
//       return 0; \
//     } \
//                                                                                \
//     xini_parse_status status; \
//     while ((status = _parse_next(ctx)) == XINI_PARSED_ENTRY) { \
//       _parse_section_##sname##_entry(ctx, cfg); \
//     } \
//                                                                                \
//     if (status == XINI_PARSED_ERROR) { \
//       /* error */ \
//       return 0; \
//     } \
//                                                                                \
//     return 1; \
//   }
//
// XINI_SECTIONS
//
// #undef XINI_DYNAMIC_SECTION
// #undef XINI_STATIC_SECTION

bool xini_parse_config(xini_context *ctx, xini_config *cfg) {

  xini_parse_status status;
  while ((status = _parse_next(ctx)) != XINI_PARSED_EOF) {

    if (status == XINI_PARSED_SECTION) continue;

    if (status == XINI_PARSED_ERROR) {
      // error
      return 0;
    }

    if (!_parse_entry(ctx, cfg)) {
      // error
      return 0;
    }
  }

  return 1;
}
