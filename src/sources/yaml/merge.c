#include "sources/yaml/merge.h"
#include "leuko_debug.h"
#include <stdlib.h>
#include <string.h>

/* Implementation: emit merged YAML via libyaml events into an in-memory
 * buffer (via a write callback), then parse that buffer into a new
 * heap-allocated yaml_document_t. This avoids fragile manual formatting
 * while keeping the merged structure as a proper YAML document.
 */

struct dyn_buf {
    unsigned char *buf;
    size_t len;
    size_t cap;
};

static int emitter_write_handler(void *data, unsigned char *buffer, size_t size)
{
    struct dyn_buf *d = data;
    if (!d) return 0;
    if (d->len + size > d->cap) {
        size_t ncap = d->cap ? d->cap * 2 : 4096;
        while (ncap < d->len + size) ncap *= 2;
        unsigned char *n = realloc(d->buf, ncap);
        if (!n) return 0;
        d->buf = n; d->cap = ncap;
    }
    memcpy(d->buf + d->len, buffer, size);
    d->len += size;
    return 1;
}

static yaml_node_t *find_mapping_value(yaml_document_t *doc, yaml_node_t *mapping, const char *key)
{
    if (!mapping || mapping->type != YAML_MAPPING_NODE) return NULL;
    for (size_t j = 0; j < (size_t)mapping->data.mapping.pairs.top; j++) {
        yaml_node_pair_t p2 = mapping->data.mapping.pairs.start[j];
        yaml_node_t *k2 = yaml_document_get_node(doc, p2.key);
        yaml_node_t *v2 = yaml_document_get_node(doc, p2.value);
        if (!k2 || k2->type != YAML_SCALAR_NODE) continue;
        if (strcmp((char *)k2->data.scalar.value, key) == 0) return v2;
    }
    return NULL;
}

static bool emit_scalar_from_node(yaml_emitter_t *em, yaml_node_t *n)
{
    yaml_event_t ev;
    const unsigned char *value = (const unsigned char *)n->data.scalar.value;
    size_t len = strlen((const char *)value);
    if (!yaml_scalar_event_initialize(&ev, NULL, NULL, (yaml_char_t *)value, len, 1, 1, YAML_ANY_SCALAR_STYLE)) {
        return false;
    }
    bool ok = yaml_emitter_emit(em, &ev);
    yaml_event_delete(&ev);
    return ok ? true : false;
}

static bool emit_node_from_doc(yaml_emitter_t *em, yaml_document_t *doc, yaml_node_t *n);
static bool emit_merged_node(yaml_emitter_t *em, yaml_document_t *pdoc, yaml_node_t *pnode, yaml_document_t *cdoc, yaml_node_t *cnode);

static bool emit_node_from_doc(yaml_emitter_t *em, yaml_document_t *doc, yaml_node_t *n)
{
    if (!n) return true;
    if (n->type == YAML_SCALAR_NODE) return emit_scalar_from_node(em, n);
    if (n->type == YAML_SEQUENCE_NODE) {
        yaml_event_t ev;
        if (!yaml_sequence_start_event_initialize(&ev, NULL, NULL, 1, YAML_BLOCK_SEQUENCE_STYLE)) return false;
        if (!yaml_emitter_emit(em, &ev)) { yaml_event_delete(&ev); return false; }
        yaml_event_delete(&ev);
        for (size_t i = 0; i < (size_t)n->data.sequence.items.top; i++) {
            yaml_node_t *it = yaml_document_get_node(doc, n->data.sequence.items.start[i]);
            if (!emit_node_from_doc(em, doc, it)) return false;
        }
        if (!yaml_sequence_end_event_initialize(&ev)) return false;
        if (!yaml_emitter_emit(em, &ev)) { yaml_event_delete(&ev); return false; }
        yaml_event_delete(&ev);
        return true;
    }
    if (n->type == YAML_MAPPING_NODE) {
        yaml_event_t ev;
        if (!yaml_mapping_start_event_initialize(&ev, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE)) return false;
        if (!yaml_emitter_emit(em, &ev)) { yaml_event_delete(&ev); return false; }
        yaml_event_delete(&ev);
        for (size_t i = 0; i < (size_t)n->data.mapping.pairs.top; i++) {
            yaml_node_pair_t p = n->data.mapping.pairs.start[i];
            yaml_node_t *k = yaml_document_get_node(doc, p.key);
            yaml_node_t *v = yaml_document_get_node(doc, p.value);
            if (!k || k->type != YAML_SCALAR_NODE) continue;
            if (!emit_scalar_from_node(em, k)) return false;
            if (!emit_node_from_doc(em, doc, v)) return false;
        }
        if (!yaml_mapping_end_event_initialize(&ev)) return false;
        if (!yaml_emitter_emit(em, &ev)) { yaml_event_delete(&ev); return false; }
        yaml_event_delete(&ev);
        return true;
    }
    return true;

}

