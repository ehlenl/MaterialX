import os, sys

import MaterialX as mx

def load_mtlx_doc(filename):
    # Read a *.mtlx Xml file
    doc = mx.createDocument()
    try:
        mx.readFromXmlFile(doc, filename)
    except mx.ExceptionFileMissing as err:
        print(err)
        sys.exit(0)

    return doc

def load_std_lib(inputFile, paths=[], libraries=[]):
    # Attempt to load standard data libraries
    stdlib = mx.createDocument()
    filePath = os.path.dirname(os.path.abspath(__file__))
    searchPath = mx.FileSearchPath(os.path.join(filePath, '..', '..'))
    searchPath.append(os.path.dirname(inputFile))
    libraryFolders = []
    if paths:
        for pathList in paths:
            for path in pathList:
                searchPath.append(path)
    if libraries:
        for libraryList in libraries:
            for library in libraryList:
                libraryFolders.append(library)
    libraryFolders.append("libraries")
    mx.loadLibraries(libraryFolders, searchPath, stdlib)

    return stdlib

def validate_mtlx_doc(doc):
    # MaterialX document validator
    valid, msg = doc.validate()
    if not valid:
        print("Validation warnings for input document:")
        print(msg)
