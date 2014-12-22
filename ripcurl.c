#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>

/* macros */
#define LENGTH(x)		(sizeof x / sizeof x[0])
#define die(fmt, ...)	{ print_err(fmt, ##__VA_ARGS__); exit(EXIT_FAILURE); }
#define ALL_MASK		(GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK)

#define MAXLINE 1024

/* enums */
enum {
	DEFAULT,
	NEXT,
	PREVIOUS,
	ZOOM_IN,
	ZOOM_OUT,
	ZOOM_RESET,
};

/* modes */
enum mode {
	NORMAL	=	1 << 0,
	INSERT	=	1 << 1,
	ALL		=	0x7fffffff,
};

typedef struct _Arg Arg;
typedef struct _Shortcut Shortcut;
typedef struct _InputbarShortcut InputbarShortcut;
typedef struct _Ripcurl Ripcurl;
typedef struct _Browser Browser;

struct _Arg {
	int n;
	void *data;
};

struct _Shortcut {
	int mask;
	int keyval;
	void (*func)(Browser *b, const Arg *arg);
	int mode;
	const Arg arg;
};

struct _InputbarShortcut {
	int mask;
	int keyval;
	void (*func)(Browser *b, const Arg *arg);
	const Arg arg;
};

struct _Ripcurl {
	struct {
		int mode;
		GList *browsers;
		GList *bookmarks;
		GList *history;
		GList *command_history;
		WebKitWebSettings *webkit_settings;
		SoupSession *soup_session;
		GdkKeymap *keymap;
	} Global;

	struct {
		char *config_dir;
		char *bookmarks_file;
		char *history_file;
		char *cookie_file;
	} Files;

	struct {
		GdkColor inputbar_bg;
		GdkColor inputbar_fg;
		GdkColor statusbar_bg;
		GdkColor statusbar_fg;
		PangoFontDescription *font;
	} Style;
};

struct _Browser {
	struct {
		GtkWidget *window;
		GtkBox *box;
		GtkScrolledWindow *scrolled_window;
		WebKitWebView *view;
		GtkWidget *statusbar;
		GtkBox *statusbar_entries;
		GtkEntry *inputbar;
	} UI;

	struct {
		GtkLabel *text;
		GtkLabel *buffer;
		GtkLabel *position;
	} Statusbar;

	struct {
		int progress;
	} State;
};

Ripcurl *ripcurl;

/* utility functions */
void print_err(char *fmt, ...);
void *emalloc(size_t size);
int asprintf(char **str, char *fmt, ...);
void chomp(char *str);

/* shortcut functions */
void sc_abort(Browser *b, const Arg *arg);
void sc_focus_inputbar(Browser *b, const Arg *arg);
void sc_close_window(Browser *b, const Arg *arg);
void sc_reload(Browser *b, const Arg *arg);
void sc_zoom(Browser *b, const Arg *arg);

/* inputbar shortcut functions */
void isc_abort(Browser *b, const Arg *arg);
void isc_command_history(Browser *b, const Arg *arg);

/* callbacks */
void cb_win_destroy(GtkWidget *widget, Browser *b);
gboolean cb_wv_keypress(GtkWidget *widget, GdkEventKey *event, Browser *b);
WebKitWebView *cb_wv_create_web_view(WebKitWebView *v, WebKitWebFrame *f, Browser *b);
void cb_wv_notify_load_status(WebKitWebView *view, GParamSpec *pspec, Browser *b);
void cb_wv_notify_progress(WebKitWebView *view, GParamSpec *pspec, Browser *b);
void cb_wv_notify_title(WebKitWebView *view, GParamSpec *pspec, Browser *b);
void cb_wv_hover_link(WebKitWebView *view, char *title, char *uri, Browser *b);
gboolean cb_wv_mime_type_decision(WebKitWebView *view, WebKitWebFrame *frame, WebKitNetworkRequest *request, char *mimetype, WebKitWebPolicyDecision *policy_decision, Browser *b);
gboolean cb_wv_download_requested(WebKitWebView *view, WebKitDownload *download, Browser *b);
void cb_download_notify_status(WebKitDownload *download, GParamSpec *pspec, Browser *b);
void cb_wv_scrolled(GtkAdjustment *adjustment, Browser *b);