static bool emit_merged_node(yaml_emitter_t *em, yaml_document_t *pdoc, yaml_node_t *pnode, yaml_document_t *cdoc, yaml_node_t *cnode)
{
    if (pnode && cnode) {
        if (pnode->type == YAML_MAPPING_NODE && cnode->type == YAML_MAPPING_NODE) {
            yaml_event_t ev;
            if (!yaml_mapping_start_event_initialize(&ev, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE)) return false;
            if (!yaml_emitter_emit(em, &ev)) { yaml_event_delete(&ev); return false; }
            yaml_event_delete(&ev);
            for (size_t i = 0; i < (size_t)pnode->data.mapping.pairs.top; i++) {
                yaml_node_pair_t p = pnode->data.mapping.pairs.start[i];
                yaml_node_t *k = yaml_document_get_node(pdoc, p.key);
                yaml_node_t *v = yaml_document_get_node(pdoc, p.value);
                if (!k || k->type != YAML_SCALAR_NODE) continue;
                const char *kstr = (char *)k->data.scalar.value;
                yaml_node_t *cv = find_mapping_value(cdoc, cnode, kstr);
                if (!emit_scalar_from_node(em, k)) return false;
                if (cv) {
                    if (v && v->type == YAML_MAPPING_NODE && cv->type == YAML_MAPPING_NODE) {
                        if (!emit_merged_node(em, pdoc, v, cdoc, cv)) return false;
                    } else {
                        if (!emit_node_from_doc(em, cdoc, cv)) return false;
                    }
                } else {
                    if (!emit_node_from_doc(em, pdoc, v)) return false;
                }
            }
            for (size_t j = 0; j < (size_t)cnode->data.mapping.pairs.top; j++) {
                yaml_node_pair_t p2 = cnode->data.mapping.pairs.start[j];
                yaml_node_t *k2 = yaml_document_get_node(cdoc, p2.key);
                yaml_node_t *v2 = yaml_document_get_node(cdoc, p2.value);
                if (!k2 || k2->type != YAML_SCALAR_NODE) continue;
                const char *kstr = (char *)k2->data.scalar.value;
                if (find_mapping_value(pdoc, pnode, kstr)) continue;
                if (!emit_scalar_from_node(em, k2)) return false;
                if (!emit_node_from_doc(em, cdoc, v2)) return false;
            }
            if (!yaml_mapping_end_event_initialize(&ev)) return false;
            if (!yaml_emitter_emit(em, &ev)) { yaml_event_delete(&ev); return false; }
            yaml_event_delete(&ev);
            return true;
        }
        if (pnode->type == YAML_SEQUENCE_NODE && cnode->type == YAML_SEQUENCE_NODE) {
            yaml_event_t ev;
            if (!yaml_sequence_start_event_initialize(&ev, NULL, NULL, 1, YAML_BLOCK_SEQUENCE_STYLE)) return false;
            if (!yaml_emitter_emit(em, &ev)) { yaml_event_delete(&ev); return false; }
            yaml_event_delete(&ev);
            for (size_t i = 0; i < (size_t)pnode->data.sequence.items.top; i++) {
                yaml_node_t *it = yaml_document_get_node(pdoc, pnode->data.sequence.items.start[i]);
                if (!emit_node_from_doc(em, pdoc, it)) return false;
            }
            for (size_t i = 0; i < (size_t)cnode->data.sequence.items.top; i++) {
                yaml_node_t *it = yaml_document_get_node(cdoc, cnode->data.sequence.items.start[i]);
                if (!emit_node_from_doc(em, cdoc, it)) return false;
            }
            if (!yaml_sequence_end_event_initialize(&ev)) return false;
            if (!yaml_emitter_emit(em, &ev)) { yaml_event_delete(&ev); return false; }
            yaml_event_delete(&ev);
            return true;
        }
        return emit_node_from_doc(em, cdoc, cnode);
    }
    if (pnode && !cnode) return emit_node_from_doc(em, pdoc, pnode);
    if (cnode && !pnode) return emit_node_from_doc(em, cdoc, cnode);
    return true;
}

