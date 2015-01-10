/* files */
#define BOOKMARKS_FILE	"bookmarks"
#define HISTORY_FILE	"history"
#define COOKIE_FILE		"cookies"
static char *ca_file =	"/etc/ssl/certs/ca-certificates.crt";

/* browser settings */
char *user_agent	=	NULL;
char *home_page		=	"https://duckduckgo.com";
int history_limit	=	0;
int strict_ssl		=	FALSE;

/* download settings */
char *download_dir	=	"~/Downloads";

/* appearance */
char *font						=	"monospace normal 9";
char *inputbar_bg_color			=	"#000000";
char *inputbar_fg_color			=	"#FFFFFF";
char *statusbar_bg_color		=	"#000000";
char *statusbar_fg_color		=	"#FFFFFF";
char *statusbar_ssl_bg_color	=	"#000000";
char *statusbar_ssl_fg_color	=	"#00FF00";
char *notification_e_bg_color	=	"#FF0000";
char *notification_e_fg_color	=	"#FFFFFF";

gboolean show_scrollbars	=	FALSE;
gboolean show_statusbar		=	TRUE;
gboolean private_browsing	=	FALSE;

/* shortcuts */
Shortcut shortcuts[] = {
	{ 0,								GDK_Escape,		sc_abort,				ALL,	{ 0,			NULL } },
	{ GDK_CONTROL_MASK,					GDK_w,			sc_close_window,		NORMAL,	{ 0,			NULL } },
	{ 0,								GDK_colon,		sc_focus_inputbar,		NORMAL,	{ 0,			":" } },
	{ GDK_CONTROL_MASK,					GDK_l,			sc_focus_inputbar,		NORMAL,	{ 0,			":" } },
	{ 0,								GDK_slash,		sc_focus_inputbar,		NORMAL,	{ 0,			"/" } },
	{ 0,								GDK_question,	sc_focus_inputbar,		NORMAL,	{ 0,			"?" } },
	{ 0,								GDK_o,			sc_focus_inputbar,		NORMAL,	{ 0,			":open " } },
	{ 0,								GDK_O,			sc_focus_inputbar,		NORMAL,	{ 0,			":winopen " } },
	{ GDK_CONTROL_MASK,					GDK_comma,		sc_nav_history,			NORMAL,	{ PREVIOUS,		NULL } },
	{ GDK_CONTROL_MASK,					GDK_period,		sc_nav_history,			NORMAL,	{ NEXT,			NULL } },
	{ GDK_CONTROL_MASK,					GDK_n,			sc_new_window,			NORMAL,	{ 0,			NULL } },
	{ GDK_CONTROL_MASK,					GDK_p,			sc_print,				NORMAL,	{ 0,			NULL } },
	{ GDK_CONTROL_MASK,					GDK_r,			sc_reload,				NORMAL,	{ 0,			NULL } },
	{ GDK_CONTROL_MASK|GDK_SHIFT_MASK,	GDK_r,			sc_reload,				NORMAL,	{ TRUE,			NULL } },
	{ 0,								GDK_n,			sc_search,				NORMAL,	{ NEXT,			NULL } },
	{ 0,								GDK_N,			sc_search,				NORMAL,	{ PREVIOUS,		NULL } },
	{ GDK_CONTROL_MASK,					GDK_m,			sc_toggle_statusbar,	NORMAL,	{ 0,			NULL } },
	{ GDK_CONTROL_MASK,					GDK_s,			sc_toggle_source,		NORMAL,	{ 0,			NULL } },
	{ 0,								GDK_plus,		sc_zoom,				NORMAL,	{ ZOOM_IN,		NULL } },
	{ 0,								GDK_minus,		sc_zoom,				NORMAL,	{ ZOOM_OUT,		NULL } },
	{ 0,								GDK_equal,		sc_zoom,				NORMAL,	{ ZOOM_RESET,	NULL } },
};

InputbarShortcut inputbar_shortcuts[] = {
	{ 0,				GDK_Escape,		isc_abort,					{ 0,			NULL } },
	{ 0,				GDK_Up,			isc_command_history,		{ PREVIOUS,		NULL } },
	{ 0,				GDK_Down,		isc_command_history,		{ NEXT,			NULL } },
	{ 0,				GDK_BackSpace,	isc_input_manipulation,		{ DELETE_CHAR,	NULL } },
};

/* commands */
Command commands[] = {
	{ "back",		0,		cmd_back },
	{ "bmark",		"b",	cmd_bookmark },
	{ "forward",	0,		cmd_forward },
	{ "open",		"o",	cmd_open },
	{ "print",		0,		cmd_print },
	{ "quit",		"q",	cmd_quit },
	{ "reload",		"r",	cmd_reload },
	{ "quitall",	0,		cmd_quitall },
	{ "winopen",	"W",	cmd_winopen },
};

/* special commands */
SpecialCommand special_commands[] = {
	{ '/',	scmd_search,	{ NEXT,		NULL } },
	{ '?',	scmd_search,	{ PREVIOUS,	NULL } },
};
