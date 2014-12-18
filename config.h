/* files */
static const char *history_file = "history";
static const char *cookie_file = "cookies";

/* browser settings */
char *home_page = "https://duckduckgo.com";
int history_limit = 0;

/* download settings */
char *download_dir = "~/Downloads";
char *download_command = "xterm -e /bin/sh -c \"curl -O '%s'" \
						  " --user-agent '%s' --referer '%s'" \
						  " --cookie cookies --cookie-jar cookies '%s';" \
						  " sleep 5;\"";
