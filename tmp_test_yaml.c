#include <stdio.h>
#include <stdlib.h>
#include <yaml.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s file\n", argv[0]);
        return 2;
    }
    const char *path = argv[1];
    FILE *fh = fopen(path, "rb");
    if (!fh)
    {
        perror("open");
        return 1;
    }
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser))
    {
        fprintf(stderr, "parser init failed\n");
        return 1;
    }
    yaml_parser_set_input_file(&parser, fh);
    yaml_document_t doc;
    if (!yaml_parser_load(&parser, &doc))
    {
        fprintf(stderr, "parse failed\n");
        return 1;
    }
    yaml_node_t *root = yaml_document_get_root_node(&doc);
    if (!root)
    {
        fprintf(stderr, "no root\n");
        return 1;
    }
    printf("root type=%d\n", root->type);
    if (root->type == YAML_MAPPING_NODE)
    {
        printf("pairs.top=%zu\n", root->data.mapping.pairs.top);
        for (size_t i = 0; i < root->data.mapping.pairs.top && i < 16; i++)
        {
            yaml_node_pair_t p = root->data.mapping.pairs.start[i];
            yaml_node_t *k = yaml_document_get_node(&doc, p.key);
            yaml_node_t *v = yaml_document_get_node(&doc, p.value);
            printf("pair %zu: key-type=%d val-type=%d\n", i, k ? k->type : -1, v ? v->type : -1);
            if (k && k->type == YAML_SCALAR_NODE)
                printf(" key=%s\n", k->data.scalar.value);
        }
    }
    yaml_document_delete(&doc);
    yaml_parser_delete(&parser);
    fclose(fh);
    return 0;
}
