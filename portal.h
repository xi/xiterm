/* workarounds for broken portal support in GTK */

#include <string.h>
#include <gio/gio.h>

static GDBusProxy *openuri_portal;

static GDBusProxy *get_portal(const char *iface) {
	GError *error = NULL;

	GDBusProxy *proxy = g_dbus_proxy_new_for_bus_sync(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_NONE,
		NULL,
		"org.freedesktop.portal.Desktop",
		"/org/freedesktop/portal/desktop",
		iface,
		NULL,
		&error
	);

	if (error) {
		g_printerr("Failed connect to portal %s: %s\n", iface, error->message);
		g_clear_error(&error);
		return NULL;
	}

	return proxy;
}

void portal_setup(void) {
	openuri_portal = get_portal("org.freedesktop.portal.OpenURI");
}

void portal_finalize(void) {
	if (openuri_portal) {
		g_clear_object(&openuri_portal);
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