gboolean cb_inputbar_keypress(GtkWidget *widget, GdkEventKey *event, Browser *b);
void cb_inputbar_activate(GtkEntry *entry, Browser *b);

/* browser functions */
Browser *browser_new(void);
void browser_show(Browser * b);
void browser_apply_settings(Browser *b);
void browser_change_mode(Browser *b, int mode);
void browser_notify(Browser *b, int level, char *message);
void browser_load_uri(Browser * b, char *uri);
void browser_zoom(Browser * b, int mode);
void browser_update_uri(Browser *b);
void browser_update_position(Browser *b);
void browser_update(Browser *b);
void browser_destroy(Browser * b);

/* bookmark functions */
void bookmarks_read(void);
void bookmarks_write(void);

/* history functions */
void history_add(char *uri);
void history_read(void);
void history_write(void);

/* init, cleanup, and data */
void ripcurl_init(void);
void ripcurl_settings(void);
void ripcurl_style(void);
void load_data(void);
void cleanup(void);

/* configuration */
#include "config.h"

void print_err(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void *emalloc(size_t size)
{
	void *p;

	p = malloc(size);
	if (!p) {
		die("malloc of %zu bytes failed\n", size);
	}
	return p;
}

int asprintf(char **str, char *fmt, ...)
{
	va_list argp;
	char one_char[1];
	int len;

	va_start(argp, fmt);
	len = vsnprintf(one_char, 1, fmt, argp);
	if (len < 1) {
		*str = NULL;
		return -1;
	}
	va_end(argp);

	*str = emalloc(len+1);
	if (!str) {
		return -1;
	}

	va_start(argp, fmt);
	vsnprintf(*str, len+1, fmt, argp);
	va_end(argp);

	return len;
}

void chomp(char *str)
{
	char *p;

	if (!str) {
		return;
	}

	for (p = &str[strlen(str)-1]; (*p == '\n' || *p == '\r'); p--) {
		*p = '\0';
	}
}

void sc_abort(Browser *b, const Arg *arg)
{
	/* stop loading website */
	webkit_web_view_stop_loading(b->UI.view);

	/* reset to NORMAL mode */
	browser_change_mode(b, NORMAL);

	/* hide inputbar */
	gtk_widget_hide(GTK_WIDGET(b->UI.inputbar));

	/* unmark search results */
	webkit_web_view_unmark_text_matches(b->UI.view);

	gtk_widget_grab_focus(GTK_WIDGET(b->UI.view));
}

void sc_focus_inputbar(Browser *b, const Arg *arg)
{
	char *clipboard_text;

	if (arg->data) {
		browser_notify(b, DEFAULT, arg->data);

		/* save primary selection - will be overwritten on grab_focus */
		clipboard_text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));

		gtk_widget_grab_focus(GTK_WIDGET(b->UI.inputbar));
		gtk_editable_set_position(GTK_EDITABLE(b->UI.inputbar), -1);

		if (clipboard_text) {
			/* restore primary selection */
			gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), clipboard_text, -1);
			free(clipboard_text);
		}
	} else if (!gtk_widget_is_focus(GTK_WIDGET(b->UI.inputbar))) {
		gtk_widget_grab_focus(GTK_WIDGET(b->UI.inputbar));
	}
	
	if (!gtk_widget_get_visible(GTK_WIDGET(b->UI.inputbar))) {
		gtk_widget_show(GTK_WIDGET(b->UI.inputbar));
	}
}

void sc_close_window(Browser *b, const Arg *arg)
{
	browser_destroy(b);
}

void sc_reload(Browser *b, const Arg *arg)
{
	if (arg->n == TRUE) {
		webkit_web_view_reload_bypass_cache(b->UI.view);
	} else {
		webkit_web_view_reload(b->UI.view);
	}
}

void sc_zoom(Browser *b, const Arg *arg)
{
	browser_zoom(b, arg->n);
}

void isc_abort(Browser *b, const Arg *arg)
{
	gtk_widget_grab_focus(GTK_WIDGET(b->UI.view));
	gtk_widget_hide(GTK_WIDGET(b->UI.inputbar));
}

