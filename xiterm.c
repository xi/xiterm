#include <gtk/gtk.h>
#include <vte/vte.h>

#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

#define REGEX_URL "https?://[a-zA-Z0-9./_-]+"
#define KEY(v, s) (event->keyval == (v) && event->state == (GDK_CONTROL_MASK|(s)))

GtkApplication *app;
GtkWindow *window;
GtkNotebook *notebook;
VteRegex *url_regex;
GdkRGBA palette[16];
double font_scale = 1;

char *cmd[2] = {"/bin/bash", NULL};
const char *colors[16] = {
	"#000", "#c00", "#591", "#b71", "#16c", "#96a", "#299", "#ccc",
	"#333", "#f33", "#7c0", "#ed0", "#6ad", "#c8b", "#0dd", "#fff",
};

void set_font_scale(double value) {
	int i, n;
	GtkWidget *page;

	font_scale = value;

	n = gtk_notebook_get_n_pages(notebook);
	for (i = 0; i < n; i++) {
		page = gtk_notebook_get_nth_page(notebook, i);
		vte_terminal_set_font_scale(VTE_TERMINAL(page), font_scale);
	}
}

void update_show_tabs() {
	if (gtk_notebook_get_n_pages(notebook) > 1) {
		gtk_notebook_set_show_tabs(notebook, TRUE);
	} else {
		gtk_notebook_set_show_tabs(notebook, FALSE);
	}
}

void on_term_title(VteTerminal *term, gpointer user_data) {
	const char *title;

	title = vte_terminal_get_window_title(term);
	gtk_notebook_set_tab_label_text(notebook, GTK_WIDGET(term), title);
}

gboolean on_term_click(VteTerminal *term, GdkEventButton *event, gpointer user_data) {
	GError *err = NULL;
	char *uri;

	if (event->button == 3) {
		uri = vte_terminal_match_check_event(term, (GdkEvent *)event, NULL);
		if (uri != NULL) {
			gtk_show_uri_on_window(window, uri, gtk_get_current_event_time(), &err);
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

const char* get_cwd(VteTerminal *term) {
	// only works if /etc/profile.d/vte-2.91.sh was sourced in .bashrc
	const char *uri;

	if (term != NULL) {
		uri = vte_terminal_get_current_directory_uri(term);
		if (uri != NULL) {
			return g_filename_from_uri(uri, NULL, NULL);
		}
	}

	return NULL;
}

void setup_terminal(VteTerminal *term) {
	int tag;
	const char *cwd;

	cwd = get_cwd(get_current_term());

	vte_terminal_spawn_async(term, VTE_PTY_DEFAULT, cwd, cmd, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, -1, NULL, NULL, NULL);
	vte_terminal_set_cursor_blink_mode(term, VTE_CURSOR_BLINK_OFF);
	tag = vte_terminal_match_add_regex(term, url_regex, 0);
	vte_terminal_match_set_cursor_name(term, tag, "pointer");
	vte_terminal_set_colors(term, &palette[15], NULL, palette, 16);
	vte_terminal_set_bold_is_bright(term, TRUE);
	vte_terminal_set_font_scale(term, font_scale);

	g_signal_connect(term, "window-title-changed", G_CALLBACK(on_term_title), NULL);
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

	// needs to execute after gtk_widget_show() (for proper dimensions)
	// and before gtk_notebook_get_current_page() (so we can access the
	// previous term to get cwd)
	setup_terminal(VTE_TERMINAL(page));

	gtk_notebook_set_current_page(notebook, page_num);
	gtk_widget_grab_focus(page);
}

gboolean on_key(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	VteTerminal *term;

	if (!(event->state & GDK_CONTROL_MASK)) {
		return FALSE;
	} else if (KEY(GDK_KEY_T, GDK_SHIFT_MASK)) {
		add_tab();
	} else if (KEY(GDK_KEY_Page_Up, 0)) {
		gtk_notebook_prev_page(notebook);
	} else if (KEY(GDK_KEY_Page_Down, 0)) {
		gtk_notebook_next_page(notebook);
	} else if (KEY(GDK_KEY_C, GDK_SHIFT_MASK)) {
		term = get_current_term();
		vte_terminal_copy_clipboard_format(term, VTE_FORMAT_TEXT);
	} else if (KEY(GDK_KEY_V, GDK_SHIFT_MASK)) {
		term = get_current_term();
		vte_terminal_paste_clipboard(term);
	} else if (KEY(GDK_KEY_plus, GDK_SHIFT_MASK)) {
		set_font_scale(font_scale * 1.2);
	} else if (KEY(GDK_KEY_minus, 0)) {
		set_font_scale(font_scale / 1.2);
	} else if (KEY(GDK_KEY_0, 0)) {
		set_font_scale(1);
	} else {
		return FALSE;
	}
	return TRUE;
}

void activate(GtkApplication* app, gpointer user_data) {
	GtkWidget *widget;

	widget = gtk_application_window_new(app);
	window = GTK_WINDOW(widget);
	gtk_window_set_default_icon_name("utilities-terminal");
	gtk_window_set_default_size(window, 620, 340);
	gtk_window_set_title(window, "XiTerm");
	g_signal_connect(GTK_WIDGET(window), "key-press-event", G_CALLBACK(on_key), NULL);

	widget = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(window), widget);
	notebook = GTK_NOTEBOOK(widget);

	gtk_notebook_set_show_border(notebook, FALSE);
	gtk_widget_show_all(GTK_WIDGET(window));

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
