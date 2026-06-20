// Enumerations

#define XINI_ENUMS                                                             \
  XINI_ENUM(day, DAY_VALUES)                                                   \
  XINI_ENUM(month, MONTH_VALUES)                                               \
  XINI_ENUM(wm_kind, WM_VALUES)

#define DAY_VALUES                                                             \
  XINI_ENUM_VAL(DAY_SUNDAY, sunday)                                            \
  XINI_ENUM_VAL(DAY_MONDAY, monday)                                            \
  XINI_ENUM_VAL(DAY_TUESDAY, tuesday)                                          \
  XINI_ENUM_VAL(DAY_WEDNESDAY, wednesday)                                      \
  XINI_ENUM_VAL(DAY_THURSDAY, thursday)                                        \
  XINI_ENUM_VAL(DAY_FRIDAY, friday)                                            \
  XINI_ENUM_VAL(DAY_SATURDAY, saturday)

#define MONTH_VALUES                                                           \
  XINI_ENUM_VAL(MONTH_JANUARY, january)                                        \
  XINI_ENUM_VAL(MONTH_FEBRUARY, february)                                      \
  XINI_ENUM_VAL(MONTH_MARCH, march)                                            \
  XINI_ENUM_VAL(MONTH_APRIL, april)                                            \
  XINI_ENUM_VAL(MONTH_MAY, may)                                                \
  XINI_ENUM_VAL(MONTH_JUNE, june)                                              \
  XINI_ENUM_VAL(MONTH_JULY, july)                                              \
  XINI_ENUM_VAL(MONTH_AUGUST, august)                                          \
  XINI_ENUM_VAL(MONTH_SEPTEMBER, september)                                    \
  XINI_ENUM_VAL(MONTH_OCTOBER, october)                                        \
  XINI_ENUM_VAL(MONTH_NOVEMBER, november)                                      \
  XINI_ENUM_VAL(MONTH_DECEMBER, december)

#define WM_VALUES                                                              \
  XINI_ENUM_VAL(WM_FLOATING, floating)                                         \
  XINI_ENUM_VAL(WM_TILING, tiling)                                             \
  XINI_ENUM_VAL(WM_DYNAMIC, dynamic)                                           \
  XINI_ENUM_VAL(WM_SCROLLING, scrolling)

// User types

#define XINI_USER_TYPES                                                        \
  XINI_USER_TYPE(                                                              \
      struct {                                                                 \
        uint8_t r;                                                             \
        uint8_t g;                                                             \
        uint8_t b;                                                             \
      },                                                                       \
      color)

// Schema

#define XINI_SECTIONS                                                          \
  XINI_STATIC_SECTION(user, USER_ENTRIES)                                      \
  XINI_STATIC_SECTION(desktop, DESKTOP_ENTRIES)                                \
  XINI_DYNAMIC_SECTION(messages)

#define USER_ENTRIES(S)                                                        \
  XINI_ENTRY(S, xini_str, first_name, "first")                                 \
  XINI_ENTRY(S, xini_str, last_name, "last")                                   \
  XINI_ENTRY(S, xini_int, age, 0)                                              \
  XINI_ENTRY(S, xini_bool, show_desktop_info, false)                           \
  XINI_ENTRY(S, xini_enum_day, day_of_birth, DAY_SUNDAY)                       \
  XINI_ENTRY(S, xini_enum_month, month_of_birth, MONTH_JANUARY)

#define DESKTOP_ENTRIES(S)                                                     \
  XINI_ENTRY(S, xini_str, wallpaper_path, "~/wallpaper.png")                   \
  XINI_ENTRY(S, xini_enum_wm_kind, window_manager_kind, WM_FLOATING)           \
  XINI_ENTRY(S, xini_dbl, opacity, 1.0)                                        \
  XINI_ENTRY(S, xini_int, width, 800)                                          \
  XINI_ENTRY(S, xini_int, height, 600)                                         \
  XINI_ENTRY(S, xini_user_color, bg_color,                                     \
             ((xini_user_color){.r = 0x80, .g = 0x80, .b = 0x80}))

#define XINI_IMPLEMENTATION
#include "../xini.h"

// Implement the dynamic section handler for 'messages' section. You can fail
// early by returning false.