void isc_command_history(Browser *b, const Arg *arg)
{
	int len = g_list_length(ripcurl->Global.command_history);
	int n;
	char *command;

	if (len > 0) {
		if (arg->n == NEXT) {
			n = (len + 1) % len;
		} else {
			n = (len - 1) % len;
		}

		printf("n=%d\n", n);

		command = g_list_nth_data(ripcurl->Global.command_history, n);
		browser_notify(b, DEFAULT, command);
		gtk_editable_set_position(GTK_EDITABLE(b->UI.inputbar), -1);
	}
}

void cb_win_destroy(GtkWidget * widget, Browser * b)
{
	browser_destroy(b);
}

gboolean cb_wv_keypress(GtkWidget *widget, GdkEventKey *event, Browser *b)
{
	unsigned int keyval;
	GdkModifierType consumed_modifiers;
	int i;
	gboolean processed = FALSE;

	gdk_keymap_translate_keyboard_state(
			ripcurl->Global.keymap, event->hardware_keycode, event->state, event->group, /* in */
			&keyval, NULL, NULL, &consumed_modifiers);	/* out */

	for (i = 0; i < LENGTH(shortcuts); i++) {
		if (keyval == shortcuts[i].keyval
				&& (event->state & ~consumed_modifiers & ALL_MASK) == shortcuts[i].mask
				&& ripcurl->Global.mode & shortcuts[i].mode
				&& shortcuts[i].func) {
			shortcuts[i].func(b, &(shortcuts[i].arg));
			processed = TRUE;
		}
	}

	return processed;
}

WebKitWebView *cb_wv_create_web_view(WebKitWebView *v, WebKitWebFrame *f, Browser *b)
{
	Browser *n = browser_new();
	/* add to list of browsers */
	ripcurl->Global.browsers = g_list_prepend(ripcurl->Global.browsers, n);

	return n->UI.view;
}

void cb_wv_hover_link(WebKitWebView *view, char *title, char *uri, Browser *b)
{
	const char *text;

	if (uri) {
		gtk_label_set_text(b->Statusbar.text, uri);
	} else if ((text = webkit_web_view_get_uri(b->UI.view))) {
		gtk_label_set_text(b->Statusbar.text, text);
	}
}

gboolean cb_wv_mime_type_decision(WebKitWebView *view, WebKitWebFrame *frame, WebKitNetworkRequest *request, char *mimetype, WebKitWebPolicyDecision *policy_decision, Browser *b)
{
	if (!webkit_web_view_can_show_mime_type(b->UI.view, mimetype)) {
		webkit_web_policy_decision_download(policy_decision);
		return TRUE;
	}
	return FALSE;
}

gboolean cb_wv_download_requested(WebKitWebView *view, WebKitDownload *download, Browser *b)
{
	const char *suggested_filename = webkit_download_get_suggested_filename(download);
	char *download_path = NULL;
	char *filename;

	/* build download dir if necessary */
	if (download_dir[0] == '~') {
		download_path = g_build_filename(g_get_home_dir(), download_dir + 1, NULL);
	} else {
		download_path = g_strdup(download_dir);
	}

	g_mkdir_with_parents(download_path, 0771);

	/* download file */
	filename = g_build_filename("file://", download_path, suggested_filename, NULL);

	webkit_download_set_destination_uri(download, filename);
	g_signal_connect(G_OBJECT(download), "notify::status", G_CALLBACK(cb_download_notify_status), b);

	g_free(download_path);
	g_free(filename);

	return TRUE;
}

void cb_download_notify_status(WebKitDownload *download, GParamSpec *pspec, Browser *b)
{
	const char *filename = webkit_download_get_destination_uri(download);;

	switch (webkit_download_get_status(download)) {
	case WEBKIT_DOWNLOAD_STATUS_STARTED:
		printf("download started: \"%s\"\n", filename);
		break;
	case WEBKIT_DOWNLOAD_STATUS_FINISHED:
		printf("download finished: \"%s\"\n", filename);
		break;
	case WEBKIT_DOWNLOAD_STATUS_ERROR:
		printf("download error: \"%s\"\n", filename);
		break;
	default:
		break;
	}
}

