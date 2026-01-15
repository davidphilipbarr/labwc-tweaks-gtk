/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __XML_H
#define __XML_H

void xml_setup_nodes(void);
void xml_init(const char *filename);
void xml_save(void);
void xml_save_as(const char *filename);
void xml_finish(void);
void xml_set(const char *nodename, const char *value);
void xml_set_num(const char *nodename, double value);
char *xml_get(const char *nodename);
int xml_get_int(const char *nodename);
int xml_get_bool_text(const char *nodename);
int xml_get_choice(const char *nodename, const char *choices[]);

/**
 * xpath_get_content() - Get content of node specified by xpath
 * @xpath_expr: xpath expression for node
 */
char *xpath_get_content(const char *xpath_expr);

/**
 * xpath_add_node - add xml nodes from xpath
 * @xpath_expr: xpath expression for new node
 * For example xpath_expr="/labwc_config/a/b/c" creates
 * <labwc_config><a><b><c /></b></a></labwc_config>
 */
void xpath_add_node(const char *xpath_expr);

/* Font helpers */
void xpath_set_font_prop(const char *place, const char *prop, const char *value);
char *xpath_get_font_prop(const char *place, const char *prop);

#endif /* __XML_H */
