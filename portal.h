/* workarounds for broken portal support in GTK */

#include <string.h>
#include <gio/gio.h>

#define PREFER_DARK 1

static GDBusProxy *openuri_portal;
static GDBusProxy *settings_portal;

static void portal_new(const char *iface, GAsyncReadyCallback callback) {
	g_printerr("Connecting to %s\n", iface);
	g_dbus_proxy_new_for_bus(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_NONE,
		NULL,
		"org.freedesktop.portal.Desktop",
		"/org/freedesktop/portal/desktop",
		iface,
		NULL,
		callback,
		NULL
	);
}

static void set_color_scheme(GVariant *scheme) {
	unsigned int value = -1;

	g_variant_get(scheme, "u", &value);
	g_object_set(
		gtk_settings_get_default(),
		"gtk-application-prefer-dark-theme",
		value == PREFER_DARK,
		NULL
	);
}

static void on_settings_signal(
	GDBusProxy *proxy,
	const char *sender_name,
	const char *signal_name,
	GVariant *parameters,
	GdkScreen *screen
) {
	const char *ns;
	const char *key;
	GVariant *value;

	if (strcmp(signal_name, "SettingChanged") == 0) {
		g_variant_get(parameters, "(&s&sv)", &ns, &key, &value);
		if (!strcmp(ns, "org.freedesktop.appearance") && !strcmp(key, "color-scheme")) {
			set_color_scheme(value);
		}
		g_variant_unref(value);
	}
}

static void setup_color_scheme(void) {
	GVariant *raw, *scheme;

	if (settings_portal) {
		g_signal_connect(
			settings_portal,
			"g-signal",
			G_CALLBACK(on_settings_signal),
			NULL
		);

		raw = g_dbus_proxy_call_sync(
			settings_portal,
			"ReadOne",
			g_variant_new("(ss)", "org.freedesktop.appearance", "color-scheme"),
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			NULL
		);
		if (raw) {
			g_variant_get(raw, "(v)", &scheme);
			if (scheme) {
				set_color_scheme(scheme);
				g_variant_unref(scheme);
			}
			g_variant_unref(raw);
		}
	}
}

static void on_openuri_portal(GObject* source_object, GAsyncResult* res, gpointer data) {
	GError *error = NULL;
	openuri_portal = g_dbus_proxy_new_for_bus_finish(res, &error);
	if (error) {
		g_printerr("Failed connect to OpenURI portal: %s\n", error->message);
		g_clear_error(&error);
		openuri_portal = NULL;
	}
}

static void on_settings_portal(GObject* source_object, GAsyncResult* res, gpointer data) {
	GError *error = NULL;
	settings_portal = g_dbus_proxy_new_for_bus_finish(res, &error);
	if (error) {
		g_printerr("Failed connect to Settings portal: %s\n", error->message);
		g_clear_error(&error);
		settings_portal = NULL;
	}
	setup_color_scheme();
}

void portal_setup(void) {
	portal_new("org.freedesktop.portal.OpenURI", on_openuri_portal);
	portal_new("org.freedesktop.portal.Settings", on_settings_portal);
}

void portal_finalize(void) {
	if (openuri_portal) {
		g_clear_object(&openuri_portal);
	}
	if (settings_portal) {
		g_clear_object(&settings_portal);
	}
}

void open_uri(const char *uri) {
	if (openuri_portal) {
		g_dbus_proxy_call_sync(
			openuri_portal,
			"OpenURI",
			g_variant_new("(ssa{sv})", "", uri, NULL),
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			NULL
		);
	}
}
