#!/usr/bin/env python
'''
Generate and write *.mtlx nodegraph for a target format from an input document.
'''

import sys, os, argparse

import MaterialX as mx
from MaterialX import PyMaterialXGenShader as mx_gen_shader

from utils import load_mtlx_doc, load_std_lib, validate_mtlx_doc

def translate_mtlx_doc(translator, doc, dest):
    # Translate materials between shading models
    try:
        translator.translateAllMaterials(doc, dest)
    except mx.Exception as err:
        print(err)
        sys.exit(0)

def main():
    parser = argparse.ArgumentParser(description="Generate and write *.mtlx nodegraph for a target format from an input document.")
    parser.add_argument("--path", dest="paths", action='append', nargs='+', help="An additional absolute search path location (e.g. '/projects/MaterialX')")
    parser.add_argument("--library", dest="libraries", action='append', nargs='+', help="An additional relative path to a custom data library folder (e.g. 'libraries/custom')")
    parser.add_argument(dest="inputFilename", help="Filename of the input document.")
    parser.add_argument(dest="outputFilename", help="Filename of the output document.")
    parser.add_argument(dest="model", help="Target material model for translation (UsdPreviewSurface, gltf_pbr)")
    opts = parser.parse_args()

    doc = load_mtlx_doc(opts.inputFilename)
    stdlib = load_std_lib(opts.inputFilename, opts.paths, opts.libraries)
    doc.importLibrary(stdlib)
    validate_mtlx_doc(doc)

    translator = mx_gen_shader.ShaderTranslator.create()
    translate_mtlx_doc(translator, doc, opts.model)
    
    mx.writeToXmlFile(doc, opts.outputFilename)
    print(f"Wrote {opts.outputFilename}")
        
if __name__ == '__main__':
    main()
