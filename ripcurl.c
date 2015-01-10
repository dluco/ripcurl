#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>

#include "utils.h"

/* macros */
#define LENGTH(x)		(sizeof x / sizeof x[0])
#define ALL_MASK		(GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK)

/* enums */
enum {
	DEFAULT,
	ERROR,
	WARNING,
	NEXT,
	PREVIOUS,
	ZOOM_IN,
	ZOOM_OUT,
	ZOOM_RESET,
	DELETE_CHAR,
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
typedef struct _Command Command;
typedef struct _SpecialCommand SpecialCommand;
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

struct _Command {
	char *name;
	char *abbrv;
	gboolean (*func)(Browser *b, int argc, char **argv);
};

struct _SpecialCommand {
	char identifier;
	gboolean (*func)(Browser *b, char *input, const Arg *arg, gboolean activate);
	const Arg arg;
};

struct _Ripcurl {
	struct {
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
		GdkColor statusbar_ssl_bg;
		GdkColor statusbar_ssl_fg;
		GdkColor notification_e_bg;
		GdkColor notification_e_fg;
		PangoFontDescription *font;
	} Style;
};

struct _Browser {
	struct {
		int mode;
		int progress;
		gboolean ssl;
	} State;

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
};

Ripcurl *ripcurl;

/* shortcut functions */
void sc_abort(Browser *b, const Arg *arg);
void sc_focus_inputbar(Browser *b, const Arg *arg);
void sc_close_window(Browser *b, const Arg *arg);
void sc_nav_history(Browser *b, const Arg *arg);
void sc_new_window(Browser *b, const Arg *arg);
void sc_print(Browser *b, const Arg *arg);
void sc_reload(Browser *b, const Arg *arg);
void sc_search(Browser *b, const Arg *arg);
void sc_toggle_statusbar(Browser *b, const Arg *arg);
void sc_toggle_source(Browser *b, const Arg *arg);
void sc_zoom(Browser *b, const Arg *arg);

/* inputbar shortcut functions */
void isc_abort(Browser *b, const Arg *arg);
void isc_command_history(Browser *b, const Arg *arg);
void isc_input_manipulation(Browser *b, const Arg *arg);

/* commands */
gboolean cmd_back(Browser *b, int argc, char **argv);
gboolean cmd_bookmark(Browser *b, int argc, char **argv);
gboolean cmd_forward(Browser *b, int argc, char **argv);
gboolean cmd_open(Browser *b, int argc, char **argv);
gboolean cmd_print(Browser *b, int argc, char **argv);
gboolean cmd_quit(Browser *b, int argc, char **argv);
gboolean cmd_reload(Browser *b, int argc, char **argv);
gboolean cmd_quitall(Browser *b, int argc, char **argv);
gboolean cmd_winopen(Browser *b, int argc, char **argv);

/* special commands */
gboolean scmd_search(Browser *b, char *input, const Arg *arg, gboolean activate);

/* callbacks */
void cb_win_destroy(GtkWidget *widget, Browser *b);
gboolean cb_wv_console_message(WebKitWebView *view, char *message, int line, char *source_id, Browser *b);
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
void cb_inputbar_changed(GtkEntry *entry, Browser *b);
void cb_inputbar_activate(GtkEntry *entry, Browser *b);

/* browser functions */
Browser *browser_new(void);
void browser_show(Browser * b);
void browser_apply_settings(Browser *b);
void browser_change_mode(Browser *b, int mode);
void browser_nav_history(Browser *b, int direction);
void browser_search_and_highlight(Browser *b, char *input, int direction);
void browser_update_search_highlight(Browser *b, char *search_text);
void browser_notify(Browser *b, int level, char *message);
void browser_load_uri(Browser * b, char *uri);
void browser_reload(Browser * b, int bypass);
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

	gtk_widget_grab_focus(GTK_WIDGET(b->UI.scrolled_window));
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
	browser_reload(b, arg->n);
}

void sc_nav_history(Browser *b, const Arg *arg)
{
	browser_nav_history(b, arg->n);
}

void sc_new_window(Browser *b, const Arg *arg)
{
	cmd_winopen(b, 0, NULL);
}

void sc_print(Browser *b, const Arg *arg)
{
	WebKitWebFrame *frame = webkit_web_view_get_main_frame(b->UI.view);

	if (!frame) {
		return;
	}

	webkit_web_frame_print(frame);
}

