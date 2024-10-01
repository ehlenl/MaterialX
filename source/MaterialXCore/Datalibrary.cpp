//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include <MaterialXCore/Document.h>
#include <MaterialXCore/Datalibrary.h>

MATERIALX_NAMESPACE_BEGIN

const DataLibraryPtr standardDataLibrary = DataLibrary::create();

DataLibraryPtr DataLibrary::create()
{
    return std::make_shared<DataLibrary>();
}

// use loadDocuments to build this vector
void DataLibrary::loadDataLibrary(vector<DocumentPtr>& librarydocuments)
{
    _datalibrary = createDocument();

    for (auto library : librarydocuments)
    {
        for (auto child : library->getChildren())
        {
            if (child->getCategory().empty())
            {
                throw Exception("Trying to import child without a category: " + child->getName());
            }

            const string childName = child->getQualifiedName(child->getName());

            // Check for duplicate elements.
            ConstElementPtr previous = _datalibrary->getChild(childName);
            if (previous)
            {
                continue;
            }

            // Create the imported element.
            ElementPtr childCopy = _datalibrary->addChildOfCategory(child->getCategory(), childName);
            childCopy->copyContentFrom(child);
            if (!childCopy->hasFilePrefix() && library->hasFilePrefix())
            {
                childCopy->setFilePrefix(library->getFilePrefix());
            }
            if (!childCopy->hasGeomPrefix() && library->hasGeomPrefix())
            {
                childCopy->setGeomPrefix(library->getGeomPrefix());
            }
            if (!childCopy->hasColorSpace() && library->hasColorSpace())
            {
                childCopy->setColorSpace(library->getColorSpace());
            }
            if (!childCopy->hasNamespace() && library->hasNamespace())
            {
                childCopy->setNamespace(library->getNamespace());
            }
            if (!childCopy->hasSourceUri() && library->hasSourceUri())
            {
                childCopy->setSourceUri(library->getSourceUri());
            }
        }
    }

}


MATERIALX_NAMESPACE_END
