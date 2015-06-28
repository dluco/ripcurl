/* files */
static char *config_dir		=	"~/.config/ripcurl";
static char *bookmarks_file	=	"bookmarks";
static char *history_file	=	"history";
static char *cookie_file	=	"cookies";
static char *ca_file 		=	"/etc/ssl/certs/ca-certificates.crt";

/* browser settings */
char *user_agent			=	NULL;
char *home_page				=	"https://duckduckgo.com";
int history_limit			=	0;
gboolean strict_ssl			=	FALSE;
gboolean private_browsing	=	FALSE;
gboolean developer_extras	=	TRUE;

/* download settings */
char *download_dir	=	"~/Downloads";

/* appearance */
char *font								=	"monospace normal 9";
char *inputbar_bg_color					=	"#000000";
char *inputbar_fg_color					=	"#FFFFFF";
char *statusbar_bg_color				=	"#000000";
char *statusbar_fg_color				=	"#FFFFFF";
char *statusbar_ssl_trust_bg_color		=	"#000000";
char *statusbar_ssl_trust_fg_color		=	"#9FBC00";
char *statusbar_ssl_untrust_bg_color	=	"#000000";
char *statusbar_ssl_untrust_fg_color	=	"#FF0000";
char *notification_e_bg_color			=	"#FF0000";
char *notification_e_fg_color			=	"#FFFFFF";

gboolean show_scrollbars	=	FALSE;
gboolean show_statusbar		=	TRUE;

/* shortcuts */
Shortcut shortcuts[] = {
	{ 0,								GDK_Escape,		sc_abort,				ALL,	{ 0,			NULL } },
	{ GDK_CONTROL_MASK,					GDK_w,			sc_close_window,		NORMAL,	{ 0,			NULL } },
	{ 0,								GDK_colon,		sc_focus_inputbar,		NORMAL,	{ 0,			":" } },
	{ GDK_CONTROL_MASK,					GDK_l,			sc_focus_inputbar,		NORMAL,	{ 0,			":" } },
	{ 0,								GDK_slash,		sc_focus_inputbar,		NORMAL,	{ 0,			"/" } },
	{ 0,								GDK_question,	sc_focus_inputbar,		NORMAL,	{ 0,			"?" } },
	{ 0,								GDK_o,			sc_focus_inputbar,		NORMAL,	{ 0,			":open " } },
	{ 0,								GDK_O,			sc_focus_inputbar,		NORMAL,	{ APPEND_URL,	":open " } },
	{ 0,								GDK_w,			sc_focus_inputbar,		NORMAL,	{ 0,			":winopen " } },
	{ 0,								GDK_W,			sc_focus_inputbar,		NORMAL,	{ APPEND_URL,	":winopen " } },
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
	{ "quitall",	"Q",	cmd_quitall },
	{ "reload",		"r",	cmd_reload },
	{ "winopen",	"W",	cmd_winopen },
};

/* special commands */
SpecialCommand special_commands[] = {
	{ '/',	scmd_search,	{ NEXT,		NULL } },
	{ '?',	scmd_search,	{ PREVIOUS,	NULL } },
};
