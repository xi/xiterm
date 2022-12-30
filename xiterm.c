#include <string.h>
#include <gtk/gtk.h>
#include <vte/vte.h>

#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

#define REGEX_URL "https?://[^\\s<>]*[^\\s\\])}<>.,:;?!\"']"
#define KEY(v, s) (keyval == (v) && modifiers == (GDK_CONTROL_MASK|(s)))
#define KEY_S(v) (keyval == (v) && (GDK_SHIFT_MASK|modifiers) == (GDK_SHIFT_MASK|GDK_CONTROL_MASK))

GtkWindow *window;
GtkNotebook *notebook;
VteRegex *url_regex;
GdkRGBA palette[16];
double font_scale = 1;

char *cmd[4] = {"/bin/bash", NULL, NULL, NULL};
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
	GtkWidget *label;

	title = vte_terminal_get_window_title(term);
	label = gtk_notebook_get_tab_label(notebook, GTK_WIDGET(term));
	gtk_label_set_text(GTK_LABEL(label), title);
}

void on_term_click(GtkGestureClick* self, int n_press, double x, double y, gpointer user_data) {
	char *uri;
	GtkWidget *widget;
	VteTerminal *term;

	widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));
	term = VTE_TERMINAL(widget);

	uri = vte_terminal_check_match_at(term, x, y, NULL);
	if (uri != NULL) {
		gtk_show_uri(window, uri, GDK_CURRENT_TIME);
		g_free(uri);
	}
}

void on_term_exit(VteTerminal *term, int status, gpointer user_data) {
	gtk_notebook_remove_page(notebook, gtk_notebook_page_num(notebook, GTK_WIDGET(term)));

	if (gtk_notebook_get_n_pages(notebook) == 0) {
		gtk_window_close(window);
	} else {
		update_show_tabs();
	}
}

VteTerminal *get_current_term(void) {
	int page_num;

	page_num = gtk_notebook_get_current_page(notebook);
	GtkWidget *page = gtk_notebook_get_nth_page(notebook, page_num);
	return VTE_TERMINAL(page);
}

void restore_focus(void) {
	VteTerminal *term;

	term = get_current_term();
	gtk_widget_grab_focus(GTK_WIDGET(term));
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
	GtkGesture *click_controller;

	cwd = get_cwd(get_current_term());

	vte_terminal_spawn_async(term, VTE_PTY_DEFAULT, cwd, cmd, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, -1, NULL, NULL, NULL);
	vte_terminal_set_cursor_blink_mode(term, VTE_CURSOR_BLINK_OFF);
	tag = vte_terminal_match_add_regex(term, url_regex, 0);
	vte_terminal_match_set_cursor_name(term, tag, "pointer");
	vte_terminal_set_colors(term, &palette[15], NULL, palette, 16);
	vte_terminal_set_bold_is_bright(term, TRUE);
	vte_terminal_set_font_scale(term, font_scale);
	vte_terminal_set_enable_bidi(term, FALSE);

	g_signal_connect(term, "window-title-changed", G_CALLBACK(on_term_title), NULL);
	g_signal_connect(term, "child-exited", G_CALLBACK(on_term_exit), NULL);

	click_controller = gtk_gesture_click_new();
	gtk_widget_add_controller(GTK_WIDGET(term), GTK_EVENT_CONTROLLER(click_controller));
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_controller), 3);
	g_signal_connect(click_controller, "pressed", G_CALLBACK(on_term_click), NULL);
}

void add_tab(void) {
	GtkWidget *page, *label;
	int page_num;

	page = vte_terminal_new();
	page_num = gtk_notebook_get_current_page(notebook) + 1;
	page_num = gtk_notebook_insert_page(notebook, page, NULL, page_num);
	gtk_notebook_set_tab_reorderable(notebook, page, TRUE);

	label = gtk_label_new("");
	gtk_widget_set_hexpand(label, TRUE);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
	gtk_notebook_set_tab_label(notebook, page, label);

	update_show_tabs();

	// needs to execute after gtk_widget_show() (for proper dimensions)
	// and before gtk_notebook_get_current_page() (so we can access the
	// previous term to get cwd)
	setup_terminal(VTE_TERMINAL(page));

	gtk_notebook_set_current_page(notebook, page_num);
	gtk_widget_grab_focus(page);
}

