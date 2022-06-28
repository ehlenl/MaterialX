#!/usr/bin/env python
'''
Generate a .gltf file from a .mtlx gltf_pbr definition
'''

import argparse, json, os, sys
import MaterialX as mx
import MaterialX.PyMaterialXRender as mx_render


def main():
    parser = argparse.ArgumentParser(description='Generate a *.gltf material file from a *.mtlx gltf_pbr file.')
    parser.add_argument(dest='mtlxFile', help='Filename of the input *.mtlx document.')
    parser.add_argument(dest='gltfFile', help='Filename of the output *.gltf document.')
    parser.add_argument('--merge', dest='mergeFile', help='Filename of the *.gltf file to merge output into.')
    parser.add_argument('--path', dest='paths', action='append', nargs='+', help='An additional absolute search path location (e.g. "/projects/MaterialX")')
    parser.add_argument('--library', dest='libraries', action='append', nargs='+', help='An additional relative path to a custom data library folder (e.g. "libraries/custom")')
    opts = parser.parse_args()

    if opts.gltfFile[-5:] != '.gltf':
        parser.error("Output must specify a *.gltf file.")

    loader = mx_render.CgltfMaterialLoader.create()
    doc = mx.createDocument()
    try:
        mx.readFromXmlFile(doc, opts.mtlxFile)
    except mx.ExceptionFileMissing as err:
        parser.error(err)

    stdlib = mx.createDocument()
    filePath = os.path.dirname(os.path.abspath(__file__))
    searchPath = mx.FileSearchPath(os.path.join(filePath, '..', '..'))
    libraryFolders = []
    if opts.paths:
        for pathList in opts.paths:
            for path in pathList:
                searchPath.append(path)
    if opts.libraries:
        for libraryList in opts.libraries:
            for library in libraryList:
                libraryFolders.append(library)
    libraryFolders.append('libraries')
    mx.loadLibraries(libraryFolders, searchPath, stdlib)
    
    loader.setDefinitions(stdlib)
    doc.importLibrary(stdlib)

    valid, msg = doc.validate()
    if not valid:
        print('Validation warnings for input document:')
        print(msg)
    
    loader.setMaterials(doc)
    loader.save(opts.gltfFile)

    if opts.mergeFile:
        with open(opts.gltfFile) as f:
            source = json.load(f)
        with open(opts.mergeFile) as f:
            try:
                target = json.load(f)
            except json.decoder.JSONDecodeError as err:
                parser.error(f'{opts.mergeFile}: {err}')
        for k,v in source.items():
            target[k] = v
        with open(opts.mergeFile, 'w') as f:
            json.dump(target, f, indent=2)

if __name__ == '__main__':
    main()
