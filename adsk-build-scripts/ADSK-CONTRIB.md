# MaterialX build step for Autodesk

- Need debug filename qualifiers
- Need to define custom layout to meet Package Config guidelines.

# Generate steps:
```
cmake -S . -B_mtlxbuild -G "Visual Studio 14 2015 Win64" 
-DCMAKE_INSTALL_PREFIX=%CD%\install 
-DMATERIALX_BUILD_PYTHON=OFF 
-DMATERIALX_TEST_RENDER=OFF 
-DMATERIALX_BUILD_RENDER=OFF
-DMATERIALX_BUILD_TESTS=OFF 
-DMATERIALX_DEBUG_POSTFIX=d 
-DMATERIALX_INSTALL_INCLUDE_PATH=inc 
-DMATERIALX_INSTALL_LIB_PATH=libs 
-DMATERIALX_INSTALL_STDLIB_PATH=content/libraries
```

Slack: #viz-material-interop
Contacts(s): 
ashwin.bhat@autodesk.com
bernard.kwok@autodesk.com 
nicolas.savva@autodesk.com
niklas.harrysson@autodesk.com