void move_tab(int offset) {
	int k = gtk_notebook_get_current_page(notebook);
	GtkWidget *widget = gtk_notebook_get_nth_page(notebook, k);
	if (k + offset >= 0) {
		gtk_notebook_reorder_child(notebook, widget, k + offset);
	}
}

gboolean on_key(GtkEventControllerKey* self, unsigned int keyval, unsigned int keycode, GdkModifierType state, gpointer user_data) {
	VteTerminal *term;
	GdkModifierType modifiers;

	modifiers = state & gtk_accelerator_get_default_mod_mask();

	if (!(modifiers & GDK_CONTROL_MASK)) {
		return FALSE;
	} else if (KEY(GDK_KEY_T, GDK_SHIFT_MASK)) {
		add_tab();
	} else if (KEY(GDK_KEY_Page_Up, 0)) {
		gtk_notebook_prev_page(notebook);
		restore_focus();
	} else if (KEY(GDK_KEY_Page_Down, 0)) {
		gtk_notebook_next_page(notebook);
		restore_focus();
	} else if (KEY(GDK_KEY_Page_Up, GDK_SHIFT_MASK)) {
		move_tab(-1);
	} else if (KEY(GDK_KEY_Page_Down, GDK_SHIFT_MASK)) {
		move_tab(1);
	} else if (KEY(GDK_KEY_C, GDK_SHIFT_MASK)) {
		term = get_current_term();
		vte_terminal_copy_clipboard_format(term, VTE_FORMAT_TEXT);
	} else if (KEY(GDK_KEY_V, GDK_SHIFT_MASK)) {
		term = get_current_term();
		vte_terminal_paste_clipboard(term);
	} else if (KEY_S(GDK_KEY_plus)) {
		set_font_scale(font_scale * 1.2);
	} else if (KEY_S(GDK_KEY_minus)) {
		set_font_scale(font_scale / 1.2);
	} else if (KEY_S(GDK_KEY_0)) {
		set_font_scale(1);
	} else {
		return FALSE;
	}
	return TRUE;
}

int main(int argc, char **argv) {
	int i;
	char command[128] = "";
	GError *err = NULL;
	GtkWidget *widget;
	GtkEventController *key_controller;

	if (argc > 1) {
		if (strcmp(argv[1], "-e") != 0) {
			fprintf(stderr, "Usage: xiterm [-e CMD]\n");
			exit(EXIT_FAILURE);
		}
		for (i = 2; i < argc; i++) {
			strncat(command, "\"", 128 - 1 - strlen(command));
			strncat(command, argv[i], 128 - 1 - strlen(command));
			strncat(command, "\" ", 128 - 1 - strlen(command));
		}
		cmd[0] = "/bin/sh";
		cmd[1] = "-c";
		cmd[2] = command;
	}

	url_regex = vte_regex_new_for_match(REGEX_URL, -1, PCRE2_MULTILINE, &err);
	g_assert(err == NULL);

	for (i = 0; i < 16; i++) {
		gdk_rgba_parse(palette + i, colors[i]);
	}

	gtk_init();
	widget = gtk_window_new();
	window = GTK_WINDOW(widget);
	gtk_window_set_default_icon_name("utilities-terminal");
	gtk_window_set_default_size(window, 620, 340);
	gtk_window_set_title(window, "XiTerm");

	key_controller = gtk_event_controller_key_new();
	gtk_widget_add_controller(widget, key_controller);
	gtk_event_controller_set_propagation_phase(key_controller, GTK_PHASE_CAPTURE);
	g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key), NULL);

	widget = gtk_notebook_new();
	gtk_window_set_child(window, widget);
	notebook = GTK_NOTEBOOK(widget);

	gtk_notebook_set_show_border(notebook, FALSE);
	gtk_widget_show(GTK_WIDGET(window));

	add_tab();
	while (g_list_model_get_n_items(gtk_window_get_toplevels()) > 0) {
		g_main_context_iteration(NULL, TRUE);
	}
	vte_regex_unref(url_regex);

	return 0;
}
