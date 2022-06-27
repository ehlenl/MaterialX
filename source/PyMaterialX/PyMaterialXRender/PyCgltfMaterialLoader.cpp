//
// TM & (c) 2022 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <PyMaterialX/PyMaterialX.h>

#include <MaterialXRender/CgltfMaterialLoader.h>

namespace py = pybind11;
namespace mx = MaterialX;

void bindPyCgltfMaterialLoader(py::module& mod)
{
    py::class_<mx::CgltfMaterialLoader, mx::CgltfMaterialLoaderPtr>(mod, "CgltfMaterialLoader")
        .def_static("create", &mx::CgltfMaterialLoader::create)
        .def(py::init<>())
        .def("load", &mx::CgltfMaterialLoader::load)
        .def("save", &mx::CgltfMaterialLoader::save)
        .def("setDefinitions", &mx::CgltfMaterialLoader::setDefinitions)
        .def("setMaterials", &mx::CgltfMaterialLoader::setMaterials)
        .def("getMaterials", &mx::CgltfMaterialLoader::getMaterials);
}