void cb_wv_notify_load_status(WebKitWebView *view, GParamSpec *pspec, Browser *b)
{
	char *uri;

	switch (webkit_web_view_get_load_status(b->UI.view)) {
	case WEBKIT_LOAD_COMMITTED:
		break;
	case WEBKIT_LOAD_FINISHED:
		/* add uri to history */
		if ((uri = (char *)webkit_web_view_get_uri(b->UI.view))) {
			history_add(uri);
		}
		b->State.progress = 100;
		break;
	default:
		break;
	}

	/* update browser (statusbar, position) */
	browser_update(b);
}

void cb_wv_notify_progress(WebKitWebView *view, GParamSpec *pspec, Browser *b)
{
	b->State.progress = webkit_web_view_get_progress(b->UI.view) * 100;
	browser_update_uri(b);
}	

void cb_wv_notify_title(WebKitWebView *view, GParamSpec *pspec, Browser *b)
{
	const char *title = webkit_web_view_get_title(b->UI.view);
	if (title) {
		/* update state */
		browser_update(b);
	}
}

void cb_wv_scrolled(GtkAdjustment *adjustment, Browser *b)
{
	browser_update_position(b);
}

gboolean cb_inputbar_keypress(GtkWidget *widget, GdkEventKey *event, Browser *b)
{
	unsigned int keyval;
	GdkModifierType consumed_modifiers;
	int i;
	gboolean processed = FALSE;

	gdk_keymap_translate_keyboard_state(
			ripcurl->Global.keymap, event->hardware_keycode, event->state, event->group, /* in */
			&keyval, NULL, NULL, &consumed_modifiers);	/* out */

	for (i = 0; i < LENGTH(inputbar_shortcuts); i++) {
		if (keyval == inputbar_shortcuts[i].keyval
				&& (event->state & ~consumed_modifiers & ALL_MASK) == inputbar_shortcuts[i].mask
				&& inputbar_shortcuts[i].func) {
			inputbar_shortcuts[i].func(b, &(inputbar_shortcuts[i].arg));
			processed = TRUE;
		}
	}

	return processed;
}

void cb_inputbar_activate(GtkEntry *entry, Browser *b)
{
	char *input = strdup(gtk_entry_get_text(entry));

	/* FIXME: search commands */

	browser_load_uri(b, input);

	free(input);
}

