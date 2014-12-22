/* files */
#define BOOKMARKS_FILE	"bookmarks"
#define HISTORY_FILE	"history"
#define COOKIE_FILE		"cookies"

/* browser settings */
char *user_agent	=	NULL;
char *home_page		=	"https://duckduckgo.com";
int history_limit	=	0;

/* download settings */
char *download_dir	=	"~/Downloads";

/* appearance */
char *font					=	"monospace normal 9";
char *inputbar_bg_color		=	"#FFFFFF";
char *inputbar_fg_color		=	"#000000";
char *statusbar_bg_color	=	"#000000";
char *statusbar_fg_color	=	"#FFFFFF";

gboolean show_scrollbars	=	FALSE;
gboolean show_statusbar		=	TRUE;

/* shortcuts */
Shortcut shortcuts[] = {
	{ GDK_CONTROL_MASK,					GDK_w,		sc_close_window,	{ 0,			NULL } },
	{ GDK_CONTROL_MASK,					GDK_r,		sc_reload,			{ 0,			NULL } },
	{ GDK_CONTROL_MASK|GDK_SHIFT_MASK,	GDK_r,		sc_reload,			{ TRUE,			NULL } },
	{ GDK_CONTROL_MASK|GDK_SHIFT_MASK,	GDK_plus,	sc_zoom,			{ ZOOM_IN,		NULL } },
	{ GDK_CONTROL_MASK,					GDK_minus,	sc_zoom,			{ ZOOM_OUT,		NULL } },
	{ GDK_CONTROL_MASK,					GDK_0,		sc_zoom,			{ ZOOM_RESET,	NULL } },
};