void sc_search(Browser *b, const Arg *arg)
{
	browser_search_and_highlight(b, NULL, arg->n);
}

void sc_toggle_statusbar(Browser *b, const Arg *arg)
{
	gtk_widget_set_visible(GTK_WIDGET(b->UI.statusbar),
			!gtk_widget_get_visible(GTK_WIDGET(b->UI.statusbar)));
}

void sc_toggle_source(Browser *b, const Arg *arg)
{
	webkit_web_view_set_view_source_mode(b->UI.view,
			!webkit_web_view_get_view_source_mode(b->UI.view));

	browser_reload(b, FALSE);
}

void sc_zoom(Browser *b, const Arg *arg)
{
	browser_zoom(b, arg->n);
}

void isc_abort(Browser *b, const Arg *arg)
{
	browser_notify(b, DEFAULT, "");
	gtk_widget_grab_focus(GTK_WIDGET(b->UI.scrolled_window));
	gtk_widget_hide(GTK_WIDGET(b->UI.inputbar));
}

void isc_command_history(Browser *b, const Arg *arg)
{
	static int current = 0;
	int len = g_list_length(ripcurl->Global.command_history);
	char *command;

	if (len > 0) {
		if (arg->n == NEXT) {
			current = (len + current + 1) % len;
		} else {
			current = (len + current - 1) % len;
		}

		command = g_list_nth_data(ripcurl->Global.command_history, current);
		browser_notify(b, DEFAULT, command);
		gtk_editable_set_position(GTK_EDITABLE(b->UI.inputbar), -1);
	}
}

void isc_input_manipulation(Browser *b, const Arg *arg)
{
	char *input;
	int length, pos;

	input = strdup(gtk_entry_get_text(b->UI.inputbar));
	length = strlen(input);
	pos = gtk_editable_get_position(GTK_EDITABLE(b->UI.inputbar));

	if (arg->n == DELETE_CHAR) {
		/* delete the previous character */
		if (length == 1) {
			/* deletion of last character hides inputbar */
			isc_abort(b, NULL);
		}

		gtk_editable_delete_text(GTK_EDITABLE(b->UI.inputbar), pos - 1, pos);
	}

	free(input);
}

gboolean cmd_back(Browser *b, int argc, char **argv)
{
	browser_nav_history(b, PREVIOUS);

	return TRUE;
}

gboolean cmd_bookmark(Browser *b, int argc, char **argv)
{
	GList *list;
	char *uri, *tags, *bookmark;

	uri = strdup(webkit_web_view_get_uri(b->UI.view));

	/* check if bookmark already exists in list */
	for (list = ripcurl->Global.bookmarks; list; list = g_list_next(list)) {
		if (!strcmp(uri, (char *) list->data)) {
			/* remove old bookmark so tags are updated */
			free(list->data);
			ripcurl->Global.bookmarks = g_list_delete_link(ripcurl->Global.bookmarks, list);
			break;
		}
	}

	/* append any tags to the bookmark string */
	/* NOTE: argv is null terminated */
	if (argc > 0) {
		tags = strjoinv(argv, " ");
		bookmark = strconcat(uri, " ", tags, NULL);

		free(tags);
	} else {
		bookmark = strdup(uri);
	}

	ripcurl->Global.bookmarks = g_list_prepend(ripcurl->Global.bookmarks, bookmark);

	free(uri);

	return TRUE;
}

gboolean cmd_forward(Browser *b, int argc, char **argv)
{
	browser_nav_history(b, NEXT);

	return TRUE;
}

gboolean cmd_open(Browser *b, int argc, char **argv)
{
	char *uri;

	if (argc <= 0 || argv[argc] != NULL) {
		return TRUE;
	}

	uri = strjoinv(argv, " ");
	browser_load_uri(b, uri);

	free(uri);

	return TRUE;
}

gboolean cmd_print(Browser *b, int argc, char **argv)
{
	sc_print(b, NULL);

	return TRUE;
}

gboolean cmd_quit(Browser *b, int argc, char **argv)
{
	browser_destroy(b);

	return TRUE;
}

gboolean cmd_quitall(Browser *b, int argc, char **argv)
{
	
	while (ripcurl->Global.browsers) {
		browser_destroy(ripcurl->Global.browsers->data);
	}

	return TRUE;
}

gboolean cmd_reload(Browser *b, int argc, char **argv)
{
	browser_reload(b, FALSE);

	return TRUE;
}

