#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <webkit/webkit.h>

/* macros */
#define die(fmt, ...) { print_err(fmt, ##__VA_ARGS__); exit(EXIT_FAILURE); }

#define MAXLINE 1024

typedef struct _Ripcurl Ripcurl;
typedef struct _Browser Browser;

struct _Ripcurl {
	WebKitWebSettings *webkit_settings;
	SoupSession *soup_session;
	GList *browsers;
	GList *bookmarks;
	GList *history;
	char *config_dir;
	char *bookmarks_file;
	char *history_file;
	char *cookie_file;
};

struct _Browser {
	GtkWidget *window;
	GtkBox *box;
	WebKitWebView *view;
	GtkEntry *inputbar;
};

Ripcurl *ripcurl;

/* the name we were invoked as */
static char *progname;

/* utility functions */
void print_err(char *fmt, ...);
void *emalloc(size_t size);
void chomp(char *str);

/* callbacks */
void cb_destroy(GtkWidget * widget, Browser * b);
WebKitWebView *cb_create_web_view(WebKitWebView *v, WebKitWebFrame *f, Browser *b);
void cb_notify_load_status(WebKitWebView *view, GParamSpec *pspec, Browser *b);
void cb_notify_title(WebKitWebView *view, GParamSpec *pspec, Browser *b);
gboolean cb_mime_type_decision(WebKitWebView *view, WebKitWebFrame *frame, WebKitNetworkRequest *request, char *mimetype, WebKitWebPolicyDecision *policy_decision, Browser *b);
gboolean cb_download_requested(WebKitWebView *view, WebKitDownload *download, Browser *b);
void cb_download_notify_status(WebKitDownload *download, GParamSpec *pspec, Browser *b);

void cb_inputbar_activate(GtkEntry *entry, Browser *b);

/* browser functions */
Browser *browser_new(void);
void browser_show(Browser * b);
void browser_load_uri(Browser * b, char *uri);
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

void cb_destroy(GtkWidget * widget, Browser * b)
{
	browser_destroy(b);
}

WebKitWebView *cb_create_web_view(WebKitWebView *v, WebKitWebFrame *f, Browser *b)
{
	Browser *n = browser_new();
	/* add to list of browsers */
	ripcurl->browsers = g_list_prepend(ripcurl->browsers, n);
	browser_show(n);

	return n->view;
}

gboolean cb_mime_type_decision(WebKitWebView *view, WebKitWebFrame *frame, WebKitNetworkRequest *request, char *mimetype, WebKitWebPolicyDecision *policy_decision, Browser *b)
{
	if (!webkit_web_view_can_show_mime_type(b->view, mimetype)) {
		webkit_web_policy_decision_download(policy_decision);
		return TRUE;
	}
	return FALSE;
}

gboolean cb_download_requested(WebKitWebView *view, WebKitDownload *download, Browser *b)
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

void cb_notify_load_status(WebKitWebView *view, GParamSpec *pspec, Browser *b)
{
	char *uri;

	switch (webkit_web_view_get_load_status(b->view)) {
	case WEBKIT_LOAD_COMMITTED:
		break;
	case WEBKIT_LOAD_FINISHED:
		/* add uri to history */
		if ((uri = (char *)webkit_web_view_get_uri(b->view))) {
			history_add(uri);
		}
		break;
	default:
		break;
	}
}