bool xini_messages_entry_handler(xini_context *ctx) {

  int *count = ctx->userdata;
  (*count)++;
  printf("[messages-handler]: [%d] %s says '%s'\n", *count, ctx->entry.key,
         ctx->entry.value);

  return true;
}

// Helper for parsing hex string to integer.

static inline bool parse_hex(int *dest, const char *src) {
  char *endptr = NULL;
  long val;

  errno = 0;
  val = strtol(src, &endptr, 16);

  if (errno == ERANGE || *endptr != '\0')
    return false;

  if (val < INT_MIN || val > INT_MAX)
    return false;

  *dest = (int)val;
  return true;
}

// Implement handlers for xini_user_color

// Parse handler: Takes "#RRGGBB" string as input and parses each color
// component to correspoding uint8_t (r, g, b).

static inline bool xini_user_parse_color(xini_user_color *dest,
                                         const char *src) {
  int hex;

  if (strlen(src) != 7 || src[0] != '#')
    return false;

  if (!parse_hex(&hex, src + 1))
    return false;

  *dest = (xini_user_color){
      .r = (hex >> 16) & 0xff, .g = (hex >> 8) & 0xff, .b = hex & 0xff};

  return true;
}

// Print handler: Print the type back in "#RRGGBB" format .

static inline void xini_user_print_color(FILE *file, xini_user_color value) {

  int hex =
      ((uint32_t)value.r << 16) | ((uint32_t)value.g << 8) | (uint32_t)value.b;
  fprintf(file, "\"#%.6X\"", hex);
}

// Free handler: This type does not own any dynamically allocated resources,
// so nothing needs to be freed.

static inline void xini_user_free_color(xini_user_color *s) { (void)s; }

int main() {

  int message_counter = 0;
  xini_context ctx = xini_context_init("./example.ini", &message_counter);

  xini_config cfg;
  xini_config_init(&cfg);
  if (!xini_config_parse(&ctx, &cfg))
    return 1;

  // You can use xini_is_entry_parsed() and xini_is_section_parsed() for
  // enforcing mandatory entries / sections.

  if (!xini_is_entry_parsed(&cfg, user, first_name) ||
      !xini_is_entry_parsed(&cfg, user, last_name)) {
    printf("'first_name' or 'last_name' not found in 'user' section.\n");
    return 1;
  }

  if (!xini_is_section_parsed(&cfg, desktop)) {
    printf("'desktop' section not found in the config.\n");
    return 1;
  }

  // Use cfg.<section>.<key> to access parsed values.

  printf("\nWelcome %s %s!\n", cfg.user.first_name, cfg.user.last_name);
  printf("Your age is %d.\n", cfg.user.age);

  if (cfg.user.day_of_birth == DAY_SUNDAY)
    printf("You were born on Sunday.\n");
  else
    printf("You were not born on Sunday.\n");

  printf("You were born in %s half of the year.\n\n",
         cfg.user.month_of_birth <= MONTH_JUNE ? "first" : "second");

  // Boolean
  if (cfg.user.show_desktop_info) {

    printf("Your desktop info:\n");
    printf("Wallpaper path: '%s'\n", cfg.desktop.wallpaper_path);
    printf("Display resolution: %dx%d\n", cfg.desktop.width,
           cfg.desktop.height);
    printf("Window opacity: %.1f\n", cfg.desktop.opacity);
    printf("Background color components:\n");
    printf("    Red:   %3.d/255\n", cfg.desktop.bg_color.r);
    printf("    Green: %3.d/255\n", cfg.desktop.bg_color.g);
    printf("    Blue:  %3.d/255\n", cfg.desktop.bg_color.b);
    printf("Supports floating windows?: %s\n\n",
           cfg.desktop.window_manager_kind != WM_TILING ? "yes" : "no");
  }

  printf("Total messages read: %d\n", message_counter);
  printf("Saving config to 'backup.ini'\n");

  // Use xini_config_print() to dump the config to a file.
  // Note: Dynamic section is not included.

  FILE *f = fopen("backup.ini", "w");
  xini_config_print(f, &cfg);
  fclose(f);

  // Don't forget to free the xini_config after use.

  xini_config_free(&cfg);
  return 0;
}