gboolean cmd_winopen(Browser *b, int argc, char **argv)
{
	Browser *n = browser_new();
	char *uri;

	uri = strjoinv(argv, " ");
	browser_load_uri(n, uri);

	free(uri);

	return TRUE;
}

gboolean scmd_search(Browser *b, char *input, const Arg *arg, gboolean activate)
{
	if (input && !strlen(input)) {
		return TRUE;
	}

	/* update highlighted matches */
	browser_update_search_highlight(b, input);

	/* only perform search if activated */
	if (activate) {
		browser_search_and_highlight(b, input, arg->n);
	}

	return TRUE;
}

void cb_win_destroy(GtkWidget * widget, Browser * b)
{
	browser_destroy(b);
}

gboolean cb_wv_console_message(WebKitWebView *view, char *message, int line, char *source_id, Browser *b)
{
	if (!strcmp(message, "hintmode_off") || !strcmp(message, "insertmode_off")) {
		browser_change_mode(b, NORMAL);
	} else if (!strcmp(message, "insertmode_on")) {
		browser_change_mode(b, INSERT);
	}

	return FALSE;
}

gboolean cb_wv_keypress(GtkWidget *widget, GdkEventKey *event, Browser *b)
{
	unsigned int keyval;
	GdkModifierType consumed_modifiers;
	int i;

	gdk_keymap_translate_keyboard_state(
			ripcurl->Global.keymap, event->hardware_keycode, event->state, event->group, /* in */
			&keyval, NULL, NULL, &consumed_modifiers);	/* out */

	for (i = 0; i < LENGTH(shortcuts); i++) {
		if (keyval == shortcuts[i].keyval
				&& (event->state & ~consumed_modifiers & ALL_MASK) == shortcuts[i].mask
				&& b->State.mode & shortcuts[i].mode
				&& shortcuts[i].func) {
			shortcuts[i].func(b, &(shortcuts[i].arg));
			return TRUE;
		}
	}

	return FALSE;
}