bool leuko_yaml_merge_documents(yaml_document_t *parent, yaml_document_t *child, yaml_document_t **out_merged)
{
    if (!out_merged) return false;

    yaml_node_t *proot = parent ? yaml_document_get_root_node(parent) : NULL;
    yaml_node_t *croot = child ? yaml_document_get_root_node(child) : NULL;

    yaml_emitter_t emitter;
    if (!yaml_emitter_initialize(&emitter)) { LDEBUG("yaml: emitter init failed"); return false; }
    struct dyn_buf d = {NULL, 0, 0};
    yaml_emitter_set_output(&emitter, emitter_write_handler, &d);
    yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

    if (!yaml_emitter_open(&emitter)) { yaml_emitter_delete(&emitter); return false; }
    yaml_event_t ev;
    if (!yaml_stream_start_event_initialize(&ev, YAML_UTF8_ENCODING)) { yaml_emitter_delete(&emitter); return false; }
    if (!yaml_emitter_emit(&emitter, &ev)) { yaml_event_delete(&ev); yaml_emitter_delete(&emitter); return false; }
    yaml_event_delete(&ev);

    if (!yaml_document_start_event_initialize(&ev, NULL, NULL, NULL, 0)) { yaml_emitter_delete(&emitter); return false; }
    if (!yaml_emitter_emit(&emitter, &ev)) { yaml_event_delete(&ev); yaml_emitter_delete(&emitter); return false; }
    yaml_event_delete(&ev);

    if (proot && proot->type == YAML_MAPPING_NODE && croot && croot->type == YAML_MAPPING_NODE) {
        if (!emit_merged_node(&emitter, parent, proot, child, croot)) { yaml_emitter_delete(&emitter); free(d.buf); return false; }
    } else if (proot && proot->type == YAML_SEQUENCE_NODE && croot && croot->type == YAML_SEQUENCE_NODE) {
        if (!emit_merged_node(&emitter, parent, proot, child, croot)) { yaml_emitter_delete(&emitter); free(d.buf); return false; }
    } else if (croot) {
        if (!emit_node_from_doc(&emitter, child, croot)) { yaml_emitter_delete(&emitter); free(d.buf); return false; }
    } else if (proot) {
        if (!emit_node_from_doc(&emitter, parent, proot)) { yaml_emitter_delete(&emitter); free(d.buf); return false; }
    }

    if (!yaml_document_end_event_initialize(&ev, 0)) { yaml_emitter_delete(&emitter); free(d.buf); return false; }
    if (!yaml_emitter_emit(&emitter, &ev)) { yaml_event_delete(&ev); yaml_emitter_delete(&emitter); free(d.buf); return false; }
    yaml_event_delete(&ev);

The file now contains additional old implementation pieces further down: the append_buf etc remains. Our new clean file content is now duplicated and corrupted because the earlier replacement left mid-lines. Creating a clean file and moving it over will be easier now. Let's create a clean file content and move it to overwrite this file. We'll create merge.c.new with the clean content and then run 'mv' to replace the original.
