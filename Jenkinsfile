@Library("PSL") _
    
def axisNode = ["GEC-vs2017", "GEC-xcode1021"]

def tasks = [:]

for(int i=0; i< axisNode.size(); i++) {
    def axisNodeValue = axisNode[i]
    tasks["${axisNodeValue}"] = {
        node(axisNodeValue) {
            def components = env.JOB_NAME.split('/')
            env.COMPONENT = components[1].toLowerCase()
            def branch = "${env.BRANCH_NAME}"
            def WorkDir
            def WorkDirComp
            if(axisNodeValue.contains("GEC-vs2017")) {
                if(! branch.contains("PR-")) {
                    WorkDir="D:\\Stage\\workspace\\${env.COMPONENT}\\" + env.BRANCH_NAME.replace("/", "\\")
                } else {
                    WorkDir="D:\\Stage\\workspace\\${env.COMPONENT}\\PR"
                }
                WorkDirComp = WorkDir + "\\${env.COMPONENT}"
            } else {
                if(! branch.contains("PR-")) {
                    WorkDir="/Stage/workspace/${env.COMPONENT}/${env.BRANCH_NAME}"
                } else {
                    WorkDir="/Stage/workspace/${env.COMPONENT}/PR"
                }
                WorkDirComp = WorkDir + "/${env.COMPONENT}"
            }
            try {
                ws("${WorkDirComp}"){
                    stage("Sync") {
                        println "Node=${env.NODE_NAME}"
                    
                        checkout([$class: 'GitSCM', branches: scm.branches, doGenerateSubmoduleConfigurations: false, extensions: [[$class: 'SubmoduleOption', disableSubmodules: false, parentCredentials: true, recursiveSubmodules: true, reference: '', trackingSubmodules: false]], submoduleCfg: [], userRemoteConfigs: scm.userRemoteConfigs])
                        if(axisNodeValue.contains("GEC-vs2017")) {
                            if(! branch.contains("PR-")) {
                                map = readProperties file: "adsk-build-scripts\\adsk_contrib\\engops.properties"
                            } else {
                                echo "This is a PR branch"
                                map = readProperties file: "adsk-build-scripts\\adsk_contrib\\dev.properties"
                            }
                        } else {
                            if(! branch.contains("PR-")) {
                                map = readProperties file: "adsk-build-scripts/adsk_contrib/engops.properties"
                            } else {
                                echo "This is a PR branch"
                                map = readProperties file: "adsk-build-scripts/adsk_contrib/dev.properties"
                            }
                        }
                        properties = map.collect { key, value -> return key+'='+value }
                    }
                    stage("Build") {
                        println (properties)
                        if(axisNodeValue.contains("GEC-vs2017")) {
                            bat "git clean -fdx"
                            bat '''
                            set cmake=C:\\Users\\svc_airbuild\\.jenny\\tools\\cmake\\3.7.1\\bin\\cmake
                            %cmake% -S . -B_mtlxbuild -G "Visual Studio 14 2015 Win64" -DCMAKE_INSTALL_PREFIX=%WORKSPACE%\\install -DMATERIALX_BUILD_PYTHON=OFF  -DMATERIALX_BUILD_RENDER=OFF -DMATERIALX_BUILD_TESTS=OFF -DMATERIALX_DEBUG_POSTFIX=d -DMATERIALX_INSTALL_INCLUDE_PATH=inc -DMATERIALX_INSTALL_LIB_PATH=libs -DMATERIALX_INSTALL_STDLIB_PATH=content/libraries
                            %cmake% --build _mtlxbuild --config Debug
                            %cmake% --build _mtlxbuild --target install
                            %cmake% --build _mtlxbuild --config RelWithDebInfo
                            %cmake% --build _mtlxbuild --config RelWithDebInfo --target install
                            '''
                        } else {
                            sh "git clean -fdx"
                            sh '''
                            cmake -S . -B_mtlxbuild -G "Xcode" -DCMAKE_INSTALL_PREFIX=$WORKSPACE/install -DMATERIALX_BUILD_PYTHON=OFF -DMATERIALX_BUILD_RENDER=OFF -DMATERIALX_BUILD_TESTS=OFF -DMATERIALX_DEBUG_POSTFIX=d -DMATERIALX_INSTALL_INCLUDE_PATH=inc -DMATERIALX_INSTALL_LIB_PATH=libs -DMATERIALX_INSTALL_STDLIB_PATH=content/libraries
                            cmake --build _mtlxbuild --config Debug
                            cmake --build _mtlxbuild --target install
                            cmake --build _mtlxbuild --config RelWithDebInfo
                            cmake --build _mtlxbuild --config RelWithDebInfo --target install
                            '''
                        }
                    }

                    stage("Create Nuget Packages") {
                        withEnv(properties) {
                            if(axisNodeValue.contains("GEC-vs2017")) {
                                bat """
                                nuget pack adsk-build-scripts\\nuget\\win\\adsk_materialx-headers_win.nuspec -Version %Version% -OutputDirectory %WORKSPACE%\\packages -Prop installdir=%WORKSPACE%\\install -Prop materialx=${materialx_version} -Prop win_compiler=${win_compiler}
                                nuget pack adsk-build-scripts\\nuget\\win\\adsk_materialx-lib_win_debug_intel64.nuspec -Version %Version% -OutputDirectory %WORKSPACE%\\packages -Prop installdir=%WORKSPACE%\\install -Prop materialx=${materialx_version} -Prop win_compiler=${win_compiler}
                                nuget pack adsk-build-scripts\\nuget\\win\\adsk_materialx-lib_win_release_intel64.nuspec -Version %Version% -OutputDirectory %WORKSPACE%\\packages -Prop installdir=%WORKSPACE%\\install -Prop materialx=${materialx_version} -Prop win_compiler=${win_compiler}
                                nuget pack adsk-build-scripts\\nuget\\win\\adsk_materialx-content.nuspec -Version %Version% -OutputDirectory %WORKSPACE%\\packages -Prop installdir=%WORKSPACE%\\install -Prop materialx=${materialx_version} -Prop win_compiler=${win_compiler}
                                nuget pack adsk-build-scripts\\nuget\\win\\adsk_materialx-sdk_win_intel64.nuspec -Version %Version% -OutputDirectory %WORKSPACE%\\packages -Prop installdir=%WORKSPACE%\\install -Prop materialx=${materialx_version} -Prop win_compiler=${win_compiler}
                                """
                            } else {
                                sh """
                                nuget pack adsk-build-scripts/nuget/osx/adsk_materialx-headers_osx.nuspec -Version $Version -OutputDirectory $WORKSPACE/packages -Prop installdir=$WORKSPACE/install -Prop materialx=${materialx_version} -Prop osx_compiler=${osx_compiler} -Prop osx_target=${osx_target}
                                nuget pack adsk-build-scripts/nuget/osx/adsk_materialx-lib_osx_debug_intel64.nuspec -Version $Version -OutputDirectory $WORKSPACE/packages -Prop installdir=$WORKSPACE/install -Prop materialx=${materialx_version} -Prop osx_compiler=${osx_compiler} -Prop osx_target=${osx_target}
                                nuget pack adsk-build-scripts/nuget/osx/adsk_materialx-lib_osx_release_intel64.nuspec -Version $Version -OutputDirectory $WORKSPACE/packages -Prop installdir=$WORKSPACE/install -Prop materialx=${materialx_version} -Prop osx_compiler=${osx_compiler} -Prop osx_target=${osx_target}
                                nuget pack adsk-build-scripts/nuget/osx/adsk_materialx-content.nuspec -Version $Version -OutputDirectory $WORKSPACE/packages -Prop installdir=$WORKSPACE/install -Prop materialx=${materialx_version} -Prop osx_compiler=${osx_compiler} -Prop osx_target=${osx_target}
                                nuget pack adsk-build-scripts/nuget/osx/adsk_materialx-sdk_osx_intel64.nuspec -Version $Version -OutputDirectory $WORKSPACE/packages -Prop installdir=$WORKSPACE/install -Prop materialx=${materialx_version} -Prop osx_compiler=${osx_compiler} -Prop osx_target=${osx_target}
                                """
                            }
                        }
                    }
                    stage("Upload Artifactory") {
                        withEnv(properties) {
                            if(!"${env.BRANCH_NAME}".contains("PR-")) {
                                uploadArtifactory()
                            } else {
                                echo "Artifacts not posted for PR branches"
                            }
                        }
                    }
                }
            } catch (caughtError) {
                println ("Error: " + caughtError)
                currentBuild.result="FAILURE"
            }
        }
    }
}

stage("Build") {
    parallel tasks
}

def uploadArtifactory() {
    echo "INFO: Upload to Artifactory"
    withCredentials([usernamePassword(credentialsId: "832c4de4-8264-4b99-856e-e1d2b5dc2ffc", passwordVariable: 'PASSWORD', usernameVariable: 'USERNAME')]) {
    def server = Artifactory.newServer url: 'https://art-bobcat.autodesk.com/artifactory/', username: "${USERNAME}", password: "${PASSWORD}"
        def uploadSpec = """{
            "files": [{
                "pattern": "${env.WORKSPACE}/packages/*.nupkg",
                "target": "oss-stg-nuget/materialx/${env.Version}/",
                "recursive": "false",
                "props":"git.branch=${env.BRANCH_NAME};git.hash=${env.GITCOMMIT}"
            }]
        }"""
        server.upload(uploadSpec)
    }   
}