Browser *browser_new(void)
{
	GtkAdjustment *adjustment;

	Browser *b = emalloc(sizeof *b);

	b->UI.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	b->UI.box = GTK_BOX(gtk_vbox_new(FALSE, 0));
	b->UI.scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
	b->UI.view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	b->UI.statusbar = gtk_event_box_new();
	b->UI.statusbar_entries = GTK_BOX(gtk_hbox_new(FALSE, 0));
	b->UI.inputbar = GTK_ENTRY(gtk_entry_new());

	/* window */
	gtk_window_set_title(GTK_WINDOW(b->UI.window), "ripcurl");
	g_signal_connect(G_OBJECT(b->UI.window), "destroy", G_CALLBACK(cb_win_destroy), b);

	/* box */
	gtk_container_add(GTK_CONTAINER(b->UI.window), GTK_WIDGET(b->UI.box));

	/* view */
	adjustment = gtk_scrolled_window_get_vadjustment(b->UI.scrolled_window);

	g_signal_connect(G_OBJECT(b->UI.view), "create-web-view", G_CALLBACK(cb_wv_create_web_view), b);
	g_signal_connect(G_OBJECT(b->UI.view), "hovering-over-link", G_CALLBACK(cb_wv_hover_link), b);
	g_signal_connect(G_OBJECT(b->UI.view), "mime-type-policy-decision-requested", G_CALLBACK(cb_wv_mime_type_decision), b);
	g_signal_connect(G_OBJECT(b->UI.view), "download-requested", G_CALLBACK(cb_wv_download_requested), b);
	g_signal_connect(G_OBJECT(b->UI.view), "notify::load-status", G_CALLBACK(cb_wv_notify_load_status), b);
	g_signal_connect(G_OBJECT(b->UI.view), "notify::progress", G_CALLBACK(cb_wv_notify_progress), b);
	g_signal_connect(G_OBJECT(b->UI.view), "notify::title", G_CALLBACK(cb_wv_notify_title), b);
	g_signal_connect(G_OBJECT(adjustment), "value-changed", G_CALLBACK(cb_wv_scrolled), b);

	g_signal_connect(G_OBJECT(b->UI.scrolled_window), "key-press-event", G_CALLBACK(cb_wv_keypress), b);

	gtk_container_add(GTK_CONTAINER(b->UI.scrolled_window), GTK_WIDGET(b->UI.view));

	/* statusbar */
	b->Statusbar.text = GTK_LABEL(gtk_label_new(NULL));
	b->Statusbar.buffer = GTK_LABEL(gtk_label_new(NULL));
	b->Statusbar.position = GTK_LABEL(gtk_label_new(NULL));

	gtk_box_pack_start(b->UI.statusbar_entries, GTK_WIDGET(b->Statusbar.text), TRUE, TRUE, 0);
	gtk_box_pack_start(b->UI.statusbar_entries, GTK_WIDGET(b->Statusbar.buffer), FALSE, FALSE, 0);
	gtk_box_pack_start(b->UI.statusbar_entries, GTK_WIDGET(b->Statusbar.position), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(b->UI.statusbar), GTK_WIDGET(b->UI.statusbar_entries));

	/* inputbar */
	g_signal_connect(G_OBJECT(b->UI.inputbar), "key-press-event", G_CALLBACK(cb_inputbar_keypress), b);
	g_signal_connect(G_OBJECT(b->UI.inputbar), "activate", G_CALLBACK(cb_inputbar_activate), b);
	
	/* packing */
	gtk_box_pack_start(b->UI.box, GTK_WIDGET(b->UI.scrolled_window), TRUE, TRUE, 0);
	gtk_box_pack_start(b->UI.box, GTK_WIDGET(b->UI.statusbar), FALSE, FALSE, 0);
	gtk_box_pack_start(b->UI.box, GTK_WIDGET(b->UI.inputbar), FALSE, FALSE, 0);

	browser_apply_settings(b);
	browser_show(b);

	return b;
}

void browser_show(Browser * b)
{
	gtk_widget_show_all(b->UI.window);
	gtk_widget_hide(GTK_WIDGET(b->UI.inputbar));

	if (!show_statusbar) {
		gtk_widget_hide(GTK_WIDGET(b->UI.statusbar));
	}
}

