// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <glib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "xml.h"

static struct ctx {
	char *filename;
	xmlDoc *doc;
	xmlXPathContextPtr xpath_ctx_ptr;
	const char *nodename;
	const char *value;
	xmlNode *node;
	enum {
		XML_MODE_SETTING = 0,
		XML_MODE_GETTING,
	} mode;
} ctx;

static void
entry(xmlNode *node, char *nodename, char *content)
{
	if (!nodename)
		return;
	if (!strcasecmp(nodename, ctx.nodename)) {
		if (ctx.mode == XML_MODE_SETTING) {
			xmlNodeSetContent(node, (const xmlChar *)ctx.value);
		} else if (ctx.mode == XML_MODE_GETTING) {
			ctx.value = (char *)content;
			ctx.node = node;
		}
	}
}

/**
 * nodename - return simplistic xpath style nodename
 * For example: <A><B><C></C></B></A> is represented by nodename /a/b/c
 */
static char *
nodename(xmlNode *node, char *buf, int len)
{
	if (!node || !node->name) {
		return NULL;
	}

	/* Ignore superflous '/text' in node name */
	if (node->parent && !strcmp((char *)node->name, "text")) {
		node = node->parent;
	}

	buf += len;
	*--buf = 0;
	len--;

	for (;;) {
		const char *name = (char *)node->name;
		int i = strlen(name);
		while (--i >= 0) {
			unsigned char c = name[i];
			*--buf = tolower(c);
			if (!--len)
				return buf;
		}
		node = node->parent;
		if (!node || !node->name) {
			*--buf = '/';
			return buf;
		}
		*--buf = '/';
		if (!--len)
			return buf;
	}
}

static void
process_node(xmlNode *node)
{
	char *content;
	static char buffer[256];
	char *name;

	content = (char *)node->content;
	if (xmlIsBlankNode(node)) {
		return;
	}
	name = nodename(node, buffer, sizeof(buffer));
	entry(node, name, content);
}

static void xml_tree_walk(xmlNode *node);

static void
traverse(xmlNode *n)
{
	process_node(n);
	for (xmlAttr *attr = n->properties; attr; attr = attr->next) {
		xml_tree_walk(attr->children);
	}
	xml_tree_walk(n->children);
}

static void
xml_tree_walk(xmlNode *node)
{
	for (xmlNode *n = node; n && n->name; n = n->next) {
		if (!strcasecmp((char *)n->name, "comment")) {
			continue;
		}
		traverse(n);
	}
}

static const char rcxml_template[] =
	"<?xml version=\"1.0\"?>\n"
	"<labwc_config>\n"
	"  <core>\n"
	"  </core>\n"
	"</labwc_config>\n";

static void
create_basic_rcxml(const char *filename)
{
	FILE *file = fopen(filename, "w");
	if (!file) {
		fprintf(stderr, "warn: fopen(%s) failed\n", filename);
		return;
	}
	if (!fwrite(rcxml_template, sizeof(rcxml_template), 1, file)) {
		fprintf(stderr, "warn: error writing to %s", filename);
	}
	fclose(file);
}

/* case-sensitive */
static xmlNode *
xpath_get_node(xmlChar *expr)
{
	xmlNode *ret = NULL;
	xmlXPathObjectPtr object = xmlXPathEvalExpression(expr, ctx.xpath_ctx_ptr);
	if (!object) {
		fprintf(stderr, "warn: xmlXPathEvalExpression()\n");
		return NULL;
	}
	if (!object->nodesetval) {
		// fprintf(stderr, "warn: no nodesetval\n");
		goto out2;
	}

	for (int i = 0; i < object->nodesetval->nodeNr; i++) {
		if (!object->nodesetval->nodeTab[i]) {
			continue;
		}
		ret = object->nodesetval->nodeTab[i];
		break;
	}
out2:
	xmlXPathFreeObject(object);
	return ret;
}

void
xml_setup_nodes(void)
{
	/* Ensure all relevant nodes exist before we start getting/setting */
	xpath_add_node("/labwc_config/theme/cornerRadius");
	xpath_add_node("/labwc_config/theme/name");
	xpath_add_node("/labwc_config/libinput/device/naturalScroll");
	xml_save();
}

void
xml_init(const char *filename)
{
	LIBXML_TEST_VERSION

	if (access(filename, F_OK)) {
		create_basic_rcxml(filename);
	}

	/* Use XML_PARSE_NOBLANKS for xmlSaveFormatFile() to indent properly */
	ctx.filename = strdup(filename);
	ctx.doc = xmlReadFile(filename, NULL, XML_PARSE_NOBLANKS);
	if (!ctx.doc) {
		fprintf(stderr, "warn: xmlReadFile('%s')\n", filename);
	}
	ctx.xpath_ctx_ptr = xmlXPathNewContext(ctx.doc);
	if (!ctx.xpath_ctx_ptr) {
		fprintf(stderr, "warn: xmlXPathNewContext()\n");
		xmlFreeDoc(ctx.doc);
	}
}

void
xml_save(void)
{
	xmlSaveFormatFile(ctx.filename, ctx.doc, 1);
}

void
xml_save_as(const char *filename)
{
	xmlSaveFormatFile(filename, ctx.doc, 1);
}

void
xml_finish(void)
{
	xmlXPathFreeContext(ctx.xpath_ctx_ptr);
	xmlFreeDoc(ctx.doc);
	xmlCleanupParser();
	free(ctx.filename);
}

void
xml_set(const char *nodename, const char *value)
{
	ctx.nodename = nodename;
	ctx.value = value;
	ctx.mode = XML_MODE_SETTING;
	xml_tree_walk(xmlDocGetRootElement(ctx.doc));
}