void cb_notify_title(WebKitWebView *view, GParamSpec *pspec, Browser *b)
{
	const char *title = webkit_web_view_get_title(b->view);
	if (title) {
		/* update state */
		gtk_window_set_title(GTK_WINDOW(b->window), title);
	}
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
	Browser *b = emalloc(sizeof *b);

	b->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	b->box = GTK_BOX(gtk_vbox_new(FALSE, 0));
	b->view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	b->inputbar = GTK_ENTRY(gtk_entry_new());

	/* window */
	gtk_window_set_title(GTK_WINDOW(b->window), "ripcurl");
	g_signal_connect(G_OBJECT(b->window), "destroy",
					 G_CALLBACK(cb_destroy), b);

	/* box */
	gtk_container_add(GTK_CONTAINER(b->window), GTK_WIDGET(b->box));

	/* view */
	g_signal_connect(G_OBJECT(b->view), "create-web-view", G_CALLBACK(cb_create_web_view), b);
	g_signal_connect(G_OBJECT(b->view), "mime-type-policy-decision-requested", G_CALLBACK(cb_mime_type_decision), b);
	g_signal_connect(G_OBJECT(b->view), "download-requested", G_CALLBACK(cb_download_requested), b);
	g_signal_connect(G_OBJECT(b->view), "notify::load-status", G_CALLBACK(cb_notify_load_status), b);
	g_signal_connect(G_OBJECT(b->view), "notify::title", G_CALLBACK(cb_notify_title), b);

	/* inputbar */
	gtk_entry_set_inner_border(b->inputbar, NULL);
	gtk_entry_set_has_frame(b->inputbar, FALSE);
	gtk_editable_set_editable(GTK_EDITABLE(b->inputbar), TRUE);

	g_signal_connect(G_OBJECT(b->inputbar), "activate", G_CALLBACK(cb_inputbar_activate), b);
	
	/* packing */
	gtk_box_pack_start(b->box, GTK_WIDGET(b->view), TRUE, TRUE, 0);
	gtk_box_pack_start(b->box, GTK_WIDGET(b->inputbar), FALSE, FALSE, 0);

	return b;
}

void browser_show(Browser * b)
{
	gtk_widget_show_all(b->window);
}

void browser_load_uri(Browser * b, char *uri)
{
	char *new_uri = NULL;

	if (!uri) {
		new_uri = strdup(home_page);
	} else {
		new_uri = strdup(uri);
	}

	webkit_web_view_load_uri(b->view, new_uri);

	free(new_uri);
}

void browser_destroy(Browser * b)
{
	webkit_web_view_stop_loading(b->view);
	/* destroy elements */
	gtk_widget_destroy(b->window);

	/* remove from list of browsers */
	ripcurl->browsers = g_list_remove(ripcurl->browsers, b);
	/* free data */
	free(b);

	/* quit if no windows left */
	if (g_list_length(ripcurl->browsers) == 0) {
		gtk_main_quit();
	}
}

