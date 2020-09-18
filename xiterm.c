#include <gtk/gtk.h>
#include <vte/vte.h>

#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

#define REGEX_URL "https?://[a-zA-Z0-9./_-]+"

GtkApplication *app;
GtkWidget *window;
GtkNotebook *notebook;
VteRegex *url_regex;
GdkRGBA palette[16];

char *cmd[2] = {"/bin/bash", NULL};
const char *colors[16] = {
	"#000", "#c00", "#591", "#b71", "#16c", "#96a", "#299", "#ccc",
	"#333", "#f33", "#7c0", "#ed0", "#6ad", "#c8b", "#0dd", "#fff",
};

gboolean match_key(GdkEventKey *event, int state, int keyval) {
	return event->state == state && event->keyval == keyval;
}

void update_show_tabs() {
	if (gtk_notebook_get_n_pages(notebook) > 1) {
		gtk_notebook_set_show_tabs(notebook, TRUE);
	} else {
		gtk_notebook_set_show_tabs(notebook, FALSE);
	}
}

gboolean on_term_click(VteTerminal *term, GdkEventButton *event, gpointer user_data) {
	GError *err = NULL;
	char *uri;

	if (event->button == 3) {
		uri = vte_terminal_match_check_event(term, (GdkEvent *)event, NULL);
		if (uri != NULL) {
			gtk_show_uri_on_window(GTK_WINDOW(window), uri, gtk_get_current_event_time(), &err);
			if (err != NULL) {
				fprintf(stderr, "Unable to open URI: %s\n", err->message);
				g_error_free(err);
			}
			g_free(uri);
			return TRUE;
		}
	}
	return FALSE;
}

void on_term_exit(VteTerminal *term, int status, gpointer user_data) {
	gtk_notebook_remove_page(notebook, gtk_notebook_page_num(notebook, GTK_WIDGET(term)));

	if (gtk_notebook_get_n_pages(notebook) == 0) {
		g_application_quit(G_APPLICATION(app));
	} else {
		update_show_tabs();
	}
}

VteTerminal *get_current_term(void) {
	int page_num;

	page_num = gtk_notebook_get_current_page(notebook);
	if (page_num == -1) {
		return NULL;
	} else {
		GtkWidget *page = gtk_notebook_get_nth_page(notebook, page_num);
		return VTE_TERMINAL(page);
	}
}

void setup_terminal(VteTerminal *term) {
	int tag;

	vte_terminal_spawn_async(term, VTE_PTY_DEFAULT, NULL, cmd, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, -1, NULL, NULL, NULL);
	vte_terminal_set_cursor_blink_mode(term, VTE_CURSOR_BLINK_OFF);
	tag = vte_terminal_match_add_regex(term, url_regex, 0);
	vte_terminal_match_set_cursor_name(term, tag, "pointer");
	vte_terminal_set_colors(term, &palette[15], NULL, palette, 16);
	vte_terminal_set_bold_is_bright(term, TRUE);

	g_signal_connect(term, "button-press-event", G_CALLBACK(on_term_click), NULL);
	g_signal_connect(term, "child-exited", G_CALLBACK(on_term_exit), NULL);
}

void add_tab(void) {
	GtkWidget *page;
	int page_num;

	page = vte_terminal_new();
	page_num = gtk_notebook_get_current_page(notebook) + 1;
	page_num = gtk_notebook_insert_page(notebook, page, NULL, page_num);
	gtk_notebook_set_tab_reorderable(notebook, page, TRUE);
	gtk_container_child_set(GTK_CONTAINER(notebook), page, "tab-expand", TRUE, NULL);

	update_show_tabs();
	gtk_widget_show(page);
	gtk_notebook_set_current_page(notebook, page_num);
	gtk_widget_grab_focus(page);

	setup_terminal(VTE_TERMINAL(page));
}

gboolean on_key(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	VteTerminal *term;

	if (match_key(event, GDK_CONTROL_MASK|GDK_SHIFT_MASK, GDK_KEY_T)) {
		add_tab();
	} else if (match_key(event, GDK_CONTROL_MASK, GDK_KEY_Page_Up)) {
		gtk_notebook_prev_page(notebook);
	} else if (match_key(event, GDK_CONTROL_MASK, GDK_KEY_Page_Down)) {
		gtk_notebook_next_page(notebook);
	} else if (match_key(event, GDK_CONTROL_MASK|GDK_SHIFT_MASK, GDK_KEY_C)) {
		term = get_current_term();
		vte_terminal_copy_clipboard_format(term, VTE_FORMAT_TEXT);
	} else if (match_key(event, GDK_CONTROL_MASK|GDK_SHIFT_MASK, GDK_KEY_V)) {
		term = get_current_term();
		vte_terminal_paste_clipboard(term);
	} else {
		return FALSE;
	}
	return TRUE;
}

void activate(GtkApplication* app, gpointer user_data) {
	GtkWidget *widget;

	window = gtk_application_window_new(app);
	gtk_window_set_default_icon_name("utilities-terminal");
	gtk_window_set_default_size(GTK_WINDOW(window), 620, 340);
	gtk_window_set_title(GTK_WINDOW(window), "XiTerm");
	g_signal_connect(window, "key-press-event", G_CALLBACK(on_key), NULL);

	widget = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(window), widget);
	notebook = GTK_NOTEBOOK(widget);

	gtk_notebook_set_show_border(notebook, FALSE);
	gtk_widget_show_all(window);

	add_tab();
}

int main(int argc, char **argv) {
	int i, status;
	GError *err = NULL;

	url_regex = vte_regex_new_for_match(REGEX_URL, -1, PCRE2_MULTILINE, &err);
	g_assert(err == NULL);

	for (i = 0; i < 16; i++) {
		gdk_rgba_parse(palette + i, colors[i]);
	}

	app = gtk_application_new("org.xi.xiterm", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	vte_regex_unref(url_regex);

	return status;
}