void
xml_set_num(const char *nodename, double value)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%.0f", value);
	ctx.nodename = nodename;
	ctx.value = buf;
	ctx.mode = XML_MODE_SETTING;
	xml_tree_walk(xmlDocGetRootElement(ctx.doc));
}

char *
xml_get(const char *nodename)
{
	ctx.nodename = nodename;
	ctx.mode = XML_MODE_GETTING;
	xml_tree_walk(xmlDocGetRootElement(ctx.doc));
	return (char *)ctx.value;
}

int
xml_get_int(const char *nodename)
{
	ctx.nodename = nodename;
	ctx.mode = XML_MODE_GETTING;
	xml_tree_walk(xmlDocGetRootElement(ctx.doc));
	return ctx.value ? atoi(ctx.value) : 0;
}

int
xml_get_bool_text(const char *nodename)
{
	const char *value = xml_get(nodename);

	/* handle <foo></foo> and <foo /> where no value has been specified */
	if (!value || !*value) {
		return -1;
	}
	if (!strcasecmp(value, "yes") || !strcasecmp(value, "true")) {
		return 1;
	} else if (!strcasecmp(value, "no") || !strcasecmp(value, "false")) {
		return 0;
	} else {
		return -1;
	}
}

int
xml_get_choice(const char *nodename, const char *choices[])
{
	const char *value = xml_get(nodename);
	if (!value || !*value) {
		return 0;
	}
	for (int i = 0; choices[i]; i++) {
		if (!strcasecmp(value, choices[i])) {
			return i;
		}
	}
	return 0;
}


/* case-insensitive */
static xmlNode *
xml_get_node(const char *nodename)
{
	ctx.node = NULL;
	ctx.nodename = nodename;
	ctx.mode = XML_MODE_GETTING;
	xml_tree_walk(xmlDocGetRootElement(ctx.doc));
	return ctx.node;
}

char *
xpath_get_content(const char *xpath_expr)
{
	xmlChar *ret = NULL;
	xmlXPathObjectPtr object = xmlXPathEvalExpression((xmlChar *)xpath_expr, ctx.xpath_ctx_ptr);
	if (!object) {
		fprintf(stderr, "warn: xmlXPathEvalExpression()\n");
		return NULL;
	}
	if (!object->nodesetval) {
		// fprintf(stderr, "warn: no nodesetval\n");
		goto out;
	}
	for (int i = 0; i < object->nodesetval->nodeNr; i++) {
		if (!object->nodesetval->nodeTab[i]) {
			continue;
		}

		/* Just grab the first node and go */
		ret = xmlNodeGetContent(object->nodesetval->nodeTab[i]);
		goto out;
	}

out:
	xmlXPathFreeObject(object);
	return (char *)ret;
}

void
xpath_add_node(const char *xpath_expr)
{
	if (xml_get_node(xpath_expr)) {
		return;
	}

	/* find existing parent */
	char *parent_expr = strdup(xpath_expr);
	xmlNode *parent_node = NULL;
	while (parent_expr && *parent_expr) {
		parent_node = xpath_get_node((xmlChar *)parent_expr);
		if (parent_node) {
			break;
		}
		char *p = strrchr(parent_expr, '/');
		if (p && *p) {
			*p = '\0';
		} else {
			break;
		}
	}
	assert(parent_expr);
	if (!*parent_expr) {
		/* the whole xpath expression is new, so add to root */
		parent_node = xmlDocGetRootElement(ctx.doc);
	}

	/* add new nodes */
	gchar **nodes = g_strsplit(xpath_expr + strlen(parent_expr), "/", -1);
	for (gchar **s = nodes; *s; s++) {
		if (*s && **s) {
			parent_node = xmlNewChild(parent_node, NULL, (xmlChar *)*s, NULL);
		}
	}
	g_free(parent_expr);
	g_strfreev(nodes);
}

void xpath_set_font_prop(const char *place, const char *prop, const char *value)
{
	char xpath[1024];

	// Try to find the specific prop node first
	snprintf(xpath, sizeof(xpath), "/labwc_config/theme/font[@place='%s']/%s", place, prop);
	xmlNode *node = xpath_get_node((xmlChar *)xpath);
	if (node) {
		xmlNodeSetContent(node, (const xmlChar *)value);
		return;
	}

	// If prop doesn't exist, check if font node exists
	snprintf(xpath, sizeof(xpath), "/labwc_config/theme/font[@place='%s']", place);
	xmlNode *font_node = xpath_get_node((xmlChar *)xpath);

	if (!font_node) {
		// Font node doesn't exist, check theme node to add it to
		xmlNode *theme_node = xpath_get_node((xmlChar *)"/labwc_config/theme");
		if (!theme_node) {
			// If theme doesn't exist, we might be in trouble or need to create it
			// For robustness, reuse xpath_add_node logic for theme
			xpath_add_node("/labwc_config/theme");
			theme_node = xpath_get_node((xmlChar *)"/labwc_config/theme");
		}
		if (theme_node) {
			font_node = xmlNewChild(theme_node, NULL, (xmlChar *)"font", NULL);
			xmlSetProp(font_node, (xmlChar *)"place", (xmlChar *)place);
		}
	}

	if (font_node) {
		xmlNewChild(font_node, NULL, (xmlChar *)prop, (xmlChar *)value);
	}
}

char *xpath_get_font_prop(const char *place, const char *prop)
{
	char xpath[1024];
	snprintf(xpath, sizeof(xpath), "/labwc_config/theme/font[@place='%s']/%s", place, prop);
	return xpath_get_content(xpath);
}
