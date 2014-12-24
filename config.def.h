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
gboolean private_browsing	=	FALSE;

/* shortcuts */
Shortcut shortcuts[] = {
	{ 0,								GDK_Escape,	sc_abort,				ALL,	{ 0,			NULL } },
	{ GDK_CONTROL_MASK,					GDK_w,		sc_close_window,		NORMAL,	{ 0,			NULL } },
	{ 0,								GDK_colon,	sc_focus_inputbar,		NORMAL,	{ 0,			":" } },
	{ 0,								GDK_o,		sc_focus_inputbar,		NORMAL,	{ 0,			":open " } },
	{ GDK_CONTROL_MASK,					GDK_r,		sc_reload,				NORMAL,	{ 0,			NULL } },
	{ GDK_CONTROL_MASK|GDK_SHIFT_MASK,	GDK_r,		sc_reload,				NORMAL,	{ TRUE,			NULL } },
	{ GDK_CONTROL_MASK,					GDK_m,		sc_toggle_statusbar,	NORMAL,	{ 0,			NULL } },
	{ GDK_CONTROL_MASK,					GDK_plus,	sc_zoom,				NORMAL,	{ ZOOM_IN,		NULL } },
	{ GDK_CONTROL_MASK,					GDK_minus,	sc_zoom,				NORMAL,	{ ZOOM_OUT,		NULL } },
	{ GDK_CONTROL_MASK,					GDK_0,		sc_zoom,				NORMAL,	{ ZOOM_RESET,	NULL } },
};

InputbarShortcut inputbar_shortcuts[] = {
	{ 0,								GDK_Escape,	isc_abort,					{ 0,			NULL } },
	{ 0,								GDK_Up,		isc_command_history,		{ PREVIOUS,		NULL } },
	{ 0,								GDK_Down,	isc_command_history,		{ NEXT,			NULL } },
};

/* commands */
Command commands[] = {
	{ "back",		0,		cmd_back },
	{ "forward",	0,		cmd_forward },
	{ "open",		"o",	cmd_open },
	{ "quit",		"q",	cmd_quit },
	{ "quitall",	0,		cmd_quitall },
};
