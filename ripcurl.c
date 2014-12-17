#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <webkit/webkit.h>

/* macros */
#define die(fmt, ...) { print_err(fmt, ##__VA_ARGS__); exit(EXIT_FAILURE); }

typedef struct _Ripcurl Ripcurl;
typedef struct _Browser Browser;

struct _Ripcurl {
	WebKitWebSettings *webkit_settings;
	SoupSession *soup_session;
	GList *browsers;
	GList *history;
};

struct _Browser {
	GtkWidget *window;
	GtkBox *box;
	WebKitWebView *view;
};

Ripcurl *ripcurl;

/* the name we were invoked as */
static char *progname;

/* utility functions */
void print_err(char *fmt, ...);
void *emalloc(size_t size);

/* callbacks */
void cb_destroy(GtkWidget * widget, Browser * b);
WebKitWebView *cb_create_web_view(WebKitWebView *v, WebKitWebFrame *f, Browser *b);
void cb_notify_load_status(WebKitWebView *view, GParamSpec *pspec, Browser *b);
void cb_notify_title(WebKitWebView *view, GParamSpec *pspec, Browser *b);

/* browser functions */
Browser *browser_new(void);
void browser_show(Browser * b);
void browser_load_uri(Browser * b, char *uri);
void browser_destroy(Browser * b);

void history_add(char *uri);
void history_write(void);
void ripcurl_init(void);
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

Browser *browser_new(void)
{
	Browser *b = emalloc(sizeof *b);

	b->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	b->box = GTK_BOX(gtk_vbox_new(FALSE, 0));
	b->view = WEBKIT_WEB_VIEW(webkit_web_view_new());

	/* window */
	gtk_window_set_title(GTK_WINDOW(b->window), "ripcurl");
	g_signal_connect(G_OBJECT(b->window), "destroy",
					 G_CALLBACK(cb_destroy), b);

	/* box */
	gtk_container_add(GTK_CONTAINER(b->window), GTK_WIDGET(b->box));

	/* view */
	gtk_box_pack_start(b->box, GTK_WIDGET(b->view), TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(b->view), "create-web-view", G_CALLBACK(cb_create_web_view), b);
	g_signal_connect(G_OBJECT(b->view), "notify::load-status", G_CALLBACK(cb_notify_load_status), b);
	g_signal_connect(G_OBJECT(b->view), "notify::title", G_CALLBACK(cb_notify_title), b);

	return b;
}

void browser_show(Browser * b)
{
	gtk_widget_show_all(b->window);
}

void browser_load_uri(Browser * b, char *uri)
{
	webkit_web_view_load_uri(b->view, uri);
}

void browser_destroy(Browser * b)
{
	webkit_web_view_stop_loading(b->view);
	/* destroy elements */
	gtk_widget_destroy(GTK_WIDGET(b->view));
	gtk_widget_destroy(GTK_WIDGET(b->box));
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
	if (!history_file) {
		return;
	}
}

void history_write(void)
{
	GString *history_string;
	GList *list;
	char *uri;
	int i;

	/* create string from history */
	history_string = g_string_new(NULL);

	for (list = ripcurl->history, i = 0; list && (!history_limit || i < history_limit); list = g_list_next(list), i++) {
		uri = g_strconcat((char *)list->data, "\n", NULL);
		history_string = g_string_prepend(history_string, uri);
		g_free(uri);
	}

	g_file_set_contents(history_file, history_string->str, -1, NULL);

	g_string_free(history_string, TRUE);
}

void history_write_alt(void)
{
	GList *list;
	FILE *fp;
	int i;

	if (!(fp = fopen(history_file, "w"))) {
		print_err("unable to open history file for writing\n");
		return;
	}

	for (list = g_list_last(ripcurl->history), i = 0; list; list = g_list_previous(list), i++) {
		if (history_limit && i < history_limit) {
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

	/* history list */
	ripcurl->history = NULL;
}

void load_data(void)
{
	SoupCookieJar *cookie_jar;

	/* load cookies */
	cookie_jar = soup_cookie_jar_text_new(cookie_file, FALSE);
	soup_session_add_feature(ripcurl->soup_session, SOUP_SESSION_FEATURE(cookie_jar));
}

void cleanup(void)
{
	GList *list;

	/* destroy any remaining browsers */
	while (ripcurl->browsers) {
		browser_destroy(ripcurl->browsers->data);
	}
	g_list_free(ripcurl->browsers);

	/* write history */
	//history_write();
	history_write_alt();

	/* clear history */
	for (list = ripcurl->history; list; list = g_list_next(list)) {
		free(list->data);
	}
	g_list_free(ripcurl->history);

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
