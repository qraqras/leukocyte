#!/usr/bin/env python3
"""
Simple schema generator placeholder for Layout rules.
Generates C header/source skeletons for schema-driven deserialization.
"""
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(__file__))
SCHEMA_DIR = os.path.join(ROOT, 'schema')
OUT_INCLUDE = os.path.join(ROOT, 'include', 'configs', 'schema')
OUT_SRC = os.path.join(ROOT, 'src', 'configs', 'schema')


def ensure_dirs():
    os.makedirs(OUT_INCLUDE, exist_ok=True)
    os.makedirs(OUT_SRC, exist_ok=True)


def generate_for_layout(schema_path):
    # Minimal placeholder generator: copy schema into include for now
    with open(schema_path, 'r') as f:
        schema = json.load(f)

    # Create a tiny header that documents schema presence
    header_path = os.path.join(OUT_INCLUDE, 'layout_schema.h')
    with open(header_path, 'w') as h:
        h.write('/* Generated schema header for Layout (prototype) */\n')
        h.write('#ifndef LEUKO_CONFIGS_SCHEMA_LAYOUT_H\n')
        h.write('#define LEUKO_CONFIGS_SCHEMA_LAYOUT_H\n\n')
        h.write('/* Schema: %s */\n' % json.dumps(schema))
        h.write('\n#endif /* LEUKO_CONFIGS_SCHEMA_LAYOUT_H */\n')

    # Create a tiny C source stub
    src_path = os.path.join(OUT_SRC, 'layout_deser.c')
    with open(src_path, 'w') as s:
        s.write('/* Schema-driven deserializer stub for Layout */\n')
        s.write('#include "configs/schema/layout_schema.h"\n')
        s.write('#include <stdbool.h>\n')
        s.write('\n')
        s.write('bool leuko_schema_layout_enabled(void) { return true; }\n')

    print('Generated schema artifacts: ', header_path, src_path)


if __name__ == '__main__':
    ensure_dirs()
    schema_file = os.path.join(SCHEMA_DIR, 'layout.json')
    if not os.path.exists(schema_file):
        print('Schema file not found:', schema_file)
        sys.exit(1)
    generate_for_layout(schema_file)
    print('OK')