WebKitWebView *cb_wv_create_web_view(WebKitWebView *v, WebKitWebFrame *f, Browser *b)
{
	Browser *n = browser_new();

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
	WebKitWebFrame *frame;
	WebKitWebDataSource *source;
	WebKitNetworkRequest *request;
	SoupMessage *message;
	char *uri;

	switch (webkit_web_view_get_load_status(b->UI.view)) {
	case WEBKIT_LOAD_COMMITTED:
		frame = webkit_web_view_get_main_frame(b->UI.view);
		source = webkit_web_frame_get_data_source(frame);
		request = webkit_web_data_source_get_request(source);
		message = webkit_network_request_get_message(request);
		b->State.ssl = soup_message_get_flags(message)
			^ SOUP_MESSAGE_CERTIFICATE_TRUSTED;
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

void cb_inputbar_changed(GtkEntry *entry, Browser *b)
{
	char *input;
	char identifier;
	int i;

	input = strdup(gtk_entry_get_text(entry));
	identifier = input[0];

	/* special commands */
	for (i = 0; i < LENGTH(special_commands); i++) {
		if (identifier == special_commands[i].identifier) {
			special_commands[i].func(b, input + 1, &(special_commands[i].arg), FALSE);
			free(input);
			return;
		}
	}

	free(input);
}

void cb_inputbar_activate(GtkEntry *entry, Browser *b)
{
	char *input, **tokens, *command;
	char identifier;
	int i, n;
	gboolean ret = FALSE;
	gboolean processed = FALSE;
	GList *list;

	input = strdup(gtk_entry_get_text(entry));

	if (strlen(input) <= 1) {
		/* no input */
		free(input);
		/* hide inputbar */
		isc_abort(b, NULL);
		return;
	}

	identifier = input[0];

	/* special commands */
	for (i = 0; i < LENGTH(special_commands); i++) {
		if (identifier == special_commands[i].identifier) {
			ret = special_commands[i].func(b, input + 1, &(special_commands[i].arg), TRUE);

			gtk_widget_grab_focus(GTK_WIDGET(b->UI.scrolled_window));
			/* ret == TRUE: hide inputbar */
			if (ret) {
				isc_abort(b, NULL);
			}
			free(input);
			return;
		}
	}

	/* append input to command history */
	if (!private_browsing) {
		ripcurl->Global.command_history = g_list_append(ripcurl->Global.command_history, strdup(input));
	}

	/* tokenize input, skipping first char */
	tokens = tokenize(input + 1, " ");
	free(input);
	command = tokens[0];
	n = strlenv(tokens);

	/* search commands */
	for (i = 0; i < LENGTH(commands); i++) {
		if ((strcmp_s(command, commands[i].name) == 0)
				|| (strcmp_s(command, commands[i].abbrv) == 0)) {
			ret = commands[i].func(b, n - 1, tokens + 1);
			processed = TRUE;
			break;
		}
	}

	if (!processed) {
		browser_notify(b, ERROR, "Unknown command");
	}

	/* check if b was destroyed by a command */
	for (list = ripcurl->Global.browsers; list; list = g_list_next(list)) {
		if (list->data == b) {
			/* browser found - grab focus */
			gtk_widget_grab_focus(GTK_WIDGET(b->UI.scrolled_window));

			/* ret == TRUE: hide inputbar */
			if (ret) {
				isc_abort(b, NULL);
			}
		}
	}

	strfreev(tokens);
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

	g_signal_connect(G_OBJECT(b->UI.view), "console-message", G_CALLBACK(cb_wv_console_message), b);
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
	g_signal_connect(G_OBJECT(b->UI.inputbar), "changed", G_CALLBACK(cb_inputbar_changed), b);
	g_signal_connect(G_OBJECT(b->UI.inputbar), "activate", G_CALLBACK(cb_inputbar_activate), b);
	
	/* packing */
	gtk_box_pack_start(b->UI.box, GTK_WIDGET(b->UI.scrolled_window), TRUE, TRUE, 0);
	gtk_box_pack_start(b->UI.box, GTK_WIDGET(b->UI.statusbar), FALSE, FALSE, 0);
	gtk_box_pack_start(b->UI.box, GTK_WIDGET(b->UI.inputbar), FALSE, FALSE, 0);

	browser_apply_settings(b);
	browser_show(b);

	/* add to list of browsers */
	ripcurl->Global.browsers = g_list_prepend(ripcurl->Global.browsers, b);

	/* mode */
	b->State.mode = NORMAL;

	gtk_widget_grab_focus(GTK_WIDGET(b->UI.scrolled_window));

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

	b->State.mode = mode;
	browser_notify(b, DEFAULT, text);
}

void browser_nav_history(Browser *b, int direction)
{
	switch (direction) {
	case PREVIOUS:
		webkit_web_view_go_back(b->UI.view);
		break;
	case NEXT:
		webkit_web_view_go_forward(b->UI.view);
		break;
	default:
		break;
	}
}

void browser_search_and_highlight(Browser *b, char *input, int direction)
{
	static char *search_text = NULL;
	gboolean forward;

	if (input) {
		/* free search_text from previous search */
		if (search_text) {
			free(search_text);
		}
		search_text = strdup(input);
	}

	if (search_text == NULL) {
		/*
		 * the only way this should happen is if sc_search is called
		 * and no searches have been performed during this session.
		 */
		return;
	}

	forward = (direction == NEXT) ? TRUE : FALSE;

	/* TODO: set case-sensitivity with "/.../c" */
	webkit_web_view_search_text(b->UI.view, search_text, FALSE, forward, TRUE);
}

void browser_update_search_highlight(Browser *b, char *search_text)
{
	/* remove previous highlighting */
	webkit_web_view_unmark_text_matches(b->UI.view);
	/* highlight all occurrences of search text */
	webkit_web_view_mark_text_matches(b->UI.view, search_text, FALSE, 0);
	webkit_web_view_set_highlight_text_matches(b->UI.view, TRUE);
}

void browser_notify(Browser *b, int level, char *message)
{
	/* show inputbar */
	if (!gtk_widget_get_visible(GTK_WIDGET(b->UI.inputbar))) {
		gtk_widget_show(GTK_WIDGET(b->UI.inputbar));
	}

	/* set inputbar color */
	switch (level) {
	case ERROR:
		gtk_widget_modify_base(GTK_WIDGET(b->UI.inputbar), GTK_STATE_NORMAL, &(ripcurl->Style.notification_e_bg));
		gtk_widget_modify_text(GTK_WIDGET(b->UI.inputbar), GTK_STATE_NORMAL, &(ripcurl->Style.notification_e_fg));
		break;
	case WARNING:
		break;
	default:
		gtk_widget_modify_base(GTK_WIDGET(b->UI.inputbar), GTK_STATE_NORMAL, &(ripcurl->Style.inputbar_bg));
		gtk_widget_modify_text(GTK_WIDGET(b->UI.inputbar), GTK_STATE_NORMAL, &(ripcurl->Style.inputbar_fg));
		break;
	}

	/* display message, if any */
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

void browser_reload(Browser *b, int bypass)
{
	if (bypass == TRUE) {
		webkit_web_view_reload_bypass_cache(b->UI.view);
	} else {
		webkit_web_view_reload(b->UI.view);
	}
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
	char *text, *nav, *temp;
	GdkColor *bg, *fg;

	if (b->State.progress > 0 && b->State.progress < 100) {
		asprintf(&text, "Loading... %s (%d%%)", (uri) ? uri : "", b->State.progress);
	} else {
		text = strdup((uri) ? uri : "[No name]");
	}

	/* ssl */
	if (uri && strstr(uri, "https://") == uri) {
		bg = &(ripcurl->Style.statusbar_ssl_bg);
		fg = &(ripcurl->Style.statusbar_ssl_fg);
	} else {
		bg = &(ripcurl->Style.statusbar_bg);
		fg = &(ripcurl->Style.statusbar_fg);
	}

	/* check for navigation */
	if (uri) {
		nav = strdup("");

		if (webkit_web_view_can_go_back(b->UI.view)) {
			strappend(nav, "-");
		}
		if (webkit_web_view_can_go_forward(b->UI.view)) {
			strappend(nav, "+");
		}

		if (strlen(nav) > 0) {
			/* navigation possible */
			temp = strconcat(text, " [", nav, "]", NULL);
			free(text);
			text = temp;
		}

		free(nav);
	}

	/* apply statusbar colors */
	gtk_widget_modify_bg(GTK_WIDGET(b->UI.statusbar), GTK_STATE_NORMAL, bg);
	gtk_widget_modify_fg(GTK_WIDGET(b->UI.statusbar), GTK_STATE_NORMAL, fg);
	gtk_widget_modify_fg(GTK_WIDGET(b->Statusbar.text), GTK_STATE_NORMAL, fg);
	gtk_widget_modify_fg(GTK_WIDGET(b->Statusbar.buffer), GTK_STATE_NORMAL, fg);
	gtk_widget_modify_fg(GTK_WIDGET(b->Statusbar.position), GTK_STATE_NORMAL, fg);

	/* set text */
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
	link = g_list_find_custom(ripcurl->Global.history, uri, (GCompareFunc)strcmp_s);
	if (link) {
		/* uri is already present - move to front of list */
		ripcurl->Global.history = g_list_remove_link(ripcurl->Global.history, link);
		ripcurl->Global.history = g_list_concat(link, ripcurl->Global.history);
	} else {
		/* uri not present - prepend to list */
		ripcurl->Global.history = g_list_prepend(ripcurl->Global.history, strdup(uri));
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

	/* ssl */
	g_object_set(G_OBJECT(ripcurl->Global.soup_session), "ssl-ca-file", ca_file, NULL);
	g_object_set(G_OBJECT(ripcurl->Global.soup_session), "ssl-strict", strict_ssl, NULL);

	/* load bookmarks */
	ripcurl->Files.bookmarks_file = g_build_filename(ripcurl->Files.config_dir, BOOKMARKS_FILE, NULL);
	if (!ripcurl->Files.bookmarks_file) {
		print_err("error building bookmarks file path\n");
	} else {
		ripcurl->Global.bookmarks = read_file(ripcurl->Files.bookmarks_file, ripcurl->Global.bookmarks);
		ripcurl->Global.bookmarks= g_list_reverse(ripcurl->Global.bookmarks);
	}

	/* load history */
	ripcurl->Files.history_file = g_build_filename(ripcurl->Files.config_dir, HISTORY_FILE, NULL);
	if (!ripcurl->Files.history_file) {
		print_err("error building history file path\n");
	} else {
		ripcurl->Global.history = read_file(ripcurl->Files.history_file, ripcurl->Global.history);
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
	gdk_color_parse(statusbar_ssl_bg_color, &(ripcurl->Style.statusbar_ssl_bg));
	gdk_color_parse(statusbar_ssl_fg_color, &(ripcurl->Style.statusbar_ssl_fg));
	gdk_color_parse(notification_e_bg_color, &(ripcurl->Style.notification_e_bg));
	gdk_color_parse(notification_e_fg_color, &(ripcurl->Style.notification_e_fg));

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