void browser_apply_settings(Browser *b)
{
	WebKitWebFrame *frame;

	/* view */
	if (show_scrollbars) {
		gtk_scrolled_window_set_policy(b->UI.scrolled_window, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	} else {
		frame = webkit_web_view_get_main_frame(b->UI.view);
		g_signal_connect(G_OBJECT(frame), "scrollbars-policy-changed", G_CALLBACK(gtk_true), NULL);

		gtk_scrolled_window_set_policy(b->UI.scrolled_window, GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	}

	/* apply browser settings */
	webkit_web_view_set_settings(b->UI.view, ripcurl->Global.webkit_settings);

	/* statusbar */
	gtk_misc_set_alignment(GTK_MISC(b->Statusbar.text), 0.0, 0.0);
	gtk_misc_set_alignment(GTK_MISC(b->Statusbar.buffer), 1.0, 0.0);
	gtk_misc_set_alignment(GTK_MISC(b->Statusbar.position), 1.0, 0.0);

	gtk_misc_set_padding(GTK_MISC(b->Statusbar.text), 1.0, 2.0);
	gtk_misc_set_padding(GTK_MISC(b->Statusbar.buffer), 1.0, 2.0);
	gtk_misc_set_padding(GTK_MISC(b->Statusbar.position), 1.0, 2.0);

	gtk_widget_modify_bg(GTK_WIDGET(b->UI.statusbar), GTK_STATE_NORMAL, &(ripcurl->Style.statusbar_bg));
	gtk_widget_modify_fg(GTK_WIDGET(b->Statusbar.text), GTK_STATE_NORMAL, &(ripcurl->Style.statusbar_fg));
	gtk_widget_modify_fg(GTK_WIDGET(b->Statusbar.buffer), GTK_STATE_NORMAL, &(ripcurl->Style.statusbar_fg));
	gtk_widget_modify_fg(GTK_WIDGET(b->Statusbar.position), GTK_STATE_NORMAL, &(ripcurl->Style.statusbar_fg));

	gtk_widget_modify_font(GTK_WIDGET(b->Statusbar.text), ripcurl->Style.font);
	gtk_widget_modify_font(GTK_WIDGET(b->Statusbar.buffer), ripcurl->Style.font);
	gtk_widget_modify_font(GTK_WIDGET(b->Statusbar.position), ripcurl->Style.font);

	/* inputbar settings */
	gtk_entry_set_inner_border(b->UI.inputbar, NULL);
	gtk_entry_set_has_frame(b->UI.inputbar, FALSE);
	gtk_editable_set_editable(GTK_EDITABLE(b->UI.inputbar), TRUE);

	gtk_widget_modify_base(GTK_WIDGET(b->UI.inputbar), GTK_STATE_NORMAL, &(ripcurl->Style.inputbar_bg));
	gtk_widget_modify_text(GTK_WIDGET(b->UI.inputbar), GTK_STATE_NORMAL, &(ripcurl->Style.inputbar_fg));
	gtk_widget_modify_font(GTK_WIDGET(b->UI.inputbar), ripcurl->Style.font);

}

void browser_change_mode(Browser *b, int mode)
{
	char *text = NULL;

	switch (mode) {
	default:
		mode = NORMAL;
	}

	ripcurl->Global.mode = mode;
	browser_notify(b, DEFAULT, text);
}

void browser_notify(Browser *b, int level, char *message)
{
	if (!gtk_widget_get_visible(GTK_WIDGET(b->UI.inputbar))) {
		gtk_widget_show(GTK_WIDGET(b->UI.inputbar));
	}

	if (message) {
		gtk_entry_set_text(b->UI.inputbar, message);
	}
}

void browser_load_uri(Browser * b, char *uri)
{
	char *new_uri = NULL;

	if (!uri) {
		new_uri = strdup(home_page);
	} else {
		new_uri = strdup(uri);
	}

	webkit_web_view_load_uri(b->UI.view, new_uri);

	free(new_uri);
}

void browser_zoom(Browser *b, int mode)
{
	switch (mode) {
	case ZOOM_IN:
		webkit_web_view_zoom_in(b->UI.view);
		break;
	case ZOOM_OUT:
		webkit_web_view_zoom_out(b->UI.view);
		break;
	default:
		webkit_web_view_set_zoom_level(b->UI.view, 1.0);
		break;
	}
}


void browser_update_uri(Browser *b)
{
	const char *uri = webkit_web_view_get_uri(b->UI.view);
	char *text;

	if (b->State.progress > 0 && b->State.progress < 100) {
		asprintf(&text, "Loading... %s (%d%%)", (uri) ? uri : "", b->State.progress);
	} else {
		text = strdup((uri) ? uri : "[No name]");
	}

	gtk_label_set_text(b->Statusbar.text, text);

	free(text);
}

void browser_update_position(Browser *b)
{
	GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(b->UI.scrolled_window);
	double view_size = gtk_adjustment_get_page_size(adjustment);
	double value = gtk_adjustment_get_value(adjustment);
	double max = gtk_adjustment_get_upper(adjustment) - view_size;
	char *position;

	if (max == 0) {
		position = strdup("All");
	} else if (value == max) {
		position = strdup("Bot");
	} else if (value == 0) {
		position = strdup("Top");
	} else {
		asprintf(&position, "%2d%%", (int) ceil((value / max) * 100));
	}

	gtk_label_set_text(b->Statusbar.position, position);

	free(position);
}

void browser_update(Browser *b)
{
	const char *title;

	/* update title */
	title = webkit_web_view_get_title(b->UI.view);
	gtk_window_set_title(GTK_WINDOW(b->UI.window), (title) ? title : "ripcurl");

	browser_update_uri(b);
	browser_update_position(b);
}

void browser_destroy(Browser * b)
{
	webkit_web_view_stop_loading(b->UI.view);
	/* block signal handler for b->UI.window:"destroy" - prevents infinite loop */
	g_signal_handlers_block_by_func(G_OBJECT(b->UI.window), G_CALLBACK(cb_win_destroy), b);
	/* destroy elements */
	gtk_widget_destroy(b->UI.window);

	/* remove from list of browsers */
	ripcurl->Global.browsers = g_list_remove(ripcurl->Global.browsers, b);
	/* free data */
	free(b);

	/* quit if no windows left */
	if (g_list_length(ripcurl->Global.browsers) == 0) {
		gtk_main_quit();
	}
}

void bookmarks_read(void)
{
	FILE *fp;
	char *line;
	size_t nbytes = MAXLINE;

	if (!(fp = fopen(ripcurl->Files.bookmarks_file, "r"))) {
		/* bookmarks file not found - one will be created on exit */
		return;
	}

	line = emalloc(nbytes * sizeof *line);

	while (getline(&line, &nbytes, fp) != -1) {
		chomp(line);
		if (strlen(line) == 0) {
			continue;
		}
		ripcurl->Global.bookmarks = g_list_append(ripcurl->Global.bookmarks, strdup(line));
	}

	if (line) {
		free(line);
	}

	if (fclose(fp)) {
		print_err("unable to close bookmarks file\n");
	}
}

void bookmarks_write(void)
{
	GList *list;
	FILE *fp;

	if (!(fp = fopen(ripcurl->Files.bookmarks_file, "w"))) {
		print_err("unable to open bookmarks file for writing\n");
		return;
	}

	for (list = ripcurl->Global.bookmarks; list; list = g_list_next(list)) {
		fprintf(fp, "%s\n", (char *)list->data);
	}

	if (fclose(fp)) {
		print_err("unable to close bookmarks file\n");
	}
}

void history_add(char *uri)
{
	GList *link;

	if (!uri) {
		return;
	}

	/* check if uri is already in history */
	link = g_list_find_custom(ripcurl->Global.history, uri, (GCompareFunc)strcmp);
	if (link) {
		/* uri is already present - move to front of list */
		ripcurl->Global.history = g_list_remove_link(ripcurl->Global.history, link);
		ripcurl->Global.history = g_list_concat(link, ripcurl->Global.history);
	} else {
		/* uri not present - prepend to list */
		ripcurl->Global.history = g_list_prepend(ripcurl->Global.history, strdup(uri));
	}
}

void history_read(void)
{
	FILE *fp;
	char *line;
	size_t nbytes = MAXLINE;

	if (!(fp = fopen(ripcurl->Files.history_file, "r"))) {
		/* history file not found - one will be created on exit */
		return;
	}

	line = emalloc(nbytes * sizeof *line);

	while (getline(&line, &nbytes, fp) != -1) {
		chomp(line);
		if (strlen(line) == 0) {
			continue;
		}
		ripcurl->Global.history = g_list_prepend(ripcurl->Global.history, strdup(line));
	}

	if (line) {
		free(line);
	}

	if (fclose(fp)) {
		print_err("unable to close history file\n");
	}
}

void history_write(void)
{
	GList *list;
	FILE *fp;
	int i;

	if (!(fp = fopen(ripcurl->Files.history_file, "w"))) {
		print_err("unable to open history file for writing\n");
		return;
	}

	for (list = g_list_last(ripcurl->Global.history), i = 0; list; list = g_list_previous(list), i++) {
		if (history_limit && i >= history_limit) {
			/* history limit reached - stop */
			break;
		}
		fprintf(fp, "%s\n", (char *)list->data);
	}

	if (fclose(fp)) {
		print_err("unable to close history file\n");
	}
}

void ripcurl_init(void)
{
	/* mode */
	ripcurl->Global.mode = NORMAL;

	/* webkit settings */
	ripcurl->Global.webkit_settings = webkit_web_settings_new();

	/* libsoup session */
	ripcurl->Global.soup_session = webkit_get_default_session();

	/* browser list */
	ripcurl->Global.browsers = NULL;

	/* bookmarks list */
	ripcurl->Global.bookmarks = NULL;

	/* history list */
	ripcurl->Global.history = NULL;

	/* command history list */
	ripcurl->Global.command_history = NULL;

	/* GDK keymap */
	ripcurl->Global.keymap = gdk_keymap_get_default();

	/* create config dir */
	ripcurl->Files.config_dir = g_build_filename(g_get_user_config_dir(), "ripcurl", NULL);
	g_mkdir_with_parents(ripcurl->Files.config_dir, 0771);
}

void load_data(void)
{
	SoupCookieJar *cookie_jar;

	/* load cookies */
	ripcurl->Files.cookie_file = g_build_filename(ripcurl->Files.config_dir, COOKIE_FILE, NULL);
	if (!ripcurl->Files.cookie_file) {
		print_err("error building cookie file path\n");
	} else {
		cookie_jar = soup_cookie_jar_text_new(ripcurl->Files.cookie_file, FALSE);
		soup_session_add_feature(ripcurl->Global.soup_session, SOUP_SESSION_FEATURE(cookie_jar));
	}

	/* load bookmarks */
	ripcurl->Files.bookmarks_file = g_build_filename(ripcurl->Files.config_dir, BOOKMARKS_FILE, NULL);
	if (!ripcurl->Files.bookmarks_file) {
		print_err("error building bookmarks file path\n");
	} else {
		bookmarks_read();
	}

	/* load history */
	ripcurl->Files.history_file = g_build_filename(ripcurl->Files.config_dir, HISTORY_FILE, NULL);
	if (!ripcurl->Files.history_file) {
		print_err("error building history file path\n");
	} else {
		history_read();
	}
}

void ripcurl_settings(void)
{
	if (user_agent) {
		g_object_set(G_OBJECT(ripcurl->Global.webkit_settings), "user-agent", user_agent, NULL);
	}
}

void ripcurl_style(void)
{
	/* parse colors */
	gdk_color_parse(inputbar_bg_color, &(ripcurl->Style.inputbar_bg));
	gdk_color_parse(inputbar_fg_color, &(ripcurl->Style.inputbar_fg));
	gdk_color_parse(statusbar_bg_color, &(ripcurl->Style.statusbar_bg));
	gdk_color_parse(statusbar_fg_color, &(ripcurl->Style.statusbar_fg));

	/* font */
	ripcurl->Style.font = pango_font_description_from_string(font);
}

void cleanup(void)
{
	GList *list;

	/* destroy any remaining browsers */
	while (ripcurl->Global.browsers) {
		browser_destroy(ripcurl->Global.browsers->data);
	}
	g_list_free(ripcurl->Global.browsers);

	/* free cookie file */
	g_free(ripcurl->Files.cookie_file);

	/* write bookmarks */
	if (ripcurl->Files.bookmarks_file) {
		bookmarks_write();
	}

	/* clear bookmarks */
	for (list = ripcurl->Global.bookmarks; list; list = g_list_next(list)) {
		free(list->data);
	}
	g_list_free(ripcurl->Global.bookmarks);
	g_free(ripcurl->Files.bookmarks_file);

	/* write history */
	if (ripcurl->Files.history_file) {
		history_write();
	}

	/* clear history */
	for (list = ripcurl->Global.history; list; list = g_list_next(list)) {
		free(list->data);
	}
	g_list_free(ripcurl->Global.history);
	g_free(ripcurl->Files.history_file);

	/* free config dir file */
	g_free(ripcurl->Files.config_dir);

	/* free font */
	pango_font_description_free(ripcurl->Style.font);

	free(ripcurl);
}

int main(int argc, char *argv[])
{
	Browser *b;

	gtk_init(&argc, &argv);

	/* init toplevel struct */
	ripcurl = emalloc(sizeof *ripcurl);
	ripcurl_init();
	ripcurl_settings();
	ripcurl_style();
	
	load_data();

	/* init first browser window */
	b = browser_new();
	/* add to list of browsers */
	ripcurl->Global.browsers = g_list_prepend(ripcurl->Global.browsers, b);

	if (argc > 1) {
		browser_load_uri(b, argv[1]);
	} else {
		browser_load_uri(b, home_page);
	}

	/* start GTK+ main loop */
	gtk_main();

	cleanup();

	return 0;
}
