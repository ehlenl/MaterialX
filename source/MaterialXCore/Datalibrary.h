//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MATERIALX_DATALIBRARY
#define MATERIALX_DATALIBRARY

/// @file
/// The top-level DataLibrary class

#include <MaterialXCore/Document.h>

MATERIALX_NAMESPACE_BEGIN

class DataLibrary;
using DataLibraryPtr = shared_ptr<DataLibrary>;
using ConstDataLibraryPtr = shared_ptr<const DataLibrary>;

class MX_CORE_API DataLibrary
{
  public:
    ConstDocumentPtr dataLibrary()
    {
        return _datalibrary;
    }

    void loadDataLibrary(vector<DocumentPtr>& documents);

    static DataLibraryPtr create();

  private:
    // Shared node library used across documents.
    DocumentPtr _datalibrary;
};

extern MX_CORE_API const DataLibraryPtr standardDataLibrary;

MATERIALX_NAMESPACE_END

#endif