void bookmarks_read(void)
{
	FILE *fp;
	char *line;
	size_t nbytes = MAXLINE;

	if (!(fp = fopen(ripcurl->bookmarks_file, "r"))) {
		/* bookmarks file not found - one will be created on exit */
		return;
	}

	line = emalloc(nbytes * sizeof *line);

	while (getline(&line, &nbytes, fp) != -1) {
		chomp(line);
		if (strlen(line) == 0) {
			continue;
		}
		ripcurl->bookmarks = g_list_append(ripcurl->bookmarks, strdup(line));
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

	if (!(fp = fopen(ripcurl->bookmarks_file, "w"))) {
		print_err("unable to open bookmarks file for writing\n");
		return;
	}

	for (list = ripcurl->bookmarks; list; list = g_list_next(list)) {
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
	link = g_list_find_custom(ripcurl->history, uri, (GCompareFunc)strcmp);
	if (link) {
		/* uri is already present - move to front of list */
		ripcurl->history = g_list_remove_link(ripcurl->history, link);
		ripcurl->history = g_list_concat(link, ripcurl->history);
	} else {
		/* uri not present - prepend to list */
		ripcurl->history = g_list_prepend(ripcurl->history, strdup(uri));
	}
}

void history_read(void)
{
	FILE *fp;
	char *line;
	size_t nbytes = MAXLINE;

	if (!(fp = fopen(ripcurl->history_file, "r"))) {
		/* history file not found - one will be created on exit */
		return;
	}

	line = emalloc(nbytes * sizeof *line);

	while (getline(&line, &nbytes, fp) != -1) {
		chomp(line);
		if (strlen(line) == 0) {
			continue;
		}
		ripcurl->history = g_list_prepend(ripcurl->history, strdup(line));
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

	if (!(fp = fopen(ripcurl->history_file, "w"))) {
		print_err("unable to open history file for writing\n");
		return;
	}

	for (list = g_list_last(ripcurl->history), i = 0; list; list = g_list_previous(list), i++) {
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
	ripcurl->webkit_settings = webkit_web_settings_new();

	/* libsoup session */
	ripcurl->soup_session = webkit_get_default_session();

	/* browser list */
	ripcurl->browsers = NULL;

	/* bookmarks list */
	ripcurl->bookmarks = NULL;

	/* history list */
	ripcurl->history = NULL;

	/* create config dir */
	ripcurl->config_dir = g_build_filename(g_get_user_config_dir(), "ripcurl", NULL);
	g_mkdir_with_parents(ripcurl->config_dir, 0771);
}

void load_data(void)
{
	SoupCookieJar *cookie_jar;

	/* load cookies */
	ripcurl->cookie_file = g_build_filename(ripcurl->config_dir, COOKIE_FILE, NULL);
	if (!ripcurl->cookie_file) {
		print_err("error building cookie file path\n");
	} else {
		cookie_jar = soup_cookie_jar_text_new(ripcurl->cookie_file, FALSE);
		soup_session_add_feature(ripcurl->soup_session, SOUP_SESSION_FEATURE(cookie_jar));
	}

	/* load bookmarks */
	ripcurl->bookmarks_file = g_build_filename(ripcurl->config_dir, BOOKMARKS_FILE, NULL);
	if (!ripcurl->bookmarks_file) {
		print_err("error building bookmarks file path\n");
	} else {
		bookmarks_read();
	}

	/* load history */
	ripcurl->history_file = g_build_filename(ripcurl->config_dir, HISTORY_FILE, NULL);
	if (!ripcurl->history_file) {
		print_err("error building history file path\n");
	} else {
		history_read();
	}
}

void ripcurl_settings(void)
{
	if (user_agent) {
		g_object_set(G_OBJECT(ripcurl->webkit_settings), "user-agent", user_agent, NULL);
	}
}

void cleanup(void)
{
	GList *list;

	/* destroy any remaining browsers */
	while (ripcurl->browsers) {
		browser_destroy(ripcurl->browsers->data);
	}
	g_list_free(ripcurl->browsers);

	/* free cookie file */
	g_free(ripcurl->cookie_file);

	/* write bookmarks */
	if (ripcurl->bookmarks_file) {
		bookmarks_write();
	}

	/* clear bookmarks */
	for (list = ripcurl->bookmarks; list; list = g_list_next(list)) {
		free(list->data);
	}
	g_list_free(ripcurl->bookmarks);
	g_free(ripcurl->bookmarks_file);

	/* write history */
	if (ripcurl->history_file) {
		history_write();
	}

	/* clear history */
	for (list = ripcurl->history; list; list = g_list_next(list)) {
		free(list->data);
	}
	g_list_free(ripcurl->history);
	g_free(ripcurl->history_file);

	/* free config dir file */
	g_free(ripcurl->config_dir);

	free(ripcurl);
}

int main(int argc, char *argv[])
{
	Browser *b;

	progname = argv[0];

	gtk_init(&argc, &argv);

	/* init toplevel struct */
	ripcurl = emalloc(sizeof *ripcurl);
	ripcurl_init();
	ripcurl_settings();
	
	load_data();

	/* init first browser window */
	b = browser_new();
	/* add to list of browsers */
	ripcurl->browsers = g_list_prepend(ripcurl->browsers, b);

	if (argc > 1) {
		browser_load_uri(b, argv[1]);
	} else {
		browser_load_uri(b, home_page);
	}

	browser_show(b);

	/* start GTK+ main loop */
	gtk_main();

	cleanup();

	return 0;
}
