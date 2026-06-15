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

// Schema

#define XINI_SECTIONS                                                          \
  XINI_STATIC_SECTION(user, USER_ENTRIES)                                      \
  XINI_STATIC_SECTION(desktop, DESKTOP_ENTRIES)                                \
  XINI_DYNAMIC_SECTION(messages)

#define USER_ENTRIES(S)                                                        \
  XINI_ENTRY(S, xini_str, first_name, "first")                                 \
  XINI_ENTRY(S, xini_str, last_name, "last")                                   \
  XINI_ENTRY(S, xini_int, age, 0)                                              \
  XINI_ENTRY(S, xini_enum_day, day_of_birth, DAY_SUNDAY)                       \
  XINI_ENTRY(S, xini_enum_month, month_of_birth, MONTH_JANUARY)

#define DESKTOP_ENTRIES(S)                                                     \
  XINI_ENTRY(S, xini_str, wallpaper_path, "~/wallpaper.png")                   \
  XINI_ENTRY(S, xini_enum_wm_kind, window_manager_kind, WM_FLOATING)           \
  XINI_ENTRY(S, xini_dbl, opacity, 1.0)                                        \
  XINI_ENTRY(S, xini_int, width, 800)                                          \
  XINI_ENTRY(S, xini_int, height, 600)

#define XINI_IMPLEMENTATION
#include "../xini.h"

bool xini_messages_entry_handler(xini_context *ctx) {

  int *count = ctx->userdata;
  (*count)++;
  printf("[messages-handler]: [%d] %s says '%s'\n", *count, ctx->entry.key,
         ctx->entry.value);

  return true;
}

int main() {

  int message_counter = 0;
  xini_context ctx = xini_context_init("./example.ini", &message_counter);

  xini_config cfg;
  xini_config_init(&cfg);
  if (!xini_config_parse(&ctx, &cfg))
    return 1;

  printf("\nWelcome %s %s!\n", cfg.user.first_name, cfg.user.last_name);
  printf("Your age is %d.\n", cfg.user.age);

  if (cfg.user.day_of_birth == DAY_SUNDAY)
    printf("You were born on Sunday.\n");
  else
    printf("You were not born on Sunday.\n");

  printf("You were born in %s half of the year.\n\n",
         cfg.user.month_of_birth <= MONTH_JUNE ? "first" : "second");

  printf("Your desktop info:\n");
  printf("Wallpaper path: '%s'\n", cfg.desktop.wallpaper_path);
  printf("Display resolution: %dx%d\n", cfg.desktop.width, cfg.desktop.height);
  printf("Window opacity: %.1f\n", cfg.desktop.opacity);
  printf("Supports floating windows?: %s\n\n",
         cfg.desktop.window_manager_kind != WM_TILING ? "yes" : "no");

  printf("Total messages read: %d\n", message_counter);
  printf("Saving config to 'backup.ini'\n");

  FILE *f = fopen("backup.ini", "w");
  xini_config_print(f, &cfg);
  fclose(f);

  xini_config_free(&cfg);
  return 0;
}
